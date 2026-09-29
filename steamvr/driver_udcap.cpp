#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <openvr_driver.h>

#include "../src/components/SteamVRBridgeProtocol.h"
#include "../src/components/SteamVRTrackingPresets.h"
#include "HandRenderModel.h"
#include "HandSkeleton.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

namespace {
using Clock = std::chrono::steady_clock;

#if defined(_WIN32)
using NativeSocket = SOCKET;
constexpr NativeSocket invalidSocket = INVALID_SOCKET;
using SocketLength = int;
void closeSocket(NativeSocket socket) { closesocket(socket); }
bool wouldBlock() { return WSAGetLastError() == WSAEWOULDBLOCK; }
bool nonBlocking(NativeSocket socket) {
    u_long enabled = 1;
    return ioctlsocket(socket, FIONBIO, &enabled) == 0;
}
#else
using NativeSocket = int;
constexpr NativeSocket invalidSocket = -1;
using SocketLength = socklen_t;
void closeSocket(NativeSocket socket) { close(socket); }
bool wouldBlock() { return errno == EWOULDBLOCK || errno == EAGAIN; }
bool nonBlocking(NativeSocket socket) {
    const int flags = fcntl(socket, F_GETFL, 0);
    return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}
#endif

float bounded(float value, float low, float high) {
    return std::isfinite(value) ? std::clamp(value, low, high) : 0.0f;
}

vr::HmdQuaternion_t headRotation(const vr::HmdMatrix34_t &matrix) {
    const double trace = matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2];
    vr::HmdQuaternion_t result{};
    if (trace > 0) {
        const double s = std::sqrt(trace + 1.0) * 2.0;
        result.w = 0.25 * s;
        result.x = (matrix.m[2][1] - matrix.m[1][2]) / s;
        result.y = (matrix.m[0][2] - matrix.m[2][0]) / s;
        result.z = (matrix.m[1][0] - matrix.m[0][1]) / s;
    } else if (matrix.m[0][0] > matrix.m[1][1] && matrix.m[0][0] > matrix.m[2][2]) {
        const double s = std::sqrt(1.0 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2]) * 2.0;
        result.w = (matrix.m[2][1] - matrix.m[1][2]) / s;
        result.x = 0.25 * s;
        result.y = (matrix.m[0][1] + matrix.m[1][0]) / s;
        result.z = (matrix.m[0][2] + matrix.m[2][0]) / s;
    } else if (matrix.m[1][1] > matrix.m[2][2]) {
        const double s = std::sqrt(1.0 + matrix.m[1][1] - matrix.m[0][0] - matrix.m[2][2]) * 2.0;
        result.w = (matrix.m[0][2] - matrix.m[2][0]) / s;
        result.x = (matrix.m[0][1] + matrix.m[1][0]) / s;
        result.y = 0.25 * s;
        result.z = (matrix.m[1][2] + matrix.m[2][1]) / s;
    } else {
        const double s = std::sqrt(1.0 + matrix.m[2][2] - matrix.m[0][0] - matrix.m[1][1]) * 2.0;
        result.w = (matrix.m[1][0] - matrix.m[0][1]) / s;
        result.x = (matrix.m[0][2] + matrix.m[2][0]) / s;
        result.y = (matrix.m[1][2] + matrix.m[2][1]) / s;
        result.z = 0.25 * s;
    }
    return result;
}

vr::HmdQuaternion_t multiply(const vr::HmdQuaternion_t &a, const vr::HmdQuaternion_t &b) {
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

vr::HmdQuaternion_t rotationOffset(const float (&degrees)[3]) {
    constexpr double radians = 3.14159265358979323846 / 180.0;
    // UTK's official X preset is 220 degrees. Wrapping preserves that pose;
    // clamping it to 180 degrees changes the intended hand orientation.
    const double x = steamVRWrappedRotation(degrees[0]) * radians * 0.5;
    const double y = steamVRWrappedRotation(degrees[1]) * radians * 0.5;
    const double z = steamVRWrappedRotation(degrees[2]) * radians * 0.5;
    const vr::HmdQuaternion_t pitch{std::cos(x), std::sin(x), 0, 0};
    const vr::HmdQuaternion_t yaw{std::cos(y), 0, std::sin(y), 0};
    const vr::HmdQuaternion_t roll{std::cos(z), 0, 0, std::sin(z)};
    return multiply(multiply(yaw, pitch), roll);
}

bool trackerIsHand(std::uint32_t index, int hand, const std::string &requestedSerial) {
    const auto container = vr::VRProperties()->TrackedDeviceToPropertyContainer(index);
    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    if (vr::VRProperties()->GetInt32Property(container, vr::Prop_DeviceClass_Int32, &error) !=
            vr::TrackedDeviceClass_GenericTracker || error != vr::TrackedProp_Success) return false;
    const auto serial = vr::VRProperties()->GetStringProperty(container, vr::Prop_SerialNumber_String, &error);
    if (error != vr::TrackedProp_Success || serial.empty()) return false;
    if (!requestedSerial.empty()) return serial == requestedSerial;
    const auto registered = vr::VRProperties()->GetStringProperty(
        container, vr::Prop_RegisteredDeviceType_String, &error);
    if (error == vr::TrackedProp_Success) {
        const auto slash = registered.find('/');
        if (slash != std::string::npos) {
            // SteamVR uses the full registered type, e.g. htc/vive_trackerLHR-...
            const std::string key = "/devices/" + registered;
            vr::EVRSettingsError settingsError = vr::VRSettingsError_None;
            char role[64]{};
            vr::VRSettings()->GetString("trackers", key.c_str(), role, sizeof(role), &settingsError);
            if (settingsError == vr::VRSettingsError_None) {
                const std::string assignedRole(role);
                const bool left = assignedRole == "left_hand" || assignedRole == "left" ||
                                  assignedRole == "TrackerRole_LeftHand" ||
                                  assignedRole == "TrackerRole_Handed,TrackedControllerRole_LeftHand";
                const bool right = assignedRole == "right_hand" || assignedRole == "right" ||
                                   assignedRole == "TrackerRole_RightHand" ||
                                   assignedRole == "TrackerRole_Handed,TrackedControllerRole_RightHand";
                if (left || right) return hand == 0 ? left : right;
                if (assignedRole != "TrackerRole_Handed" && assignedRole != "handed" &&
                    assignedRole != "TrackerRole_AnyHand" && assignedRole != "any_hand") return false;
            }
        }
    }
    // For "held in hand", SteamVR assigns the side as a controller role.
    const auto hint = vr::VRProperties()->GetInt32Property(container,
        vr::Prop_ControllerRoleHint_Int32, &error);
    return error == vr::TrackedProp_Success &&
        hint == (hand == 0 ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);
}

class Controller final : public vr::ITrackedDeviceServerDriver {
public:
    explicit Controller(int hand) : hand_(hand) { packet_.hand = static_cast<std::uint8_t>(hand); }

    vr::EVRInitError Activate(std::uint32_t index) override {
        index_ = index;
        const auto container = vr::VRProperties()->TrackedDeviceToPropertyContainer(index);
        vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32,
            hand_ == 0 ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);
        vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, "UdCap Community Glove");
        vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "UdCap Community");
        vr::VRProperties()->SetStringProperty(container, vr::Prop_ControllerType_String, "udcapc_glove");
        vr::VRProperties()->SetStringProperty(container, vr::Prop_RegisteredDeviceType_String,
            hand_ == 0 ? "udcapc/glove_left" : "udcapc/glove_right");
        vr::VRProperties()->SetStringProperty(container, vr::Prop_RenderModelName_String,
            hand_ == 0 ? "{udcapc}udcapc_hand_left" : "{udcapc}udcapc_hand_right");
        // The glove supplies hand input; its backing Tracker supplies only location.
        vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerHandSelectionPriority_Int32, 10);
        const std::string icon = std::string("{udcapc}/icons/") + (hand_ == 0 ? "left_" : "right_");
        for (const auto &entry : std::array<std::pair<vr::ETrackedDeviceProperty, const char *>, 9>{{
                {vr::Prop_NamedIconPathDeviceOff_String, "off.png"},
                {vr::Prop_NamedIconPathDeviceSearching_String, "searching.png"},
                {vr::Prop_NamedIconPathDeviceSearchingAlert_String, "alert.png"},
                {vr::Prop_NamedIconPathDeviceReady_String, "ready.png"},
                {vr::Prop_NamedIconPathDeviceReadyAlert_String, "alert.png"},
                {vr::Prop_NamedIconPathDeviceNotReady_String, "error.png"},
                {vr::Prop_NamedIconPathDeviceStandby_String, "off.png"},
                {vr::Prop_NamedIconPathDeviceStandbyAlert_String, "alert.png"},
                {vr::Prop_NamedIconPathDeviceAlertLow_String, "alert.png"}}})
            vr::VRProperties()->SetStringProperty(container, entry.first, (icon + entry.second).c_str());
        vr::VRProperties()->SetStringProperty(container, vr::Prop_InputProfilePath_String,
            "{udcapc}/input/udcap_profile.json");
        auto *input = vr::VRDriverInput();
        input->CreateBooleanComponent(container, "/input/a/click", &a_);
        input->CreateBooleanComponent(container, "/input/b/click", &b_);
        input->CreateBooleanComponent(container, "/input/application_menu/click", &menu_);
        input->CreateBooleanComponent(container, "/input/joystick/click", &joystickClick_);
        input->CreateScalarComponent(container, "/input/joystick/x", &joystickX_,
                                     vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
        input->CreateScalarComponent(container, "/input/joystick/y", &joystickY_,
                                     vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
        input->CreateBooleanComponent(container, "/input/trigger/click", &triggerClick_);
        input->CreateScalarComponent(container, "/input/trigger/value", &trigger_,
                                     vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
        input->CreateBooleanComponent(container, "/input/grip/click", &gripClick_);
        input->CreateScalarComponent(container, "/input/grip/value", &grip_,
                                     vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
        input->CreateBooleanComponent(container, "/input/trackpad/click", &trackpadClick_);
        const char *side = hand_ == 0 ? "left" : "right";
        const std::string component = std::string("/input/skeleton/") + side;
        const std::string path = std::string("/skeleton/hand/") + side;
        const auto error = input->CreateSkeletonComponent(container, component.c_str(), path.c_str(), "/pose/raw",
                vr::VRSkeletalTracking_Partial, nullptr, 0, &skeleton_);
        if (error != vr::VRInputError_None) {
            logSkeletonError("CreateSkeletonComponent", error);
            Deactivate();
            return vr::VRInitError_Driver_Failed;
        }
        for (std::size_t i = 0; i < renderPoses_.size(); ++i) {
            const auto path = handRenderPosePath(handRenderBones[i]);
            const auto poseError = input->CreatePoseComponent(container, path.c_str(), &renderPoses_[i]);
            if (poseError != vr::VRInputError_None) {
                logSkeletonError("CreatePoseComponent", poseError);
                Deactivate();
                return vr::VRInitError_Driver_Failed;
            }
        }
        // SteamVR decides skeletal availability during activation. The UI may not
        // be running yet, so initialize BOTH ranges before waiting for a packet.
        if (!updateSkeleton(SteamVRBridgePacket{})) {
            Deactivate();
            return vr::VRInitError_Driver_Failed;
        }
        vr::VRDriverLog()->Log(hand_ == 0 ? "UdCap: left hand skeleton initialized (31 bones, both ranges)" :
                                               "UdCap: right hand skeleton initialized (31 bones, both ranges)");
        return vr::VRInitError_None;
    }

    void Deactivate() override {
        index_ = vr::k_unTrackedDeviceIndexInvalid;
        skeleton_ = vr::k_ulInvalidInputComponentHandle;
        renderPoses_.fill(vr::k_ulInvalidInputComponentHandle);
        lastSkeletonError_ = vr::VRInputError_None;
    }
    void EnterStandby() override {}
    void *GetComponent(const char *) override { return nullptr; }
    void DebugRequest(const char *, char *output, std::uint32_t size) override {
        if (size && output) output[0] = 0;
    }

    vr::DriverPose_t GetPose() override {
        vr::DriverPose_t pose{};
        pose.qWorldFromDriverRotation.w = 1;
        pose.qDriverFromHeadRotation.w = 1;
        pose.qRotation.w = 1;
        const bool fresh = lastPacket_ != Clock::time_point{} &&
                           Clock::now() - lastPacket_ < std::chrono::seconds(1);
        pose.deviceIsConnected = fresh && packet_.connected;
        pose.result = pose.deviceIsConnected ? vr::TrackingResult_Running_OK :
                                                vr::TrackingResult_Running_OutOfRange;
        if (!pose.deviceIsConnected) return pose;
        std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount> devices{};
        vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0, devices.data(),
            static_cast<std::uint32_t>(devices.size()));
        const auto *serialEnd = static_cast<const char *>(
            std::memchr(packet_.trackerSerial, 0, sizeof(packet_.trackerSerial)));
        if (!serialEnd) {
            pose.result = vr::TrackingResult_Running_OutOfRange;
            return pose;
        }
        const std::string requestedSerial(packet_.trackerSerial,
            static_cast<std::size_t>(serialEnd - packet_.trackerSerial));
        for (std::uint32_t i = 0; i < devices.size(); ++i) {
            if (!devices[i].bDeviceIsConnected) continue;
            if (!trackerIsHand(i, hand_, requestedSerial)) continue;
            const auto &tracker = devices[i];
            if (!tracker.bPoseIsValid || tracker.eTrackingResult != vr::TrackingResult_Running_OK) {
                pose.result = vr::TrackingResult_Running_OutOfRange;
                return pose;
            }
            const auto &m = tracker.mDeviceToAbsoluteTracking;
            pose.qRotation = multiply(headRotation(m), rotationOffset(packet_.trackerRotationDegrees));
            for (int axis = 0; axis < 3; ++axis) {
                pose.vecPosition[axis] = m.m[axis][3];
                for (int local = 0; local < 3; ++local)
                    pose.vecPosition[axis] += m.m[axis][local] *
                        bounded(packet_.trackerOffsetMeters[local], -1, 1);
            }
            pose.poseIsValid = true;
            return pose;
        }
        pose.result = vr::TrackingResult_Running_OutOfRange;
        return pose;
    }

    void SetPacket(const SteamVRBridgePacket &packet) {
        packet_ = packet;
        lastPacket_ = Clock::now();
    }

    void RunFrame() {
        if (index_ == vr::k_unTrackedDeviceIndexInvalid) return;
        const auto pose = GetPose();
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(index_, pose, sizeof(pose));
        auto *input = vr::VRDriverInput();
        const bool active = pose.deviceIsConnected;
        input->UpdateBooleanComponent(a_, active && packet_.buttonA, 0);
        input->UpdateBooleanComponent(b_, active && packet_.buttonB, 0);
        input->UpdateBooleanComponent(menu_, active && packet_.buttonMenu, 0);
        input->UpdateBooleanComponent(joystickClick_, active && packet_.buttonJoystick, 0);
        input->UpdateBooleanComponent(triggerClick_, active && packet_.buttonTrigger, 0);
        input->UpdateBooleanComponent(gripClick_, active && packet_.buttonGrip, 0);
        input->UpdateBooleanComponent(trackpadClick_, active && packet_.buttonTrackpad, 0);
        input->UpdateScalarComponent(joystickX_, active ? bounded(packet_.joystickX, -1, 1) : 0, 0);
        input->UpdateScalarComponent(joystickY_, active ? bounded(packet_.joystickY, -1, 1) : 0, 0);
        input->UpdateScalarComponent(trigger_, active ? bounded(packet_.trigger, 0, 1) : 0, 0);
        input->UpdateScalarComponent(grip_, active ? bounded(packet_.grip, 0, 1) : 0, 0);
        // Preserve skeletal availability across glove reconnects. Connection and
        // tracking validity are still reported separately by the device pose.
        updateSkeleton(active ? packet_ : SteamVRBridgePacket{});
    }

private:
    void logSkeletonError(const char *operation, vr::EVRInputError error) const {
        const std::string message = std::string("UdCap: ") + (hand_ == 0 ? "left " : "right ") +
            operation + " failed, VRInputError=" + std::to_string(static_cast<int>(error));
        vr::VRDriverLog()->Log(message.c_str());
    }
    bool updateSkeleton(const SteamVRBridgePacket &packet) {
        if (skeleton_ == vr::k_ulInvalidInputComponentHandle) return false;
        const auto bones = makeBones(packet, hand_);
        const auto with = vr::VRDriverInput()->UpdateSkeletonComponent(skeleton_,
            vr::VRSkeletalMotionRange_WithController, bones.data(), static_cast<std::uint32_t>(bones.size()));
        const auto without = vr::VRDriverInput()->UpdateSkeletonComponent(skeleton_,
            vr::VRSkeletalMotionRange_WithoutController, bones.data(), static_cast<std::uint32_t>(bones.size()));
        auto error = with != vr::VRInputError_None ? with : without;
        // A Render Model OBJ is not skinned by UpdateSkeletonComponent. SteamVR
        // animates its separate meshes through pose_component motions instead.
        const auto transforms = handRenderTransforms(bones);
        for (std::size_t i = 0; i < renderPoses_.size(); ++i) {
            const auto poseError = vr::VRDriverInput()->UpdatePoseComponent(renderPoses_[i], &transforms[i], 0);
            if (poseError != vr::VRInputError_None && error == vr::VRInputError_None) error = poseError;
        }
        if (error != vr::VRInputError_None && error != lastSkeletonError_)
            logSkeletonError("UpdateSkeletonComponent/UpdatePoseComponent", error);
        lastSkeletonError_ = error;
        return error == vr::VRInputError_None;
    }
    int hand_;
    std::uint32_t index_ = vr::k_unTrackedDeviceIndexInvalid;
    SteamVRBridgePacket packet_{};
    Clock::time_point lastPacket_{};
    vr::VRInputComponentHandle_t a_{}, b_{}, menu_{}, joystickClick_{}, joystickX_{}, joystickY_{};
    vr::VRInputComponentHandle_t triggerClick_{}, trigger_{}, gripClick_{}, grip_{};
    vr::VRInputComponentHandle_t trackpadClick_{};
    vr::VRInputComponentHandle_t skeleton_ = vr::k_ulInvalidInputComponentHandle;
    std::array<vr::VRInputComponentHandle_t, 16> renderPoses_{};
    vr::EVRInputError lastSkeletonError_ = vr::VRInputError_None;
};

class Provider final : public vr::IServerTrackedDeviceProvider {
public:
    vr::EVRInitError Init(vr::IVRDriverContext *context) override {
        VR_INIT_SERVER_DRIVER_CONTEXT(context);
#if defined(_WIN32)
        WSADATA winsock{};
        if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0) {
            VR_CLEANUP_SERVER_DRIVER_CONTEXT();
            return vr::VRInitError_Driver_Failed;
        }
        winsockReady_ = true;
#endif
        socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        local.sin_port = htons(steamVrBridgePort);
        if (socket_ == invalidSocket ||
            ::bind(socket_, reinterpret_cast<const sockaddr *>(&local), sizeof(local)) != 0 ||
            !nonBlocking(socket_)) {
            vr::VRDriverLog()->Log("UdCap: failed to bind localhost UDP port 8998");
            if (socket_ != invalidSocket) closeSocket(socket_);
            socket_ = invalidSocket;
#if defined(_WIN32)
            WSACleanup();
            winsockReady_ = false;
#endif
            VR_CLEANUP_SERVER_DRIVER_CONTEXT();
            return vr::VRInitError_Driver_Failed;
        }
        left_ = std::make_unique<Controller>(0);
        right_ = std::make_unique<Controller>(1);
        vr::VRServerDriverHost()->TrackedDeviceAdded("UDCAP-COMMUNITY-LEFT", vr::TrackedDeviceClass_Controller, left_.get());
        vr::VRServerDriverHost()->TrackedDeviceAdded("UDCAP-COMMUNITY-RIGHT", vr::TrackedDeviceClass_Controller, right_.get());
        return vr::VRInitError_None;
    }

    void Cleanup() override {
        right_.reset();
        left_.reset();
        if (socket_ != invalidSocket) closeSocket(socket_);
        socket_ = invalidSocket;
#if defined(_WIN32)
        if (winsockReady_) WSACleanup();
        winsockReady_ = false;
#endif
        VR_CLEANUP_SERVER_DRIVER_CONTEXT();
    }
    const char *const *GetInterfaceVersions() override { return vr::k_InterfaceVersions; }
    void RunFrame() override {
        if (socket_ != invalidSocket) {
            for (int i = 0; i < 128; ++i) {
                SteamVRBridgePacket packet{};
                std::array<char, sizeof(SteamVRBridgePacket) + 1> datagram{};
                sockaddr_in sender{};
                SocketLength senderSize = sizeof(sender);
                const int count = ::recvfrom(socket_, datagram.data(), static_cast<int>(datagram.size()), 0,
                                             reinterpret_cast<sockaddr *>(&sender), &senderSize);
                if (count < 0 && wouldBlock()) break;
                if (count < 0) break;
                if (sender.sin_family != AF_INET ||
                    sender.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) continue;
                if (count != sizeof(packet)) continue;
                std::memcpy(&packet, datagram.data(), sizeof(packet));
                if (packet.magic != steamVrBridgeMagic || packet.version != steamVrBridgeVersion ||
                    packet.hand > 1) continue;
                (packet.hand == 0 ? left_ : right_)->SetPacket(packet);
            }
        }
        vr::VREvent_t event{};
        while (vr::VRServerDriverHost()->PollNextEvent(&event, sizeof(event))) {}
        if (left_) left_->RunFrame();
        if (right_) right_->RunFrame();
    }
    bool ShouldBlockStandbyMode() override { return false; }
    void EnterStandby() override {}
    void LeaveStandby() override {}

private:
    NativeSocket socket_ = invalidSocket;
#if defined(_WIN32)
    bool winsockReady_ = false;
#endif
    std::unique_ptr<Controller> left_, right_;
};

Provider provider;
}

#if defined(_WIN32)
#define UDCAP_EXPORT extern "C" __declspec(dllexport)
#else
#define UDCAP_EXPORT extern "C" __attribute__((visibility("default")))
#endif

UDCAP_EXPORT void *HmdDriverFactory(const char *interfaceName, int *returnCode) {
    if (std::strcmp(interfaceName, vr::IServerTrackedDeviceProvider_Version) == 0) return &provider;
    if (returnCode) *returnCode = vr::VRInitError_Init_InterfaceNotFound;
    return nullptr;
}

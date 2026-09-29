#include "SteamVRSender.h"
#include "UserConfig.h"
#include "GloveConfig.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {
SteamVRBridgeQuaternion convert(const BoneQuaternion &q) {
    return {q.x, q.y, q.z, q.w};
}

void copySkeleton(SteamVRBridgePacket &out, const HandQuaternion &in) {
    const FingerQuaternion fingers[5]{in.thumbFinger, in.indexFinger, in.middleFinger,
                                       in.ringFinger, in.littleFinger};
    for (int i = 0; i < 5; ++i) {
        out.bones[i * 3] = convert(fingers[i].proximal);
        out.bones[i * 3 + 1] = convert(fingers[i].intermediate);
        out.bones[i * 3 + 2] = convert(fingers[i].distal);
    }
}
}

SteamVRSender::SteamVRSender()
    : socket(io), endpoint(boost::asio::ip::make_address("127.0.0.1"), steamVrBridgePort) {
    socket.open(boost::asio::ip::udp::v4());
    state[0].hand = 0;
    state[1].hand = 1;
    reloadTrackingOffsets();
}

SteamVRSender::~SteamVRSender() {
    for (auto &stop : unlisten) if (stop) stop();
    std::lock_guard lock(mutex);
    for (auto &packet : state) {
        packet.connected = 0;
        boost::system::error_code error;
        socket.send_to(boost::asio::buffer(&packet, sizeof(packet)), endpoint, 0, error);
    }
    boost::system::error_code error;
    socket.close(error);
#if defined(UDCAP_HAVE_OPENVR_CLIENT)
    if (vrSystem) vr::VR_Shutdown();
#endif
}

#if defined(UDCAP_HAVE_OPENVR_CLIENT)
void SteamVRSender::refreshTrackerRoles() {
    const auto now = std::chrono::steady_clock::now();
    if (lastRoleRefresh != std::chrono::steady_clock::time_point{} &&
        now - lastRoleRefresh < std::chrono::seconds(1)) return;
    lastRoleRefresh = now;
    if (!vrSystem) {
        vr::EVRInitError error = vr::VRInitError_None;
        vrSystem = vr::VR_Init(&error, vr::VRApplication_Background);
        if (error != vr::VRInitError_None) {
            vrSystem = nullptr;
            return;
        }
    }
    std::array<std::string, 2> resolved{};
    std::array<bool, 2> ambiguous{};
    for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
        if (!vrSystem->IsTrackedDeviceConnected(i) ||
            vrSystem->GetTrackedDeviceClass(i) != vr::TrackedDeviceClass_GenericTracker) continue;
        const auto role = vrSystem->GetControllerRoleForTrackedDeviceIndex(i);
        const int hand = role == vr::TrackedControllerRole_LeftHand ? 0 :
                         role == vr::TrackedControllerRole_RightHand ? 1 : -1;
        if (hand < 0) continue;
        char serial[steamVrTrackerSerialBytes]{};
        vr::ETrackedPropertyError error = vr::TrackedProp_Success;
        vrSystem->GetStringTrackedDeviceProperty(i, vr::Prop_SerialNumber_String,
                                                 serial, sizeof(serial), &error);
        if (error != vr::TrackedProp_Success || serial[0] == 0) continue;
        if (!resolved[hand].empty()) ambiguous[hand] = true;
        else resolved[hand] = serial;
    }
    for (int hand = 0; hand < 2; ++hand) {
        auto &target = state[hand].trackerSerial;
        std::memset(target, 0, sizeof(target));
        if (!ambiguous[hand]) std::memcpy(target, resolved[hand].data(), resolved[hand].size());
    }
}
#endif

void SteamVRSender::setTrackingOffset(int hand, const std::array<float, 3> &positionMeters,
                              const std::array<float, 3> &rotationDegrees) {
    if (hand < 0 || hand > 1) throw std::invalid_argument("Invalid SteamVR hand");
    std::lock_guard lock(mutex);
    auto &packet = state[hand];
    std::copy(positionMeters.begin(), positionMeters.end(), packet.trackerOffsetMeters);
    std::copy(rotationDegrees.begin(), rotationDegrees.end(), packet.trackerRotationDegrees);
    boost::system::error_code error;
    socket.send_to(boost::asio::buffer(&packet, sizeof(packet)), endpoint, 0, error);
}

void SteamVRSender::reloadTrackingOffsets() {
    constexpr std::array<const char *, 3> axes{"x", "y", "z"};
    auto &config = UserConfig::getInstance();
    for (int hand = 0; hand < 2; ++hand) {
        const bool advanced = config.get<int>("/core/driverType", 0) == 1;
        const std::string prefix = advanced && !gloveSerials[hand].empty()
            ? gloveConfigPrefix(gloveSerials[hand]) + "/steamvr"
            : hand == 0 ? "/consumer/steamvr/left" : "/consumer/steamvr/right";
        std::array<float, 3> position{}, rotation{};
        for (int axis = 0; axis < 3; ++axis) {
            position[axis] = static_cast<float>(config.get<double>(prefix + "/position/" + axes[axis], 0));
            rotation[axis] = static_cast<float>(config.get<double>(prefix + "/rotation/" + axes[axis], 0));
        }
        setTrackingOffset(hand, position, rotation);
    }
}

void SteamVRSender::add(const std::shared_ptr<UdCapV1Core> &core) {
    if (!core) throw std::invalid_argument("Core cannot be null");
    const auto target = core->getTarget();
    if (target != UD_TARGET_LEFT_HAND && target != UD_TARGET_RIGHT_HAND)
        throw std::invalid_argument("Unknown glove hand");
    const std::size_t index = target == UD_TARGET_LEFT_HAND ? 0 : 1;
    if (unlisten[index]) throw std::runtime_error("SteamVR hand already attached");
    gloveSerials[index] = core->getUDCapSerial();
    reloadTrackingOffsets();
    state[index].connected = 1;
    unlisten[index] = core->listen([this, index](std::shared_ptr<UdCapV1MCUPacket> data) {
        std::lock_guard lock(mutex);
#if defined(UDCAP_HAVE_OPENVR_CLIENT)
        refreshTrackerRoles();
#endif
        auto &packet = state[index];
        switch (data->commandType) {
            case CMD_LINK_STATE:
                packet.connected = data->udState == UD_INIT_STATE_LINKED;
                break;
            case CMD_READY:
                packet.connected = data->isReady;
                break;
            case CMD_SKELETON_QUATERNION:
                copySkeleton(packet, data->skeletonQuaternion);
                break;
            case CMD_INPUT_JOYSTICK:
                packet.joystickX = data->joystickData.joyX;
                packet.joystickY = data->joystickData.joyY;
                break;
            case CMD_INPUT_BUTTON:
                packet.trigger = data->button.trigger;
                packet.grip = data->button.grip;
                packet.trackpad = data->button.trackpad;
                packet.buttonA = data->button.btnA;
                packet.buttonB = data->button.btnB;
                packet.buttonMenu = data->button.btnMenu;
                packet.buttonJoystick = data->button.btnJoyStick;
                packet.buttonTrigger = data->button.btnTrigger;
                packet.buttonGrip = data->button.btnGrip;
                packet.buttonTrackpad = data->button.btnTrackpad;
                break;
            default:
                return;
        }
        boost::system::error_code error;
        socket.send_to(boost::asio::buffer(&packet, sizeof(packet)), endpoint, 0, error);
    });
}

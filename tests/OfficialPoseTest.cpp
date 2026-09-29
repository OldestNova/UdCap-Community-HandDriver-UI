#include <UdCapV1Core.h>
#include <HandOffsetProjection.h>
#include "components/PoseCoordinates.h"
#include "components/QingTongPose.h"
#include "components/VRChatPackets.h"
#include "components/SteamVRTrackingPresets.h"
#include <oscpp/server.hpp>

#include <array>
#include <cmath>
#include <iostream>
#include <utility>
#include <map>
#include <string>

namespace {

bool nearlyEqual(float actual, double expected) {
    return std::abs(static_cast<double>(actual) - expected) < 0.0001;
}

bool rotationAround(const BoneQuaternion &q, char axis, double degrees) {
    const double half = degrees * 3.14159265358979323846 / 360.0;
    const double component = std::sin(half);
    return nearlyEqual(q.x, axis == 'x' ? component : 0) &&
           nearlyEqual(q.y, axis == 'y' ? component : 0) &&
           nearlyEqual(q.z, axis == 'z' ? component : 0) &&
           nearlyEqual(q.w, std::cos(half));
}

bool check(bool condition, const char *message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

bool previewMirrors(const BoneQuaternion &left, const BoneQuaternion &right) {
    return nearlyEqual(left.x, right.x) && nearlyEqual(left.y, -right.y) &&
           nearlyEqual(left.z, -right.z) && nearlyEqual(left.w, right.w);
}

BoneQuaternion multiply(const BoneQuaternion &a, const BoneQuaternion &b) {
    return {a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
            a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
            a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
            a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z};
}
bool same(const BoneQuaternion &a, const BoneQuaternion &b) {
    return nearlyEqual(a.x,b.x) && nearlyEqual(a.y,b.y) && nearlyEqual(a.z,b.z) && nearlyEqual(a.w,b.w);
}
std::array<BoneQuaternion,15> joints(const HandQuaternion &pose) {
    std::array<BoneQuaternion,15> out{};
    std::size_t i=0;
    for (const auto &f : {pose.thumbFinger,pose.indexFinger,pose.middleFinger,pose.ringFinger,pose.littleFinger})
        for (const auto &q : {f.proximal,f.intermediate,f.distal}) out[i++]=q;
    return out;
}

} // namespace

int main() {
    const std::array<float, 3> thumbFix{0.3f, 0.3f, 1.2f};
    HandRotation offset{};
    std::array<double, 28> angles{};
    angles[6] = 30; // Index proximal curl
    auto left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
    auto right = UdCapV1Core::officialHandPose(angles, UD_TARGET_RIGHT_HAND, offset, thumbFix);
    if (!check(rotationAround(left.indexFinger.proximal, 'z', -30), "Left index curl differs from official VMC") ||
        !check(rotationAround(right.indexFinger.proximal, 'z', 30), "Right index curl differs from official VMC")) return 1;

    angles = {};
    angles[21] = 20; // Index root spread
    angles[11] = 25; // Middle root yaw
    angles[15] = 15; // Ring root yaw
    left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
    right = UdCapV1Core::officialHandPose(angles, UD_TARGET_RIGHT_HAND, offset, thumbFix);
    if (!check(rotationAround(left.indexFinger.proximal, 'x', 20), "Left index spread is inverted") ||
        !check(rotationAround(right.indexFinger.proximal, 'x', 20), "Right index spread is inverted") ||
        !check(rotationAround(left.middleFinger.proximal, 'y', 25), "Left middle yaw is inverted") ||
        !check(rotationAround(right.middleFinger.proximal, 'y', 25), "Right middle yaw is inverted") ||
        !check(rotationAround(left.ringFinger.proximal, 'y', -15), "Left ring yaw is inverted") ||
        !check(rotationAround(right.ringFinger.proximal, 'y', 15), "Right ring yaw is inverted")) return 1;

    angles = {};
    angles[2] = 40; // Thumb root Y, official 0.3 scale
    angles[1] = 35; // Thumb middle yaw
    left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
    right = UdCapV1Core::officialHandPose(angles, UD_TARGET_RIGHT_HAND, offset, thumbFix);
    if (!check(rotationAround(left.thumbFinger.proximal, 'y', 12), "Left thumb root is inverted") ||
        !check(rotationAround(right.thumbFinger.proximal, 'y', -12), "Right thumb root is inverted") ||
        !check(rotationAround(left.thumbFinger.intermediate, 'y', 35), "Left thumb middle is inverted") ||
        !check(rotationAround(right.thumbFinger.intermediate, 'y', -35), "Right thumb middle is inverted")) return 1;

    angles = {};
    angles[3] = 20; // Official thumb root X does not change sign with hand.
    left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
    right = UdCapV1Core::officialHandPose(angles, UD_TARGET_RIGHT_HAND, offset, thumbFix);
    if (!check(rotationAround(left.thumbFinger.proximal, 'x', 24), "Left thumb X differs from official VMC") ||
        !check(rotationAround(right.thumbFinger.proximal, 'x', 24), "Right thumb X differs from official VMC")) return 1;
    angles = {};
    angles[20] = 30; // Official thumb root Z mirrors between hands.
    left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
    right = UdCapV1Core::officialHandPose(angles, UD_TARGET_RIGHT_HAND, offset, thumbFix);
    if (!check(rotationAround(left.thumbFinger.proximal, 'z', -9), "Left thumb Z differs from official VMC") ||
        !check(rotationAround(right.thumbFinger.proximal, 'z', 9), "Right thumb Z differs from official VMC")) return 1;

    // Preview has its own bone axes. Equal motions must remain mirror images
    // across the two hands, including the thumb root and distal joints.
    for (const auto [channel, finger] : std::array<std::pair<std::size_t, std::size_t>, 7>{{
             {3, 0}, {2, 0}, {20, 0}, {7, 1}, {11, 2}, {15, 3}, {19, 4}}}) {
        angles = {};
        angles[channel] = 20;
        left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
        right = UdCapV1Core::officialHandPose(angles, UD_TARGET_RIGHT_HAND, offset, thumbFix);
        const std::array<BoneQuaternion, 5> leftRoot{
            left.thumbFinger.proximal, left.indexFinger.proximal,
            left.middleFinger.proximal, left.ringFinger.proximal,
            left.littleFinger.proximal};
        const std::array<BoneQuaternion, 5> rightRoot{
            right.thumbFinger.proximal, right.indexFinger.proximal,
            right.middleFinger.proximal, right.ringFinger.proximal,
            right.littleFinger.proximal};
        if (!check(previewMirrors(poseForPreview(leftRoot[finger], UD_TARGET_LEFT_HAND, finger, 0),
                                  poseForPreview(rightRoot[finger], UD_TARGET_RIGHT_HAND, finger, 0)),
                   "Preview root motion is not mirrored")) return 1;
    }
    angles = {};
    angles[1] = 35;
    left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
    right = UdCapV1Core::officialHandPose(angles, UD_TARGET_RIGHT_HAND, offset, thumbFix);
    if (!check(previewMirrors(poseForPreview(left.thumbFinger.intermediate, UD_TARGET_LEFT_HAND, 0, 1),
                              poseForPreview(right.thumbFinger.intermediate, UD_TARGET_RIGHT_HAND, 0, 1)),
               "Preview thumb middle motion is not mirrored")) return 1;

    // VMC must preserve ALL 15 official local bone rotations.
    angles = {};
    angles[21] = 20;
    angles[6] = 30;
    angles[1] = 35;
    left = UdCapV1Core::officialHandPose(angles, UD_TARGET_LEFT_HAND, offset, thumbFix);
    auto vmc = poseForVMC(left);
    if (!check(nearlyEqual(vmc.indexFinger.proximal.x, left.indexFinger.proximal.x) &&
               nearlyEqual(vmc.indexFinger.proximal.y, left.indexFinger.proximal.y) &&
               nearlyEqual(vmc.indexFinger.proximal.z, left.indexFinger.proximal.z),
               "VMC MCP spread and curl adapter differs") ||
        !check(nearlyEqual(vmc.thumbFinger.intermediate.y, left.thumbFinger.intermediate.y) &&
               nearlyEqual(vmc.indexFinger.intermediate.z, left.indexFinger.intermediate.z),
               "VMC changed a thumb or distal joint")) return 1;
    // SteamVR's full skeleton conversion is covered by SteamVRDriverTest.

    // All three rotations are needed to distinguish the official Z * Y * X
    // formula from Y * X * Z. Compare the components against the decompiled
    // official OSCManager.EulerToQuaternion calculation.
    const double hx = 30.0 * 3.14159265358979323846 / 360.0;
    const double hy = 40.0 * 3.14159265358979323846 / 360.0;
    const double hz = 50.0 * 3.14159265358979323846 / 360.0;
    const double sx = std::sin(hx), sy = std::sin(hy), sz = std::sin(hz);
    const double cx = std::cos(hx), cy = std::cos(hy), cz = std::cos(hz);
    const auto mixed = UdCapV1Core::eulerToQuaternion(30, 40, 50);
    if (!check(nearlyEqual(mixed.x, sx * cy * cz - cx * sy * sz) &&
               nearlyEqual(mixed.y, cx * sy * cz + sx * cy * sz) &&
               nearlyEqual(mixed.z, cx * cy * sz - sx * sy * cz) &&
               nearlyEqual(mixed.w, cx * cy * cz + sx * sy * sz),
               "Official three-axis quaternion formula differs")) return 1;

    // Independently enumerate every official v0.1.8.6 OSCManager Euler tuple.
    for (std::size_t i=0; i<23; ++i) angles[i] = double(i+1);
    for (const auto hand : {UD_TARGET_LEFT_HAND, UD_TARGET_RIGHT_HAND}) {
        const double s = hand == UD_TARGET_LEFT_HAND ? -1 : 1;
        const std::array<std::array<double,3>,15> eulers{{
            {4*1.2, -s*3*.3, s*21*.3}, {0,-s*2,0}, {0,-s,0},
            {22,-s*8,s*7}, {0,0,s*6}, {0,0,s*5},
            {0,12,s*11}, {0,0,s*10}, {0,0,s*9},
            {0,s*16,s*15}, {0,0,s*14}, {0,0,s*13},
            {23,s*20,s*19}, {0,0,s*18}, {0,0,s*17}
        }};
        const auto actual = joints(poseForVMC(UdCapV1Core::officialHandPose(angles, hand, {}, thumbFix)));
        for (std::size_t i=0;i<15;++i) {
            const auto &e = eulers[i];
            if (!check(same(actual[i], UdCapV1Core::eulerToQuaternion(e[0],e[1],e[2])),
                       "VMC joint differs from official Euler tuple")) return 1;
        }
        // All preview maps must preserve composition and nonzero X/Y/Z offsets.
        const auto a = UdCapV1Core::eulerToQuaternion(17,29,-43);
        const auto b = UdCapV1Core::eulerToQuaternion(-31,11,23);
        for (std::size_t finger=0;finger<5;++finger) for(std::size_t joint=0;joint<3;++joint) {
            const auto pa = poseForPreview(a,hand,finger,joint);
            if (!check(nearlyEqual(pa.x*pa.x+pa.y*pa.y+pa.z*pa.z+pa.w*pa.w,1) &&
                       same(poseForPreview(multiply(a,b),hand,finger,joint),
                            multiply(pa,poseForPreview(b,hand,finger,joint))),
                       "Preview drops an axis or breaks quaternion composition")) return 1;
        }
        // A negative official flex angle must take the local +Y tip into -Z.
        const auto curled = UdCapV1Core::eulerToQuaternion(0,0,s*-60);
        for (std::size_t finger=1;finger<5;++finger) for(std::size_t joint=0;joint<3;++joint) {
            const auto q = poseForPreview(curled,hand,finger,joint);
            if (!check(2*(q.y*q.z+q.w*q.x) < -.8, "Preview finger curls away from palm")) return 1;
        }
        // Opposing splay directions: index toward center, ring/little outward.
        angles = {};
        angles[7]=angles[15]=angles[19]=20;
        const auto spread = joints(UdCapV1Core::officialHandPose(angles,hand,{},thumbFix));
        for (std::size_t finger : {1u,3u,4u}) {
            const auto q = poseForPreview(spread[finger*3],hand,finger,0);
            const float tipX=2*(q.x*q.y-q.w*q.z);
            if (!check((finger==1 ? -s : s)*tipX > .3, "Preview splay points toward wrong neighbor")) return 1;
        }
        for (std::size_t i=0; i<23; ++i) angles[i] = double(i+1);
    }

    // Decode the real OSC bytes, not the builder's in-memory parameters.
    for (const auto hand : {UD_TARGET_LEFT_HAND,UD_TARGET_RIGHT_HAND}) {
        std::map<std::string,bool> parameters;
        std::size_t packets=0;
        angles={};
        forEachVRChatPacket(angles,hand,[&](const void *data,std::size_t size) {
            OSCPP::Server::Packet packet(data,size);
            if (!packet.isBundle() || size>1400) throw std::runtime_error("Invalid OSC bundle");
            const OSCPP::Server::Bundle bundle=packet;
            if (bundle.time()!=1) throw std::runtime_error("Invalid OSC timetag");
            auto messages=bundle.packets();
            while(!messages.atEnd()) {
                const OSCPP::Server::Message message=messages.next();
                auto args=message.args();
                if(args.size()!=1 || (args.tag()!='T' && args.tag()!='F')) throw std::runtime_error("Invalid OSC boolean");
                if(!parameters.emplace(message.address(),args.tag()=='T').second) throw std::runtime_error("Duplicate OSC parameter");
            }
            ++packets;
        });
        const std::string prefix=hand==UD_TARGET_LEFT_HAND ? "/avatar/parameters/Left" : "/avatar/parameters/Right";
        const auto bits=[&](const char *name) {
            unsigned value=0;
            for(unsigned bit : {1u,2u,4u,8u}) if(parameters.at(prefix+name+std::to_string(bit))) value|=bit;
            return value;
        };
        if(!check(packets==5 && parameters.size()==60 &&
                  bits("Thumb1")== (hand==UD_TARGET_LEFT_HAND ? 2u : 6u) &&
                  bits("Thumb1spread")==1 && bits("Pinky1spread")==15,
                  "VRChat OSC names/quantization differ from official avatar contract")) return 1;
        for(const auto &[name,value] : parameters) if(!check(name.starts_with(prefix),"OSC hand crossed over")) return 1;
    }
    // QingTong uses an independent Unity Y*X*Z product, followed by protocol Z negation.
    auto qt=qingTongEuler(30,40,50);
    const auto qy=UdCapV1Core::eulerToQuaternion(0,40,0);
    const auto qx=UdCapV1Core::eulerToQuaternion(30,0,0);
    const auto qz=UdCapV1Core::eulerToQuaternion(0,0,50);
    auto qtReference=multiply(multiply(qy,qx),qz); qtReference.z=-qtReference.z;
    if(!check(same(qt,qtReference),"QingTong rotation order differs")) return 1;
    angles={}; angles[6]=-60; angles[3]=20; angles[2]=40; angles[20]=30;
    const auto qtPose=poseForQingTong(angles);
    if(!check(rotationAround(qtPose.indexFinger.proximal,'z',-60) &&
              same(qtPose.thumbFinger.proximal,qingTongEuler(-20,40,30)),
              "QingTong accidentally reused VMC hand signs or thumb gains")) return 1;
    // Inverting the measured offset DOFs must reconstruct every Core joint,
    // including all three thumb root axes and both thumb hinge segments.
    for(const auto hand:{UD_TARGET_LEFT_HAND,UD_TARGET_RIGHT_HAND}) {
        HandRotation measured{};
        measured.thumbFinger={BoneRotation{6,-3,9},BoneRotation{0,7,0},BoneRotation{0,-5,0}};
        measured.indexFinger={BoneRotation{3,4,8},BoneRotation{0,0,-3},BoneRotation{0,0,5}};
        measured.middleFinger={BoneRotation{0,-6,7},BoneRotation{0,0,-4},BoneRotation{0,0,6}};
        measured.ringFinger=measured.middleFinger;measured.littleFinger=measured.indexFinger;
        const auto projected=anglesWithHandOffset(angles,hand,measured);
        const auto original=joints(UdCapV1Core::officialHandPose(angles,hand,measured,thumbFix));
        const auto reconstructed=joints(UdCapV1Core::officialHandPose(projected,hand,{},thumbFix));
        const auto qtProjected=joints(poseForQingTong(projected));
        const auto qtOffset=joints(poseForQingTong(angles,hand,measured));
        for(std::size_t i=0;i<15;++i)
            if(!check(same(original[i],reconstructed[i]) && same(qtProjected[i],qtOffset[i]),
                      "Protocol offset projection disagrees with Core bone pose")) return 1;
    }
    HandRotation full{};
    full.thumbFinger.proximal={6,-3,9}; full.indexFinger.intermediate={2,3,4};
    const auto qtl=poseForQingTong({},UD_TARGET_LEFT_HAND,full);
    const auto qtr=poseForQingTong({},UD_TARGET_RIGHT_HAND,full);
    if(!check(same(qtl.thumbFinger.proximal,qingTongEuler(-5,-10,-30)) &&
              same(qtr.thumbFinger.proximal,qingTongEuler(-5,10,30)) &&
              same(qtl.indexFinger.intermediate,qingTongEuler(2,-3,4)) &&
              same(qtr.indexFinger.intermediate,qingTongEuler(2,3,-4)),
              "QingTong full local offsets lost an axis")) return 1;
    // The display fixtures come from official VRSettingWindow in 0.1.8.6;
    // bridge fixtures are the values it sends to its OpenVR driver.
    constexpr std::array<SteamVRTrackingOffset, 4> officialRight{{
        {{-0.10, 0.10, -0.05}, {45, -85, 0}},
        {{-0.09, -0.09, -0.04}, {60, -60, 105}},
        {{-0.10, -0.10, 0.02}, {-35, -20, 0}},
        {{0.09, -0.07, -0.07}, {220, -11, 80}}
    }};
    constexpr std::array<SteamVRTrackingOffset, 4> officialLeft{{
        {{0.10, 0.10, -0.05}, {45, 85, 0}},
        {{-0.09, 0.09, -0.04}, {60, 60, 75}},
        {{0.10, -0.10, 0.02}, {-35, 20, 0}},
        {{0.10, -0.05, 0.06}, {46, -13, -88}}
    }};
    for (std::size_t i = 0; i < steamVRTrackingPresets.size(); ++i) {
        for (int hand = 0; hand < 2; ++hand) {
            const auto &source = hand == 0 ? officialLeft[i] : officialRight[i];
            const auto &display = steamVRPresetHand(steamVRTrackingPresets[i], hand);
            if (!check(steamVRMatchesPreset(display, source), "SteamVR preset differs from official UI")) return 1;
            const auto bridge = steamVRDisplayToBridge(display);
            if (!check(nearlyEqual(static_cast<float>(bridge.position[2]), -source.position[2]) &&
                       nearlyEqual(static_cast<float>(bridge.rotation[1]), -source.rotation[1]) &&
                       steamVRMatchesPreset(steamVRDisplayToBridge(bridge), display),
                       "SteamVR preset axis conversion changed an existing offset")) return 1;
        }
    }
    if (!check(nearlyEqual(static_cast<float>(steamVRWrappedRotation(220)), -140),
               "UTK 220-degree rotation was clamped instead of wrapped")) return 1;
    std::cout << "Official 30 joints, preview/offset projections, 120 OSC parameters, and SteamVR tracker presets passed\n";
}

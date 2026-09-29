#pragma once

#include <openvr_driver.h>
#include "../src/components/SteamVRBridgeProtocol.h"
#include "HandReferencePose.h"
#include <array>
#include <algorithm>
#include <cmath>

inline constexpr std::array<int, 31> handBoneParents{
    -1, 0, 1, 2, 3, 4, 1, 6, 7, 8, 9, 1, 11, 12, 13, 14,
    1, 16, 17, 18, 19, 1, 21, 22, 23, 24, 0, 0, 0, 0, 0};
inline constexpr std::array<int, 15> measuredHandBones{
    2, 3, 4, 7, 8, 9, 12, 13, 14, 17, 18, 19, 22, 23, 24};

inline vr::HmdQuaternionf_t handMultiply(const vr::HmdQuaternionf_t &a, const vr::HmdQuaternionf_t &b) {
    return {a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
            a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
            a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};
}

inline vr::HmdQuaternionf_t handNormalize(vr::HmdQuaternionf_t q) {
    const float n = q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z;
    if (!std::isfinite(n) || n < 0.000001f) return {1,0,0,0};
    const float s = 1.0f/std::sqrt(n);
    return {q.w*s,q.x*s,q.y*s,q.z*s};
}

inline vr::VRBoneTransform_t handCompose(const vr::VRBoneTransform_t &parent,
                                        const vr::VRBoneTransform_t &child) {
    const auto &q = parent.orientation;
    const vr::HmdQuaternionf_t v{0,child.position.v[0],child.position.v[1],child.position.v[2]};
    const auto r = handMultiply(handMultiply(q,v), {q.w,-q.x,-q.y,-q.z});
    return {{{parent.position.v[0]+r.x,parent.position.v[1]+r.y,parent.position.v[2]+r.z,1}},
             handNormalize(handMultiply(q,child.orientation))};
}

inline std::array<vr::VRBoneTransform_t,31> handModelSpace(
    const std::array<vr::VRBoneTransform_t,31> &local) {
    auto model = local;
    for (std::size_t i=1;i<model.size();++i)
        model[i] = handCompose(model[handBoneParents[i]],local[i]);
    return model;
}

inline vr::HmdQuaternionf_t steamVRJointDelta(const SteamVRBridgeQuaternion &input, int hand, int source) {
    const auto q = handNormalize({input.w,input.x,input.y,input.z});
    // Valve's finger bones extend along local +X (left) / -X (right).
    // A positive LOCAL Z bend closes either hand. Core's measured flexion is
    // left +Z / right -Z; rotating it onto X merely twists the finger in place.
    // All maps below are proper basis rotations, preserving offsets, angle and
    // quaternion composition. The thumb has its own root and hinge bases.
    if (source == 0)
        return hand == 0 ? vr::HmdQuaternionf_t{q.w,-q.z,-q.y,-q.x}
                         : vr::HmdQuaternionf_t{q.w,q.z,q.y,-q.x};
    if (source < 3)
        return hand == 0 ? vr::HmdQuaternionf_t{q.w,q.x,q.z,-q.y}
                         : vr::HmdQuaternionf_t{q.w,q.x,-q.z,q.y};
    // The official middle MCP yaw is the same sign on the two hands.
    if (hand == 1 && source == 6) return {q.w,-q.x,q.y,-q.z};
    return hand == 0 ? q : vr::HmdQuaternionf_t{q.w,q.x,-q.y,-q.z};
}

inline std::array<vr::VRBoneTransform_t,31> makeBones(const SteamVRBridgePacket &packet, int hand) {
    std::array<vr::VRBoneTransform_t,31> bones{};
    std::copy(handReferenceRight.begin(),handReferenceRight.end(),bones.begin());
    if (hand == 0) {
        for (int b=2;b<26;++b) {
            auto &p = bones[b].position;
            const bool meta = b==2 || b==6 || b==11 || b==16 || b==21;
            if (meta) {
                p.v[0] = -p.v[0];
                const auto q = bones[b].orientation;
                // Mirror the wrist's child frame, including its 90-degree
                // FBX basis. Other local rotations stay identical across hands.
                bones[b].orientation = {-q.x,q.w,-q.z,q.y};
            } else {
                for (int axis=0;axis<3;++axis) p.v[axis] = -p.v[axis];
            }
        }
    }
    for (int b=0;b<26;++b) bones[b].orientation = handNormalize(bones[b].orientation);
    for (int source=0;source<15;++source) {
        auto &q = bones[measuredHandBones[source]].orientation;
        q = handNormalize(handMultiply(q,steamVRJointDelta(packet.bones[source],hand,source)));
    }
    // Auxiliary bones are root-local copies of the LAST KNUCKLES, not tips.
    // Receivers can use them for two-bone IK; zero/identity collapses that IK.
    const auto model = handModelSpace(bones);
    constexpr std::array<int,5> distal{4,9,14,19,24};
    for (int finger=0;finger<5;++finger) bones[26+finger] = model[distal[finger]];
    return bones;
}

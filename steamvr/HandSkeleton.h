#pragma once

#include <openvr_driver.h>
#include "../src/components/SteamVRBridgeProtocol.h"
#include "HandReferencePose.h"
#include "HandThumbClosedPose.h"
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

inline float handUnitInput(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}

inline vr::HmdQuaternionf_t handSlerp(vr::HmdQuaternionf_t a,
                                      vr::HmdQuaternionf_t b, float t) {
    a = handNormalize(a);
    b = handNormalize(b);
    float dot = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
    if (dot < 0) {
        b = {-b.w,-b.x,-b.y,-b.z};
        dot = -dot;
    }
    if (dot > 0.9995f) {
        return handNormalize({a.w+(b.w-a.w)*t, a.x+(b.x-a.x)*t,
                              a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t});
    }
    const float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));
    const float inverseSine = 1.0f/std::sin(angle);
    const float x = std::sin((1.0f-t)*angle)*inverseSine;
    const float y = std::sin(t*angle)*inverseSine;
    return handNormalize({x*a.w+y*b.w, x*a.x+y*b.x,
                          x*a.y+y*b.y, x*a.z+y*b.z});
}

inline void mirrorHandReferenceBone(vr::VRBoneTransform_t &bone, int index) {
    auto &p = bone.position;
    const bool meta = index==2 || index==6 || index==11 || index==16 || index==21;
    if (meta) {
        p.v[0] = -p.v[0];
        const auto q = bone.orientation;
        bone.orientation = {-q.x,q.w,-q.z,q.y};
    } else {
        for (int axis=0;axis<3;++axis) p.v[axis] = -p.v[axis];
    }
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
        // Mirror the wrist's child frame, including its 90-degree FBX basis.
        for (int b=2;b<26;++b) mirrorHandReferenceBone(bones[b], b);
    }
    for (int b=0;b<26;++b) bones[b].orientation = handNormalize(bones[b].orientation);
    for (int source=0;source<15;++source) {
        if (packet.hasNativeThumb && source < 3) continue;
        auto &q = bones[measuredHandBones[source]].orientation;
        q = handNormalize(handMultiply(q,steamVRJointDelta(packet.bones[source],hand,source)));
    }
    if (packet.hasNativeThumb) {
        for (int source=0; source<3; ++source) {
            const int boneIndex = measuredHandBones[source];
            auto &bone = bones[boneIndex];
            auto closed = handReferenceRight[boneIndex];
            closed.orientation = handThumbClosedRight[source];
            if (source == 0) {
                for (int axis=0; axis<3; ++axis)
                    closed.position.v[axis] = handThumbClosedRootRight[axis];
            }
            if (hand == 0) mirrorHandReferenceBone(closed, boneIndex);
            const float curl = handUnitInput(packet.thumbFlexion[source]);
            for (int axis=0; axis<3; ++axis)
                bone.position.v[axis] +=
                    (closed.position.v[axis]-bone.position.v[axis])*curl;
            bone.orientation = handSlerp(bone.orientation, closed.orientation, curl);
            if (source == 0) {
                // Official splay is a3 * 0.375/30. The source physical yaw is
                // 80 degrees per normalized unit; local -Z preserves the
                // direction used by the prior SteamVR thumb adapter.
                const float splay = std::isfinite(packet.thumbSplay)
                    ? std::clamp(packet.thumbSplay, -1.0f, 1.0f) : 0.0f;
                const float halfRadians = -splay * (80.0f*3.14159265358979323846f/360.0f);
                bone.orientation = handMultiply(bone.orientation,
                    {std::cos(halfRadians),0,0,std::sin(halfRadians)});
            }
            bone.orientation = handNormalize(handMultiply(bone.orientation,
                steamVRJointDelta(packet.thumbOffsets[source], hand, source)));
        }
    }
    // Auxiliary bones are root-local copies of the LAST KNUCKLES, not tips.
    // Receivers can use them for two-bone IK; zero/identity collapses that IK.
    const auto model = handModelSpace(bones);
    constexpr std::array<int,5> distal{4,9,14,19,24};
    for (int finger=0;finger<5;++finger) bones[26+finger] = model[distal[finger]];
    return bones;
}

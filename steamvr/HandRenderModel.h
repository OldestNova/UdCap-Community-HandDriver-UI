#pragma once

#include <openvr_driver.h>
#include "HandSkeleton.h"
#include <array>
#include <string>

// Palm and fifteen phalanges. Names are also used by generate_hand_resources.py.
inline constexpr std::array<int, 16> handRenderBones{
    1, 2, 3, 4, 7, 8, 9, 12, 13, 14, 17, 18, 19, 22, 23, 24};

inline std::string handRenderPosePath(int bone) {
    return "/pose/udcapc_bone_" + std::to_string(bone);
}

inline vr::HmdMatrix34_t boneMatrix(const vr::VRBoneTransform_t &bone) {
    const auto &q = bone.orientation;
    return {{{1 - 2 * (q.y*q.y + q.z*q.z), 2 * (q.x*q.y - q.w*q.z), 2 * (q.x*q.z + q.w*q.y), bone.position.v[0]},
             {2 * (q.x*q.y + q.w*q.z), 1 - 2 * (q.x*q.x + q.z*q.z), 2 * (q.y*q.z - q.w*q.x), bone.position.v[1]},
             {2 * (q.x*q.z - q.w*q.y), 2 * (q.y*q.z + q.w*q.x), 1 - 2 * (q.x*q.x + q.y*q.y), bone.position.v[2]}}};
}

inline vr::HmdMatrix34_t composeBoneMatrices(const vr::HmdMatrix34_t &parent, const vr::HmdMatrix34_t &local) {
    vr::HmdMatrix34_t result{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col)
            for (int axis = 0; axis < 3; ++axis)
                result.m[row][col] += parent.m[row][axis] * local.m[axis][col];
        result.m[row][3] += parent.m[row][3];
    }
    return result;
}

inline std::array<vr::HmdMatrix34_t, 16> handRenderTransforms(
    const std::array<vr::VRBoneTransform_t, 31> &bones) {
    // Skeletal Input is parent-local; render-model pose components are relative
    // to the device. Accumulate every ancestor, including the wrist/metacarpals.
    std::array<vr::HmdMatrix34_t, 26> global{};
    global[0] = boneMatrix(bones[0]);
    for (std::size_t i = 1; i < global.size(); ++i)
        global[i] = composeBoneMatrices(global[handBoneParents[i]], boneMatrix(bones[i]));
    std::array<vr::HmdMatrix34_t, 16> result{};
    for (std::size_t i = 0; i < result.size(); ++i) result[i] = global[handRenderBones[i]];
    return result;
}

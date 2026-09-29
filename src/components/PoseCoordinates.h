#pragma once

#include <UdCapV1Core.h>
#include <cmath>
#include <cstddef>

// Core already emits the official VMC local Unity joint rotations. Receiver
// retargeting is not a reason to invert selected components a second time.
inline HandQuaternion poseForVMC(HandQuaternion pose) {
    return pose;
}

// SteamVR receives the unchanged Core pose over the bridge. Its complete
// 31-bone reference frames and joint-specific adapter live in HandSkeleton.h.

// The 3D preview uses a forward-facing, mirrored pair of simple hands. Its
// local axes differ from the Unity/VMC bones. Each map below is an orthogonal
// change of the quaternion vector basis, with determinant +1: no joint axis
// is dropped, and composition and unit length are preserved.
inline BoneQuaternion poseForPreview(const BoneQuaternion &bone, UdTarget hand,
                                     std::size_t finger, std::size_t joint) {
    const float length2 = bone.x * bone.x + bone.y * bone.y +
                          bone.z * bone.z + bone.w * bone.w;
    if (!std::isfinite(length2) || length2 < 0.000001f) {
        return {0, 0, 0, 1};
    }
    const float scale = 1.0f / std::sqrt(length2);
    const BoneQuaternion source{bone.x * scale, bone.y * scale,
                                bone.z * scale, bone.w * scale};
    const float side = hand == UD_TARGET_LEFT_HAND ? -1.0f : 1.0f;
    if (finger == 0) {
        if (joint == 0) {
            // Core X drives opposition; the thumb rests and their visible
            // sweeps must mirror, while depth curl stays the same.
            return {side * source.z, source.y, -side * source.x, source.w};
        }
        // Core Y bends the two distal thumb joints. Tilt their bend plane
        // toward the palm; preserve X/Z offsets and the actual bend angle.
        constexpr float tilt = 0.6f;
        return {-side * std::cos(tilt) * source.y - std::sin(tilt) * source.z,
                source.x,
                -std::sin(tilt) * source.y + side * std::cos(tilt) * source.z,
                source.w};
    }
    // Fingers extend along +Y here. Official negative flexion must bend
    // into -Z on BOTH hands. The official middle MCP yaw is not hand-signed.
    if (joint == 0 && finger == 2 && hand == UD_TARGET_RIGHT_HAND) {
        return {source.z, source.x, source.y, source.w};
    }
    return {side * source.z, -side * source.x, -source.y, source.w};
}

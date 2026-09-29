#pragma once

#include "SteamVRBridgeProtocol.h"
#include <UdCapV1Core.h>
#include <array>
#include <cmath>

// Mirrors CalibrationDataModel's SteamVR InputData thumb channels. The
// already-processed CMD_ANGLE values include the selected response curve.
inline void setSteamVRThumbInput(SteamVRBridgePacket &packet,
                                 const std::array<double, 28> &angles,
                                 const HandRotation &offset) {
    if (!std::isfinite(angles[0]) || !std::isfinite(angles[1]) ||
        !std::isfinite(angles[2]) || !std::isfinite(angles[3])) {
        packet.hasNativeThumb = 0;
        return;
    }
    packet.thumbFlexion[0] = static_cast<float>(std::abs(angles[2]) / 60.0);
    packet.thumbFlexion[1] = static_cast<float>((std::abs(angles[1]) + 15.0) / 65.0);
    packet.thumbFlexion[2] = static_cast<float>((std::abs(angles[0]) + 15.0) / 65.0);
    packet.thumbSplay = static_cast<float>(angles[3] * 0.375 / 30.0);

    const std::array<BoneRotation, 3> joints{
        offset.thumbFinger.proximal,
        offset.thumbFinger.intermediate,
        offset.thumbFinger.distal};
    for (std::size_t i = 0; i < joints.size(); ++i) {
        const auto &o = joints[i];
        const auto q = UdCapV1Core::eulerToQuaternion(o.x, o.y, o.z);
        packet.thumbOffsets[i] = {q.x, q.y, q.z, q.w};
    }
    packet.hasNativeThumb = 1;
}

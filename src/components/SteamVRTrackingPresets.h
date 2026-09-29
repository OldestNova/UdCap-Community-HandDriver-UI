#ifndef UDCAP_STEAMVR_TRACKING_PRESETS_H
#define UDCAP_STEAMVR_TRACKING_PRESETS_H

#include <array>
#include <cmath>

// Values shown by the official UdcapDriver 0.1.8.6 VRSettingWindow. The
// official UI displays the numbers below, then negates position Z and rotation
// Y when it sends pose_settings to its OpenVR driver.
struct SteamVRTrackingOffset {
    std::array<double, 3> position{}; // metres
    std::array<double, 3> rotation{}; // degrees
};

struct SteamVRTrackingPreset {
    const char *id;
    SteamVRTrackingOffset right;
    SteamVRTrackingOffset left;
};

inline constexpr std::array<SteamVRTrackingPreset, 4> steamVRTrackingPresets{{
    {"vive", {{-0.10, 0.10, -0.05}, {45, -85, 0}},
             {{0.10, 0.10, -0.05}, {45, 85, 0}}},
    {"tundra", {{-0.09, -0.09, -0.04}, {60, -60, 105}},
               {{-0.09, 0.09, -0.04}, {60, 60, 75}}},
    {"quest", {{-0.10, -0.10, 0.02}, {-35, -20, 0}},
              {{0.10, -0.10, 0.02}, {-35, 20, 0}}},
    {"utk", {{0.09, -0.07, -0.07}, {220, -11, 80}},
            {{0.10, -0.05, 0.06}, {46, -13, -88}}}
}};

inline const SteamVRTrackingOffset &steamVRPresetHand(const SteamVRTrackingPreset &preset, int hand) {
    return hand == 0 ? preset.left : preset.right;
}

// Involution: use in both directions when reading/writing existing config.
// Existing config contains OpenVR local-axis values; only the editor display
// uses the official UI's axis convention, so old tracking poses are retained.
inline SteamVRTrackingOffset steamVRDisplayToBridge(SteamVRTrackingOffset display) {
    display.position[2] = -display.position[2];
    display.rotation[1] = -display.rotation[1];
    return display;
}

inline bool steamVRMatchesPreset(const SteamVRTrackingOffset &actual,
                                 const SteamVRTrackingOffset &preset) {
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(actual.position[axis] - preset.position[axis]) > 0.0005 ||
            std::abs(actual.rotation[axis] - preset.rotation[axis]) > 0.05) return false;
    }
    return true;
}

inline double steamVRWrappedRotation(float degrees) {
    return std::isfinite(degrees) ? std::remainder(static_cast<double>(degrees), 360.0) : 0.0;
}

#endif

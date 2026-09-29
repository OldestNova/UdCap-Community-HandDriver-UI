#ifndef UDCAP_STEAMVR_BRIDGE_PROTOCOL_H
#define UDCAP_STEAMVR_BRIDGE_PROTOCOL_H

#include <cstdint>
#include <type_traits>

// Localhost datagram shared by the UI process and the SteamVR driver process.
// Both processes are built for the same host architecture.
constexpr std::uint32_t steamVrBridgeMagic = 0x55444350; // UDCP
constexpr std::uint16_t steamVrBridgeVersion = 2;
constexpr std::uint16_t steamVrBridgePort = 8998;
constexpr std::uint16_t steamVrTrackerSerialBytes = 128;

struct SteamVRBridgeQuaternion {
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 1;
};

struct SteamVRBridgePacket {
    std::uint32_t magic = steamVrBridgeMagic;
    std::uint16_t version = steamVrBridgeVersion;
    std::uint8_t hand = 0; // 0=left, 1=right
    std::uint8_t connected = 0;
    SteamVRBridgeQuaternion bones[15]{};
    float joystickX = 0;
    float joystickY = 0;
    float trigger = 0;
    float grip = 0;
    float trackpad = 0;
    std::uint8_t buttonA = 0;
    std::uint8_t buttonB = 0;
    std::uint8_t buttonMenu = 0;
    std::uint8_t buttonJoystick = 0;
    std::uint8_t buttonTrigger = 0;
    std::uint8_t buttonGrip = 0;
    std::uint8_t buttonTrackpad = 0;
    // Resolved from SteamVR's Tracker roles by the OpenVR client; never selected in this UI.
    char trackerSerial[steamVrTrackerSerialBytes]{};
    float trackerOffsetMeters[3]{};
    float trackerRotationDegrees[3]{}; // X=pitch, Y=yaw, Z=roll, in tracker-local axes.
};

static_assert(std::is_trivially_copyable_v<SteamVRBridgePacket>);
static_assert(std::is_standard_layout_v<SteamVRBridgePacket>);
static_assert(sizeof(SteamVRBridgePacket) == 428, "SteamVR bridge packet ABI changed");

#endif

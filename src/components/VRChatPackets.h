#pragma once

#include <UdCapV1Core.h>
#include <oscpp/client.hpp>
#include <array>
#include <cmath>
#include <string>

// Official avatars encode the nearest reciprocal (1/n), not a linear
// four-bit value. Keep this contract with the matching avatar animator.
inline unsigned vrchatFingerBits(float value) {
    unsigned bits = 0;
    float distance = 2.0f;
    for (unsigned n = 1; n <= 15; ++n) {
        const float candidate = std::fabs(value - 1.0f / static_cast<float>(n));
        if (candidate < distance) {
            bits = n;
            distance = candidate;
            if (distance == 0) break;
        }
    }
    return bits;
}

template<class Send>
void forEachVRChatPacket(const std::array<double, 28> &a, UdTarget hand, Send send) {
    if (hand != UD_TARGET_LEFT_HAND && hand != UD_TARGET_RIGHT_HAND) return;
    const bool left = hand == UD_TARGET_LEFT_HAND;
    const std::string prefix = left ? "/avatar/parameters/Left" : "/avatar/parameters/Right";
    struct Parameter { const char *name; float value; };
    // Five packets stay below the usual Ethernet MTU. Four Boolean messages
    // per parameter, bit suffixes 1/2/4/8, 60 messages per hand in total.
    const std::array<std::array<Parameter, 4>, 5> fingers{{
        {{{"Thumb1", float((a[2] + 80) / 80 - (left ? .5 : .82))},
          {"Thumb2", float((a[1] + 30) / 40)}, {"Thumb3", float((a[0] + 80) / 65)},
          {"Thumb1spread", float((a[20] + 45) / 60)}}},
        {{{"Index1", float((a[6] + 80) / 90)}, {"Index2", float((a[5] + 87) / 87)},
          {"Index1spread", float(a[7] / 20)}, {nullptr, 0}}},
        {{{"Middle1", float((a[10] + 80) / 80)}, {"Middle2", float((a[9] + 87) / 87)},
          {nullptr, 0}, {nullptr, 0}}},
        {{{"Ring1", float((a[14] + 87) / 87)}, {"Ring2", float((a[13] + 87) / 87)},
          {"Ring1spread", float(a[15] / 12)}, {nullptr, 0}}},
        {{{"Pinky1", float((a[18] + 75) / 75)}, {"Pinky2", float((a[17] + 90) / 90)},
          {"Pinky1spread", float(a[19] / 40)}, {nullptr, 0}}}
    }};
    for (const auto &finger : fingers) {
        alignas(8) char buffer[1400]{};
        OSCPP::Client::Packet packet(buffer, sizeof(buffer));
        packet.openBundle(1); // Immediate OSC timetag, not Unix milliseconds.
        for (const auto &parameter : finger) {
            if (!parameter.name) continue;
            const unsigned bits = vrchatFingerBits(parameter.value);
            for (unsigned bit : {1u, 2u, 4u, 8u}) {
                const auto address = prefix + parameter.name + std::to_string(bit);
                packet.openMessage(address.c_str(), 1).boolean((bits & bit) != 0).closeMessage();
            }
        }
        packet.closeBundle();
        send(packet.data(), packet.size());
    }
}

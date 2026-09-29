#include "OptiTrackSender.h"

#include <OptiTrackWire.h>
#include <nng/protocol/pubsub0/pub.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

OptiTrackSender::OptiTrackSender(const char *endpoint) {
    const int opened = nng_pub0_open(&socket);
    if (opened != 0) throw std::runtime_error(nng_strerror(opened));
    const int listening = nng_listen(socket, endpoint, nullptr, 0);
    if (listening != 0) {
        nng_close(socket);
        throw std::runtime_error(nng_strerror(listening));
    }
}

OptiTrackSender::~OptiTrackSender() {
    for (auto &[core, listener] : listeners)
        if (listener.unlisten) listener.unlisten();
    nng_close(socket);
}

void OptiTrackSender::add(const std::shared_ptr<UdCapV1Core> &core,
                          const std::string &receiverSerial) {
    if (!core || listeners.contains(core.get())) return;
    const auto weak = std::weak_ptr<UdCapV1Core>(core);
    auto unlisten = core->listen([this, weak, receiverSerial]
            (const std::shared_ptr<UdCapV1MCUPacket> &packet) {
        if (packet->commandType != CMD_SKELETON_QUATERNION) return;
        const auto current = weak.lock();
        if (!current) return;
        // The paired glove can differ from the receiver's discovery name.
        // Resolve its side after pairing, when Core has the real glove serial.
        const auto side = current->getTarget();
        if (side != UD_TARGET_LEFT_HAND && side != UD_TARGET_RIGHT_HAND) return;
        udcap::optitrack::Frame frame{};
        frame.hand = static_cast<std::uint8_t>(side);
        const auto gloveSerial = current->getUDCapSerial();
        const auto &serial = gloveSerial.empty() ? receiverSerial : gloveSerial;
        std::memcpy(frame.serial, serial.data(), std::min(serial.size(), sizeof(frame.serial) - 1));
        if (const auto battery = current->getBattery()) {
            frame.batteryLevel = static_cast<std::uint8_t>(battery->first);
            frame.batteryRaw = battery->second;
        }
        if (const auto rssi = current->getRssiDbm()) frame.rssiDbm = *rssi;
        else frame.rssiDbm = INT16_MIN;
        const auto &pose = packet->skeletonQuaternion;
        const std::array<FingerQuaternion, 5> fingers{
            pose.thumbFinger, pose.indexFinger, pose.middleFinger,
            pose.ringFinger, pose.littleFinger};
        for (std::size_t finger = 0; finger < fingers.size(); ++finger) {
            const std::array<BoneQuaternion, 3> joints{
                fingers[finger].proximal, fingers[finger].intermediate,
                fingers[finger].distal};
            for (std::size_t joint = 0; joint < joints.size(); ++joint) {
                const auto &q = joints[joint];
                if (!std::isfinite(q.x) || !std::isfinite(q.y) ||
                    !std::isfinite(q.z) || !std::isfinite(q.w)) return;
                frame.joints[finger * 3 + joint] = {q.x, q.y, q.z, q.w};
            }
        }
        frame.sequence = ++sequence;
        // Pub/sub sends fresh full poses; a slow Motive instance may skip old
        // poses without ever blocking the glove's Core event thread.
        nng_send(socket, &frame, sizeof(frame), NNG_FLAG_NONBLOCK);
    });
    listeners.emplace(core.get(), Listener{core, std::move(unlisten)});
}

void OptiTrackSender::retainOnly(const std::set<UdCapV1Core *> &active) {
    for (auto it = listeners.begin(); it != listeners.end();) {
        if (active.contains(it->first)) {
            ++it;
            continue;
        }
        if (it->second.unlisten) it->second.unlisten();
        it = listeners.erase(it);
    }
}

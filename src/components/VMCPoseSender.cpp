//
// Created by max_3 on 2025/6/10.
//

#include "VMCPoseSender.h"
#include <chrono>
#include <oscpp/client.hpp>

VMCPoseSender::VMCPoseSender(std::string _host, uint16_t _port, std::shared_ptr<UdCapV1Core> _core): host(_host), port(_port), core(_core), prefix(), io_context(), socket(io_context) {
    UdTarget target = core->getTarget();
    if (target == UD_TARGET_LEFT_HAND) {
        prefix = "Left";
    } else if (target == UD_TARGET_RIGHT_HAND) {
        prefix = "Right";
    }
    socket.open(boost::asio::ip::udp::v4());
    boost::asio::ip::udp::resolver resolver(io_context);
    auto addr = boost::asio::ip::make_address(host);
    endpoints = resolver.resolve(boost::asio::ip::udp::endpoint{addr, port});
    unlisten = core->listen([this](const UdCapV1MCUPacket &data) {
        if (data.commandType == CMD_SKELETON_QUATERNION) {
            char buffer[128 * 15] = {0};
            OSCPP::Client::Packet packet(buffer, 128 * 15);
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            HandQuaternion q = data.skeletonQuaternion;
            packet.openBundle(timestamp)
                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "ThumbDistal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.thumbFinger.distal.x).float32(q.thumbFinger.distal.y).float32(q.thumbFinger.distal.z).float32(q.thumbFinger.distal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "ThumbIntermediate").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.thumbFinger.intermediate.x).float32(q.thumbFinger.intermediate.y).float32(q.thumbFinger.intermediate.z).float32(q.thumbFinger.intermediate.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "ThumbProximal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.thumbFinger.proximal.x).float32(q.thumbFinger.proximal.y).float32(q.thumbFinger.proximal.z).float32(q.thumbFinger.proximal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "IndexDistal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.indexFinger.distal.x).float32(q.indexFinger.distal.y).float32(q.indexFinger.distal.z).float32(q.indexFinger.distal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "IndexIntermediate").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.indexFinger.intermediate.x).float32(q.indexFinger.intermediate.y).float32(q.indexFinger.intermediate.z).float32(q.indexFinger.intermediate.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "IndexProximal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.indexFinger.proximal.x).float32(q.indexFinger.proximal.y).float32(q.indexFinger.proximal.z).float32(q.indexFinger.proximal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "MiddleDistal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.middleFinger.distal.x).float32(q.middleFinger.distal.y).float32(q.middleFinger.distal.z).float32(q.middleFinger.distal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "MiddleIntermediate").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.middleFinger.intermediate.x).float32(q.middleFinger.intermediate.y).float32(q.middleFinger.intermediate.z).float32(q.middleFinger.intermediate.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "MiddleProximal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.middleFinger.proximal.x).float32(q.middleFinger.proximal.y).float32(q.middleFinger.proximal.z).float32(q.middleFinger.proximal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "RingDistal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.ringFinger.distal.x).float32(q.ringFinger.distal.y).float32(q.ringFinger.distal.z).float32(q.ringFinger.distal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "RingIntermediate").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.ringFinger.intermediate.x).float32(q.ringFinger.intermediate.y).float32(q.ringFinger.intermediate.z).float32(q.ringFinger.intermediate.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "RingProximal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.ringFinger.proximal.x).float32(q.ringFinger.proximal.y).float32(q.ringFinger.proximal.z).float32(q.ringFinger.proximal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "LittleDistal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.littleFinger.distal.x).float32(q.littleFinger.distal.y).float32(q.littleFinger.distal.z).float32(q.littleFinger.distal.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "LittleIntermediate").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.littleFinger.intermediate.x).float32(q.littleFinger.intermediate.y).float32(q.littleFinger.intermediate.z).float32(q.littleFinger.intermediate.w)
                    .closeMessage()

                    .openMessage("/VMC/Ext/Bone/Pos", 8)
                    .string((prefix + "LittleProximal").c_str())
                    .float32(0).float32(0).float32(0)
                    .float32(q.littleFinger.proximal.x).float32(q.littleFinger.proximal.y).float32(q.littleFinger.proximal.z).float32(q.littleFinger.proximal.w)
                    .closeMessage()
                .closeBundle();
            try {
                socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
            } catch (std::exception& e) {

            };
        }
    });
}

VMCPoseSender::~VMCPoseSender() {
    unlisten();
    try {
        socket.cancel();
        socket.close();
    } catch (std::exception& e) {
        // Handle exception if needed
    }
}
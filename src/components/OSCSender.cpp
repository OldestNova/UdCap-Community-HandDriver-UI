//
// Created by max_3 on 2025/6/10.
//

#include "OSCSender.h"
#include <oscpp/client.hpp>

OSCSender::OSCSender(std::string _host, uint16_t _port, std::shared_ptr<UdCapV1Core> _core): host(_host), port(_port), core(_core), prefix(), io_context(), socket(io_context) {
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

        }
    });
}

OSCSender::~OSCSender() {
    unlisten();
    try {
        socket.cancel();
        socket.close();
    } catch (std::exception& e) {
        // Handle exception if needed
    }
}
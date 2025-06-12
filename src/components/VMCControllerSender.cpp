//
// Created by max_3 on 2025/6/13.
//

#include "VMCControllerSender.h"
#include <chrono>
#include <oscpp/client.hpp>
VMCControllerSender::VMCControllerSender(std::string _host, uint16_t _port, std::shared_ptr<UdCapV1Core> _coreLeft, std::shared_ptr<UdCapV1Core> _coreRight): host(_host), port(_port), coreLeft(_coreLeft), coreRight(_coreRight), io_context(), socket(io_context) {
    socket.open(boost::asio::ip::udp::v4());
    boost::asio::ip::udp::resolver resolver(io_context);
    auto addr = boost::asio::ip::make_address(host);
    endpoints = resolver.resolve(boost::asio::ip::udp::endpoint{addr, port});
    unlisten.push_back(coreLeft->listen([this](const UdCapV1MCUPacket &data) {
        if (data.commandType == CMD_INPUT_JOYSTICK) {
            if (data.joystickData.joyX > 0) {
                leftJoyXP = data.joystickData.joyX;
                leftJoyXN = 0.0f;
            } else {
                leftJoyXP = 0.0f;
                leftJoyXN = -data.joystickData.joyX;
            }
            if (data.joystickData.joyY > 0) {
                leftJoyYP = data.joystickData.joyY;
                leftJoyYN = 0.0f;
            } else {
                leftJoyYP = 0.0f;
                leftJoyYN = -data.joystickData.joyY;
            }
            updateController();
        } else if (data.commandType == CMD_INPUT_BUTTON) {
            leftButtonA = data.button.btnA ? 1.0f : 0.0f;
            leftButtonB = data.button.btnB ? 1.0f : 0.0f;
            leftButtonJoy = data.button.btnJoyStick ? 1.0f : 0.0f;
            leftButtonMenu = data.button.btnMenu ? 1.0f : 0.0f;
            updateController();
        }
    }));
    unlisten.push_back(coreRight->listen([this](const UdCapV1MCUPacket &data) {
        if (data.commandType == CMD_INPUT_JOYSTICK) {
            if (data.joystickData.joyX > 0) {
                rightJoyXP = data.joystickData.joyX;
                rightJoyXN = 0.0f;
            } else {
                rightJoyXP = 0.0f;
                rightJoyXN = -data.joystickData.joyX;
            }
            if (data.joystickData.joyY > 0) {
                rightJoyYP = data.joystickData.joyY;
                rightJoyYN = 0.0f;
            } else {
                rightJoyYP = 0.0f;
                rightJoyYN = -data.joystickData.joyY;
            }
            updateController();
        } else if (data.commandType == CMD_INPUT_BUTTON) {
            rightButtonA = data.button.btnA ? 1.0f : 0.0f;
            rightButtonB = data.button.btnB ? 1.0f : 0.0f;
            rightButtonJoy = data.button.btnJoyStick ? 1.0f : 0.0f;
            rightButtonMenu = data.button.btnMenu ? 1.0f : 0.0f;
            updateController();
        }
    }));
}

VMCControllerSender::~VMCControllerSender() {
    for (auto &func: unlisten) {
        func();
    }
    unlisten.clear();
    try {
        socket.cancel();
        socket.close();
    } catch (std::exception& e) {
        // Handle exception if needed
    }
}

void VMCControllerSender::updateController() {
    char buffer[128 * 16] = {0};
    OSCPP::Client::Packet packet(buffer, 128 * 15);
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    packet.openBundle(timestamp)
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftJoyX_Positive").float32(leftJoyXP).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftJoyX_Negative").float32(leftJoyXN).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftJoyY_Positive").float32(leftJoyYP).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftJoyY_Negative").float32(leftJoyYN).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_A").float32(leftButtonA).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_B").float32(leftButtonB).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_Joy").float32(leftButtonJoy).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_Menu").float32(leftButtonMenu).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyX_Positive").float32(rightJoyXP).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyX_Negative").float32(rightJoyXN).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyY_Positive").float32(rightJoyYP).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyY_Negative").float32(rightJoyYN).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_A").float32(rightButtonA).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_B").float32(rightButtonB).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Joy").float32(rightButtonJoy).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Menu").float32(rightButtonMenu).closeMessage()
            .openMessage("/VMC/Ext/Blend/Apply", 0)
        .closeBundle();
    try {
        socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
    } catch (std::exception& e) {

    };
}
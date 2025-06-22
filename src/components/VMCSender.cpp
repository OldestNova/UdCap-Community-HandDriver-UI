//
// Created by max_3 on 2025/6/10.
//

#include "VMCSender.h"
#include <chrono>
#include <oscpp/client.hpp>
#include <iostream>

VMCSender::VMCSender(std::string _host, uint16_t _port): host(_host), port(_port), io_context(), socket(io_context), packetQueue() {
    socket.open(boost::asio::ip::udp::v4());
    boost::asio::ip::udp::resolver resolver(io_context);
    auto addr = boost::asio::ip::make_address(host);
    endpoints = resolver.resolve(boost::asio::ip::udp::endpoint{addr, port});
    sendThread = std::thread([this]() {
        while (running) {
            if (packetQueue.empty()) {
                std::unique_lock lk(mtx);
                cv.wait_for(lk, std::chrono::milliseconds(10));
                continue;
            }
            try {
                std::lock_guard lk(queueMutex);
                UdCapV1VMCPacket p = packetQueue.front();
                packetQueue.pop();
                UdTarget target = p.target;
                std::shared_ptr<UdCapV1MCUPacket> data = p.data;
                std::string prefix;
                if (target == UD_TARGET_LEFT_HAND) {
                    prefix = "Left";
                } else {
                    prefix = "Right";
                }
                if (data->commandType == CMD_SKELETON_QUATERNION) {
                    char buffer[128 * 15] = {0};
                    OSCPP::Client::Packet packet(buffer, 128 * 15);
                    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                    HandQuaternion q = data->skeletonQuaternion;
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

                    }
                } else if (data->commandType == CMD_INPUT_JOYSTICK) {
                    if (target == UD_TARGET_LEFT_HAND) {
                        if (data->joystickData.joyX > 0) {
                            leftJoyXP = data->joystickData.joyX;
                            leftJoyXN = 0.0f;
                        } else {
                            leftJoyXP = 0.0f;
                            leftJoyXN = -data->joystickData.joyX;
                        }
                        if (data->joystickData.joyY > 0) {
                            leftJoyYP = data->joystickData.joyY;
                            leftJoyYN = 0.0f;
                        } else {
                            leftJoyYP = 0.0f;
                            leftJoyYN = -data->joystickData.joyY;
                        }
                    } else {
                        if (data->joystickData.joyX > 0) {
                            rightJoyXP = data->joystickData.joyX;
                            rightJoyXN = 0.0f;
                        } else {
                            rightJoyXP = 0.0f;
                            rightJoyXN = -data->joystickData.joyX;
                        }
                        if (data->joystickData.joyY > 0) {
                            rightJoyYP = data->joystickData.joyY;
                            rightJoyYN = 0.0f;
                        } else {
                            rightJoyYP = 0.0f;
                            rightJoyYN = -data->joystickData.joyY;
                        }
                    }
//            updateController();  // Not need, CMD_INPUT_BUTTON will fire later.
                } else if (data->commandType == CMD_INPUT_BUTTON) {
                    if (target == UD_TARGET_LEFT_HAND) {
                        leftButtonA = data->button.btnA ? 1.0f : 0.0f;
                        leftButtonB = data->button.btnB ? 1.0f : 0.0f;
                        leftButtonJoy = data->button.btnJoyStick ? 1.0f : 0.0f;
                        leftButtonMenu = data->button.btnMenu ? 1.0f : 0.0f;
                        leftButtonTrigger = data->button.btnTrigger ? 1.0f: 0.0f;
                        leftButtonGrip = data->button.btnGrip ? 1.0f: 0.0f;
                        leftButtonTrackpad = data->button.btnTrackpad ? 1.0f: 0.0f;
                        leftTrigger = data->button.trigger;
                        leftGrip = data->button.grip;
                        leftTrackpad = data->button.trackpad;
                        leftButtonPower = data->button.btnPower ? 1.0f : 0.0f;
                    } else {
                        rightButtonA = data->button.btnA ? 1.0f : 0.0f;
                        rightButtonB = data->button.btnB ? 1.0f : 0.0f;
                        rightButtonJoy = data->button.btnJoyStick ? 1.0f : 0.0f;
                        rightButtonMenu = data->button.btnMenu ? 1.0f : 0.0f;
                        rightButtonTrigger = data->button.btnTrigger ? 1.0f: 0.0f;
                        rightButtonGrip = data->button.btnGrip ? 1.0f: 0.0f;
                        rightButtonTrackpad = data->button.btnTrackpad ? 1.0f: 0.0f;
                        rightTrigger = data->button.trigger;
                        rightGrip = data->button.grip;
                        rightTrackpad = data->button.trackpad;
                        rightButtonPower = data->button.btnPower ? 1.0f : 0.0f;
                    }
                    updateController();
                }
            } catch (std::exception &e) {
//                std::cout << "Error sending packet: " << e.what() << std::endl;
            }
        }
    });
}

VMCSender::~VMCSender() {
    running = false;
    if (sendThread.joinable()) {
        sendThread.join();
    }
    if (unlistenLeft) unlistenLeft();
    if (unlistenRight) unlistenRight();
    try {
        socket.cancel();
        socket.close();
    } catch (std::exception& e) {
        // Handle exception if needed
    }
}

void VMCSender::remove(bool left, bool right) {
    if (left && coreLeft) {
        unlistenLeft();
        unlistenLeft = nullptr;
        coreLeft.reset();
    }
    if (right && coreRight) {
        unlistenRight();
        unlistenRight = nullptr;
        coreRight.reset();
    }
}

void VMCSender::add(std::shared_ptr<UdCapV1Core> _core) {
    if (!_core) {
        throw std::invalid_argument("Core cannot be null");
    }
    UdTarget target = _core->getTarget();
    if (target == UD_TARGET_LEFT_HAND && coreLeft) {
        throw std::runtime_error("Left hand already exists");
    }
    if (target == UD_TARGET_RIGHT_HAND && coreRight) {
        throw std::runtime_error("Right hand already exists");
    }
    if (target == UD_TARGET_LEFT_HAND) {
        coreLeft = _core;
    }
    if (target == UD_TARGET_RIGHT_HAND) {
        coreRight = _core;
    }
    std::function<void()> unlisten = _core->listen([this, target](std::shared_ptr<UdCapV1MCUPacket> data) {
        std::lock_guard lk(queueMutex);
        UdCapV1VMCPacket p;
        p.target = target;
        p.data = data;
        packetQueue.push(p);
        cv.notify_all();
    });
    if (target == UD_TARGET_LEFT_HAND) {
        unlistenLeft = unlisten;
    }
    if (target == UD_TARGET_RIGHT_HAND) {
        unlistenRight = unlisten;
    }
}

void VMCSender::updateController() {
    char buffer[4096] = {0};
    OSCPP::Client::Packet packet(buffer, 4096);
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
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_Trackpad").float32(leftButtonTrackpad).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_Grip").float32(leftButtonGrip).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_Trigger").float32(leftButtonTrigger).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftButton_Power").float32(leftButtonPower).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftValue_Trackpad").float32(leftTrackpad).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftValue_Grip").float32(leftGrip).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("LeftValue_Trigger").float32(leftTrigger).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyX_Positive").float32(rightJoyXP).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyX_Negative").float32(rightJoyXN).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyY_Positive").float32(rightJoyYP).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightJoyY_Negative").float32(rightJoyYN).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_A").float32(rightButtonA).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_B").float32(rightButtonB).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Joy").float32(rightButtonJoy).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Menu").float32(rightButtonMenu).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Trackpad").float32(rightButtonTrackpad).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Grip").float32(rightButtonGrip).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Trigger").float32(rightButtonTrigger).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightButton_Power").float32(rightButtonPower).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightValue_Trackpad").float32(rightTrackpad).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightValue_Grip").float32(rightGrip).closeMessage()
            .openMessage("/VMC/Ext/Blend/Val", 2).string("RightValue_Trigger").float32(rightTrigger).closeMessage()
            .openMessage("/VMC/Ext/Blend/Apply", 0)
            .closeBundle();
    try {
        socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
    } catch (std::exception& e) {

    };
}
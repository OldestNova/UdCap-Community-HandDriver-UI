//
// Created by max_3 on 2025/6/10.
//

#include "OSCSender.h"
#include <oscpp/client.hpp>
#include <iostream>

std::array<bool, 4> getBoolParamaters(float value) {
    int num1 = 0;
    float num2 = 2.0f;
    for (int index = 1; index <= 15; ++index)
    {
        float num3 = std::fabs(value - 1.0f / (float) index);
        if (num3 < num2)
        {
            num1 = index;
            num2 = num3;
            if (num2 == 0.0f)
                break;
        }
    }
    int n = static_cast<int>(num1);
    std::array<bool, 4> boolParams{};
    boolParams[0] = (num1 & 0x08) != 0;  // 检查第4位 (1000)
    boolParams[1] = (num1 & 0x04) != 0;  // 检查第3位 (0100)
    boolParams[2] = (num1 & 0x02) != 0;  // 检查第2位 (0010)
    boolParams[3] = (num1 & 0x01) != 0;  // 检查第1位 (0001)
    return boolParams;
}

OSCSender::OSCSender(std::string _host, uint16_t _port): host(_host), port(_port), io_context(), socket(io_context), packetQueue() {
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
                UdCapV1OSCPacket p = packetQueue.front();
                packetQueue.pop();
                UdTarget target = p.target;
                std::shared_ptr<UdCapV1MCUPacket> data = p.data;
                std::string prefix;
                if (target == UD_TARGET_LEFT_HAND) {
                    prefix = "Left";
                } else {
                    prefix = "Right";
                }
                if (data->commandType == CMD_ANGLE) {
                    char buffer[128 * 15] = {0};
                    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                    std::array<double, 28> array = data->result;
                    if (array.empty()) return;
                    float single1 = static_cast<float>(array[0]);
                    float single2 = static_cast<float>(array[1]);
                    float single3 = static_cast<float>(array[2]);
                    float single4 = static_cast<float>(array[3]);
                    float single5 = static_cast<float>(array[4]);
                    float single6 = static_cast<float>(array[5]);
                    float single7 = static_cast<float>(array[6]);
                    float single8 = static_cast<float>(array[7]);
                    float single9 = static_cast<float>(array[8]);
                    float single10 = static_cast<float>(array[9]);
                    float single11 = static_cast<float>(array[10]);
                    float single12 = static_cast<float>(array[11]);
                    float single13 = static_cast<float>(array[12]);
                    float single14 = static_cast<float>(array[13]);
                    float single15 = static_cast<float>(array[14]);
                    float single16 = static_cast<float>(array[15]);
                    float single17 = static_cast<float>(array[16]);
                    float single18 = static_cast<float>(array[17]);
                    float single19 = static_cast<float>(array[18]);
                    float single20 = static_cast<float>(array[19]);
                    float single21 = static_cast<float>(array[20]);
                    float single22 = static_cast<float>(array[21]);
                    float single23 = static_cast<float>(array[22]);
                    using namespace std::string_literals;
                    {
                        OSCPP::Client::Packet packet(buffer, 64 * 64);
                        // Thumb
                        std::array<bool, 4> boolParams1 = getBoolParamaters((single3 + 80.0) / 80.0 - 0.5);
                        std::array<bool, 4> boolParams2 = getBoolParamaters((single2 + 30.0) / 40.0);
                        std::array<bool, 4> boolParams3 = getBoolParamaters((single1 + 80.0) / 65.0);
                        std::array<bool, 4> boolParams4 = getBoolParamaters((single21 + 45.0) / 45.0);
                        packet.openBundle(timestamp);
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb11").c_str(), 1).boolean(boolParams1[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb12").c_str(), 1).boolean(boolParams1[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb14").c_str(), 1).boolean(boolParams1[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb18").c_str(), 1).boolean(boolParams1[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb21").c_str(), 1).boolean(boolParams2[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb22").c_str(), 1).boolean(boolParams2[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb24").c_str(), 1).boolean(boolParams2[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb28").c_str(), 1).boolean(boolParams2[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb31").c_str(), 1).boolean(boolParams3[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb32").c_str(), 1).boolean(boolParams3[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb34").c_str(), 1).boolean(boolParams3[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb38").c_str(), 1).boolean(boolParams3[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb1spread1").c_str(), 1).boolean(boolParams4[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb1spread2").c_str(), 1).boolean(boolParams4[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb1spread4").c_str(), 1).boolean(boolParams4[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Thumb1spread8").c_str(), 1).boolean(boolParams4[0]).closeMessage();
                        packet.closeBundle();
                        try {
                            socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
                        } catch (std::exception& e) {

                        }
                    }
                    {
                        OSCPP::Client::Packet packet(buffer, 64 * 64);
                        // Index
                        std::array<bool, 4> boolParams5 = getBoolParamaters((single7 + 80.0) / 90.0);
                        std::array<bool, 4> boolParams6 = getBoolParamaters((single6 + 87.0) / 87.0);
                        std::array<bool, 4> boolParams7 = getBoolParamaters(single8 / 20.0);
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index11").c_str(), 1).boolean(boolParams5[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index12").c_str(), 1).boolean(boolParams5[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index14").c_str(), 1).boolean(boolParams5[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index18").c_str(), 1).boolean(boolParams5[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index21").c_str(), 1).boolean(boolParams6[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index22").c_str(), 1).boolean(boolParams6[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index24").c_str(), 1).boolean(boolParams6[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index28").c_str(), 1).boolean(boolParams6[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index1spread1").c_str(), 1).boolean(boolParams7[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index1spread2").c_str(), 1).boolean(boolParams7[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index1spread4").c_str(), 1).boolean(boolParams7[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Index1spread8").c_str(), 1).boolean(boolParams7[0]).closeMessage();
                        packet.closeBundle();
                        try {
                            socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
                        } catch (std::exception& e) {

                        }
                    }
                    {
                        OSCPP::Client::Packet packet(buffer, 64 * 64);
                        // Middle
                        std::array<bool, 4> boolParams8 = getBoolParamaters((single11 + 80.0) / 80.0);
                        std::array<bool, 4> boolParams9 = getBoolParamaters((single10 + 87.0) / 87.0);
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle11").c_str(), 1).boolean(boolParams8[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle12").c_str(), 1).boolean(boolParams8[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle14").c_str(), 1).boolean(boolParams8[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle18").c_str(), 1).boolean(boolParams8[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle21").c_str(), 1).boolean(boolParams9[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle22").c_str(), 1).boolean(boolParams9[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle24").c_str(), 1).boolean(boolParams9[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Middle28").c_str(), 1).boolean(boolParams9[0]).closeMessage();
                        packet.closeBundle();
                        try {
                            socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
                        } catch (std::exception& e) {

                        }
                    }
                    {
                        OSCPP::Client::Packet packet(buffer, 64 * 64);
                        // Ring
                        std::array<bool, 4> boolParams10 = getBoolParamaters((single15 + 87.0) / 87.0);
                        std::array<bool, 4> boolParams11 = getBoolParamaters((single14 + 87.0) / 87.0);
                        std::array<bool, 4> boolParams12 = getBoolParamaters(single16 / 12.0f);
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring11").c_str(), 1).boolean(boolParams10[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring12").c_str(), 1).boolean(boolParams10[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring14").c_str(), 1).boolean(boolParams10[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring18").c_str(), 1).boolean(boolParams10[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring21").c_str(), 1).boolean(boolParams11[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring22").c_str(), 1).boolean(boolParams11[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring24").c_str(), 1).boolean(boolParams11[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring28").c_str(), 1).boolean(boolParams11[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring1spread1").c_str(), 1).boolean(boolParams12[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring1spread2").c_str(), 1).boolean(boolParams12[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring1spread4").c_str(), 1).boolean(boolParams12[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Ring1spread8").c_str(), 1).boolean(boolParams12[0]).closeMessage();
                        packet.closeBundle();
                        try {
                            socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
                        } catch (std::exception& e) {

                        }
                    }
                    {
                        OSCPP::Client::Packet packet(buffer, 64 * 64);
                        // Little Pinky
                        std::array<bool, 4> boolParams13 = getBoolParamaters((single19 + 75.0) / 75.0);
                        std::array<bool, 4> boolParams14 = getBoolParamaters((single18 + 90.0) / 90.0);
                        std::array<bool, 4> boolParams15 = getBoolParamaters(single20 / 40.0f);
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky11").c_str(), 1).boolean(boolParams13[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky12").c_str(), 1).boolean(boolParams13[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky14").c_str(), 1).boolean(boolParams13[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky18").c_str(), 1).boolean(boolParams13[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky21").c_str(), 1).boolean(boolParams14[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky22").c_str(), 1).boolean(boolParams14[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky24").c_str(), 1).boolean(boolParams14[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky28").c_str(), 1).boolean(boolParams14[0]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky1spread1").c_str(), 1).boolean(boolParams15[3]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky1spread2").c_str(), 1).boolean(boolParams15[2]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky1spread4").c_str(), 1).boolean(boolParams15[1]).closeMessage();
                        packet.openMessage(("/avatar/parameters/"s + prefix + "Pinky1spread8").c_str(), 1).boolean(boolParams15[0]).closeMessage();
                        packet.closeBundle();
                        try {
                            socket.send_to(boost::asio::buffer(packet.data(), packet.size()), *endpoints.begin());
                        } catch (std::exception& e) {

                        }
                    }
                }
            } catch (std::exception &e) {
                // std::cout << "Error sending packet: " << e.what() << std::endl;
            }
        }
    });
}

OSCSender::~OSCSender() {
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

void OSCSender::remove(bool left, bool right) {
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


void OSCSender::add(std::shared_ptr<UdCapV1Core> _core) {
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
        UdCapV1OSCPacket p;
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


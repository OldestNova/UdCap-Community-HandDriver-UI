//
// Created by max_3 on 2025/6/10.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_VMCSENDER_H
#define UDCAPCOMMUNITYDRIVERUI_VMCSENDER_H

#include <memory>
#include <functional>
#include <atomic>
#include <deque>
#include <boost/asio.hpp>
#include <UdCapV1Core.h>

class UdCapV1VMCPacket {
public:
    UdTarget target;
    std::shared_ptr<UdCapV1MCUPacket> data;
};

class VMCSender {
public:
    explicit VMCSender(std::string _host, uint16_t _port);
    void add(std::shared_ptr<UdCapV1Core> _core, UdTarget target);
    void remove(bool left, bool right);
    void updateController();
    ~VMCSender();
private:
    std::shared_ptr<UdCapV1Core> coreLeft;
    std::shared_ptr<UdCapV1Core> coreRight;
    std::string host;
    uint16_t port = 0;
    std::function<void()> unlistenLeft;
    std::function<void()> unlistenRight;
    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket;
    boost::asio::ip::basic_resolver_results<boost::asio::ip::udp> endpoints;

    std::mutex queueMutex;
    std::condition_variable cv;
    std::thread sendThread;
    std::deque<UdCapV1VMCPacket> packetQueue;
    std::atomic_bool running{true};

    float leftJoyXP = 0;
    float leftJoyXN = 0;
    float leftJoyYP = 0;
    float leftJoyYN = 0;
    float leftButtonA = 0;
    float leftButtonB = 0;
    float leftButtonJoy = 0;
    float leftButtonMenu = 0;
    float leftButtonTrackpad = 0;
    float leftButtonGrip = 0;
    float leftButtonTrigger = 0;
    float leftButtonPower = 0;
    float leftTrackpad = 0;
    float leftGrip = 0;
    float leftTrigger = 0;
    float rightJoyXP = 0;
    float rightJoyXN = 0;
    float rightJoyYP = 0;
    float rightJoyYN = 0;
    float rightButtonA = 0;
    float rightButtonB = 0;
    float rightButtonJoy = 0;
    float rightButtonMenu = 0;
    float rightButtonTrackpad = 0;
    float rightButtonGrip = 0;
    float rightButtonTrigger = 0;
    float rightButtonPower = 0;
    float rightTrackpad = 0;
    float rightGrip = 0;
    float rightTrigger = 0;
};


#endif //UDCAPCOMMUNITYDRIVERUI_VMCSENDER_H

//
// Created by max_3 on 2025/6/10.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_OSCSENDER_H
#define UDCAPCOMMUNITYDRIVERUI_OSCSENDER_H

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <UdCapV1Core.h>

class UdCapV1OSCPacket {
public:
    UdTarget target;
    std::shared_ptr<UdCapV1MCUPacket> data;
};

class OSCSender {
public:
    explicit OSCSender(std::string _host, uint16_t _port);
    void add(std::shared_ptr<UdCapV1Core> _core);
    void remove(bool left, bool right);
    ~OSCSender();
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
    std::mutex mtx;
    std::thread sendThread;
    std::queue<UdCapV1OSCPacket> packetQueue;
    bool running = true;
};


#endif //UDCAPCOMMUNITYDRIVERUI_OSCSENDER_H

//
// Created by max_3 on 25-6-23.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_OSCSERVER_H
#define UDCAPCOMMUNITYDRIVERUI_OSCSERVER_H

#include <string>
#include <cstdint>
#include <vector>
#include <functional>
#include <sigc++/signal.h>
#include <any>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <oscpp/server.hpp>

class OSCServer {
public:
    // Deleted to enforce singleton
    OSCServer(const OSCServer&) = delete;
    OSCServer& operator=(const OSCServer&) = delete;
    static OSCServer& getInstance();
    void restart();
    void setCallback(std::function<void(std::string, std::vector<std::any>)> cb);
private:
    OSCServer();
    ~OSCServer();
    std::mutex callbackMutex;
    std::thread worker;
    std::condition_variable workerCondition;
    std::mutex workerMutex;
    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket;
    std::array<char, 1024> recvBuffer;
    std::function<void(std::string, std::vector<std::any>)> callback;
    void startReceive();
    void stopReceive();
    void handleReceive(const boost::system::error_code& error, std::size_t /*bytes_transferred*/);
    void handleOSC(const OSCPP::Server::Packet& packet);
};


#endif //UDCAPCOMMUNITYDRIVERUI_OSCSERVER_H

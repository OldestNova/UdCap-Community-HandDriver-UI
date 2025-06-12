//
// Created by max_3 on 2025/6/10.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_OSCSENDER_H
#define UDCAPCOMMUNITYDRIVERUI_OSCSENDER_H

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <UdCapV1Core.h>
class OSCSender {
public:
    explicit OSCSender(std::string _host, uint16_t _port, std::shared_ptr<UdCapV1Core> _core);
    ~OSCSender();
private:
    std::shared_ptr<UdCapV1Core> core;
    std::string host;
    uint16_t port = 0;
    std::function<void()> unlisten;
    std::string prefix;
    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket;
    boost::asio::ip::basic_resolver_results<boost::asio::ip::udp> endpoints;
};


#endif //UDCAPCOMMUNITYDRIVERUI_OSCSENDER_H

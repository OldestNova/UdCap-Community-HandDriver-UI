//
// Created by max_3 on 2025/6/13.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_VMCCONTROLLERSENDER_H
#define UDCAPCOMMUNITYDRIVERUI_VMCCONTROLLERSENDER_H
#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <UdCapV1Core.h>

class VMCControllerSender {
public:
    explicit VMCControllerSender(std::string _host, uint16_t _port, std::shared_ptr<UdCapV1Core> _coreLeft, std::shared_ptr<UdCapV1Core> _coreRight);
    ~VMCControllerSender();
    void updateController();
private:
    std::shared_ptr<UdCapV1Core> coreLeft;
    std::shared_ptr<UdCapV1Core> coreRight;
    std::string host;
    uint16_t port = 0;
    std::vector<std::function<void()>> unlisten;
    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket;
    boost::asio::ip::basic_resolver_results<boost::asio::ip::udp> endpoints;
    float leftJoyXP = 0;
    float leftJoyXN = 0;
    float leftJoyYP = 0;
    float leftJoyYN = 0;
    float leftButtonA = 0;
    float leftButtonB = 0;
    float leftButtonJoy = 0;
    float leftButtonMenu = 0;
    float rightJoyXP = 0;
    float rightJoyXN = 0;
    float rightJoyYP = 0;
    float rightJoyYN = 0;
    float rightButtonA = 0;
    float rightButtonB = 0;
    float rightButtonJoy = 0;
    float rightButtonMenu = 0;
};


#endif //UDCAPCOMMUNITYDRIVERUI_VMCCONTROLLERSENDER_H

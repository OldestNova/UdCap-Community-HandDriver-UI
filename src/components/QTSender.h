//
// Created by max_3 on 2025/6/14.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_QTSENDER_H
#define UDCAPCOMMUNITYDRIVERUI_QTSENDER_H

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <rapidjson/document.h>
#include <UdCapV1Core.h>

struct UdCapV1QTStatus {
    UdTarget target;
    HandQuaternion mBones;
    uint64_t frame = 0;
    std::string deviceName;
    UdCapV1HandCaliStat calibrateStat;
    int battery = 100;
    float mJoyX = 0;
    float mJoyY = 0;
    float mButtonA = 0;
    float mButtonB = 0;
    float mButtonJoy = 0;
    float mButtonMenu = 0;
    float mButtonTrackpad = 0;
    float mButtonGrip = 0;
    float mButtonTrigger = 0;
    float mTrackpad = 0;
    float mGrip = 0;
    float mTrigger = 0;
    BoneQuaternion rIMU;
    rapidjson::Document doc;
};

class QTSender {
public:
    explicit QTSender(std::string _host, uint16_t _port);
    uint32_t add(std::shared_ptr<UdCapV1Core> _core);
    void remove(uint32_t fdDev);
    ~QTSender();
private:
    uint32_t nextFd = 0;
    std::mutex fdMutex;
    uint32_t getNextFd(std::string);
    std::map<std::string, uint32_t> usedFd;

    std::mutex mtx;
    std::map<uint32_t, std::shared_ptr<UdCapV1Core>> core;
    std::map<uint32_t, std::unique_ptr<UdCapV1QTStatus>> statusMap;
    std::map<uint32_t, std::function<void()>> unlisten;
    std::string host;
    uint16_t port = 0;
    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket;
    boost::asio::ip::basic_resolver_results<boost::asio::ip::udp> endpoints;
    std::thread sendThread;
    bool running = true;
};


#endif //UDCAPCOMMUNITYDRIVERUI_QTSENDER_H

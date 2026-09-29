#ifndef UDCAP_STEAMVR_SENDER_H
#define UDCAP_STEAMVR_SENDER_H

#include "SteamVRBridgeProtocol.h"
#include <UdCapV1Core.h>
#include <boost/asio.hpp>
#if defined(UDCAP_HAVE_OPENVR_CLIENT)
#include <openvr.h>
#endif
#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

class SteamVRSender {
public:
    SteamVRSender();
    ~SteamVRSender();
    void add(const std::shared_ptr<UdCapV1Core> &core);
    void setTrackingOffset(int hand, const std::array<float, 3> &positionMeters,
                    const std::array<float, 3> &rotationDegrees);
    void reloadTrackingOffsets();
    void pollHaptics();

private:
    boost::asio::io_context io;
    boost::asio::ip::udp::socket socket;
    boost::asio::ip::udp::socket hapticSocket;
    boost::asio::ip::udp::endpoint endpoint;
    std::mutex mutex;
    std::array<SteamVRBridgePacket, 2> state{};
    std::array<std::string, 2> gloveSerials{};
    std::array<std::function<void()>, 2> unlisten{};
    std::array<std::weak_ptr<UdCapV1Core>, 2> hapticCores{};
#if defined(UDCAP_HAVE_OPENVR_CLIENT)
    void refreshTrackerRoles();
    vr::IVRSystem *vrSystem = nullptr;
    std::chrono::steady_clock::time_point lastRoleRefresh{};
#endif
};

#endif

#include "OSCSender.h"
#include "VRChatPackets.h"
#include <HandOffsetProjection.h>
#include <algorithm>

OSCSender::OSCSender(std::string _host, uint16_t _port)
    : host(std::move(_host)), port(_port), socket(io_context) {
    socket.open(boost::asio::ip::udp::v4());
    boost::asio::ip::udp::resolver resolver(io_context);
    endpoints = resolver.resolve(boost::asio::ip::udp::endpoint{boost::asio::ip::make_address(host), port});
    sendThread = std::thread([this] {
        while (running) {
            UdCapV1OSCPacket p;
            {
                std::unique_lock lock(queueMutex);
                cv.wait(lock, [this] { return !running || !packetQueue.empty(); });
                if (!running) break;
                p = std::move(packetQueue.front());
                packetQueue.pop_front();
            }
            try {
                const auto angles = anglesWithHandOffset(p.data->result, p.target, p.data->angleBoneOffset);
                forEachVRChatPacket(angles, p.target, [this](const void *data, std::size_t size) {
                    socket.send_to(boost::asio::buffer(data, size), *endpoints.begin());
                });
            } catch (const std::exception &) {
                // The next full pose recovers from a dropped UDP frame.
            }
        }
    });
}

OSCSender::~OSCSender() {
    if (unlistenLeft) unlistenLeft();
    if (unlistenRight) unlistenRight();
    {
        std::lock_guard lock(queueMutex);
        running = false;
    }
    cv.notify_all();
    if (sendThread.joinable()) sendThread.join();
    boost::system::error_code ignored;
    socket.close(ignored);
}

void OSCSender::remove(bool left, bool right) {
    if (left && unlistenLeft) { unlistenLeft(); unlistenLeft = {}; coreLeft.reset(); }
    if (right && unlistenRight) { unlistenRight(); unlistenRight = {}; coreRight.reset(); }
    std::lock_guard lock(queueMutex);
    std::erase_if(packetQueue, [=](const auto &p) {
        return (left && p.target == UD_TARGET_LEFT_HAND) || (right && p.target == UD_TARGET_RIGHT_HAND);
    });
}

void OSCSender::add(std::shared_ptr<UdCapV1Core> core) {
    if (!core) throw std::invalid_argument("Core cannot be null");
    const auto target = core->getTarget();
    if (target != UD_TARGET_LEFT_HAND && target != UD_TARGET_RIGHT_HAND)
        throw std::invalid_argument("OSC requires an assigned hand");
    auto &assignedCore = target == UD_TARGET_LEFT_HAND ? coreLeft : coreRight;
    auto &unsubscribe = target == UD_TARGET_LEFT_HAND ? unlistenLeft : unlistenRight;
    if (assignedCore) throw std::runtime_error("Hand already exists");
    assignedCore = core;
    unsubscribe = core->listen([this, target](std::shared_ptr<UdCapV1MCUPacket> data) {
        if (data->commandType != CMD_ANGLE || !running) return;
        std::lock_guard lock(queueMutex);
        std::erase_if(packetQueue, [target](const auto &p) { return p.target == target; });
        packetQueue.push_back({target, std::move(data)});
        cv.notify_one();
    });
}

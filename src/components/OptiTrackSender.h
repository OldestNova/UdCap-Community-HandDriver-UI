#pragma once

#include <UdCapV1Core.h>
#include <OptiTrackWire.h>
#include <nng/nng.h>
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <string>

class OptiTrackSender {
public:
    explicit OptiTrackSender(const char *endpoint = udcap::optitrack::endpoint);
    ~OptiTrackSender();
    OptiTrackSender(const OptiTrackSender &) = delete;
    OptiTrackSender &operator=(const OptiTrackSender &) = delete;
    void add(const std::shared_ptr<UdCapV1Core> &core, const std::string &receiverSerial);
    void retainOnly(const std::set<UdCapV1Core *> &active);

private:
    nng_socket socket{};
    struct Listener {
        std::shared_ptr<UdCapV1Core> core;
        std::function<void()> unlisten;
    };
    std::map<UdCapV1Core *, Listener> listeners;
    std::atomic_uint64_t sequence{0};
};

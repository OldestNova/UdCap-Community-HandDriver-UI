//
// Created by max_3 on 25-6-23.
//

#include <iostream>
#include <algorithm>
#include <cctype>
#include "OSCServer.h"
#include "UserConfig.h"
#include "ModeConfig.h"

OSCServer &OSCServer::getInstance() {
    static OSCServer instance;
    return instance;
}

OSCServer::OSCServer(): io_context(), workGuard(boost::asio::make_work_guard(io_context)), socket(io_context), recvBuffer() {
    worker = std::thread([this]() {
        try {
            io_context.run();
        } catch (const std::exception &e) {
            std::cerr << "OSCServer: Error in io_context run: " << e.what() << std::endl;
        }
    });
}

OSCServer::~OSCServer() {
    stopping = true;
    boost::asio::post(io_context, [this] { stopReceive(); });
    workGuard.reset();
    if (worker.joinable()) worker.join();
}

void OSCServer::restart() {
    const auto prefix = activeModeConfigPrefix() + "/oscServer";
    const bool enabled = UserConfig::getInstance().get(prefix + "/enabled", false);
    const bool lan = UserConfig::getInstance().get(prefix + "/lan", false);
    const int port = UserConfig::getInstance().get(prefix + "/port", 8999);
    boost::asio::post(io_context, [this, enabled, lan, port] {
        if (stopping) return;
        stopReceive();
        if (!enabled || port < 1 || port > 65535) return;
        try {
            const auto address = boost::asio::ip::make_address(lan ? "0.0.0.0" : "127.0.0.1");
            socket.open(boost::asio::ip::udp::v4());
            socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            socket.bind({address, static_cast<unsigned short>(port)});
            startReceive();
        } catch (const std::exception &e) {
            std::cerr << "OSCServer: Error starting receive: " << e.what() << std::endl;
            stopReceive();
        }
    });
}


void OSCServer::startReceive() {
    if (stopping || !socket.is_open()) return;
    try {
        socket.async_receive(
                boost::asio::buffer(recvBuffer),
                [this](const boost::system::error_code& err, std::size_t len) {
                    try {
                        handleReceive(err, len);
                    } catch (const std::exception &e) {
                        std::cerr << "OSCServer: Error in handleReceive: " << e.what() << std::endl;
                    }
                    if (socket.is_open() && !stopping) startReceive();
                });
    } catch (const std::exception &e) {
        std::cerr << "OSCServer: Error starting receive: " << e.what() << std::endl;
    }
}

void OSCServer::stopReceive() {
    boost::system::error_code error;
    socket.cancel(error);
    socket.close(error);
}

void OSCServer::handleReceive(const boost::system::error_code &error, std::size_t bytes_transferred) {
    if (!error)
    {
        handleOSC(OSCPP::Server::Packet(&recvBuffer, bytes_transferred));
    }
}

void OSCServer::setCallback(std::function<void(std::string, std::vector<std::any>)> cb) {
    std::lock_guard lk(callbackMutex);
    callback = std::move(cb);
}

void OSCServer::handleOSC(const OSCPP::Server::Packet &packet) {
    if (packet.isBundle()) {
        OSCPP::Server::Bundle bundle(packet);
        OSCPP::Server::PacketStream packets(bundle.packets());

        while (!packets.atEnd()) {
            handleOSC(packets.next());
        }
    } else {
        OSCPP::Server::Message msg(packet);
        OSCPP::Server::ArgStream args(msg.args());
        std::string address = msg.address();
        std::transform(address.begin(), address.end(), address.begin(), [](unsigned char c){ return std::tolower(c); });
        std::vector<std::any> argsList;
        if (address == "/udcap/device/calibrate") {
            argsList.push_back(args.int32());
            argsList.push_back(args.int32());
            std::vector<std::string> serials;
            if (args.tag() == 's') {
                while (!args.atEnd()) {
                    serials.push_back(args.string());
                }
            } else {
                auto array = args.array();
                for (size_t i = 0; i < array.size(); ++i) {
                    serials.push_back(array.string());
                }
            }

            argsList.push_back(serials);
            std::lock_guard lk(callbackMutex);
            if (callback) callback(address, argsList);
        }
    }
}

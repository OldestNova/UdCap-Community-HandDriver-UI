//
// Created by max_3 on 25-6-23.
//

#include <iostream>
#include "OSCServer.h"
#include "UserConfig.h"

OSCServer &OSCServer::getInstance() {
    static OSCServer instance;
    return instance;
}

OSCServer::OSCServer(): io_context(), socket(io_context), recvBuffer() {
    worker = std::thread([this]() {
        while (true) {
            try {
                std::unique_lock lock(workerMutex);
                workerCondition.wait_for(lock, std::chrono::milliseconds(1000));
                io_context.run();
            } catch (const std::exception &e) {
                std::cerr << "OSCServer: Error in io_context run: " << e.what() << std::endl;
            }
        }
    });
    worker.detach();
}

OSCServer::~OSCServer() {
    stopReceive();
    io_context.stop();
}

void OSCServer::restart() {
    if (UserConfig::getInstance().get("/core/oscServer/enabled", false)) {
        if (socket.is_open()) {
            stopReceive();
        }
        startReceive();
    } else {
        stopReceive();
    }
}


void OSCServer::startReceive() {
    if (io_context.stopped()) return;
    try {
        if (!socket.is_open()) {
            int port = UserConfig::getInstance().get("/core/oscServer/port", 8999);
            if (UserConfig::getInstance().get("/core/oscServer/lan", false)) {
                socket.open(boost::asio::ip::udp::v4());
                socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
                socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
            } else {
                boost::asio::ip::udp::endpoint local_end_point(boost::asio::ip::make_address("127.0.0.1"),port);
                socket.open(boost::asio::ip::udp::v4());
                socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
                socket.bind(local_end_point);
            }
        }

        socket.async_receive(
                boost::asio::buffer(recvBuffer),
                [&](const boost::system::error_code& err, std::size_t len) {
                    try {
                        handleReceive(err, len);
                    } catch (const std::exception &e) {
                        std::cerr << "OSCServer: Error in handleReceive: " << e.what() << std::endl;
                    }
                    startReceive();
                    workerCondition.notify_all();
                });
        workerCondition.notify_all();
    } catch (const std::exception &e) {
        std::cerr << "OSCServer: Error starting receive: " << e.what() << std::endl;
    }
}

void OSCServer::stopReceive() {
    socket.close();
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
            if (callback != nullptr) callback(address, argsList);
        }
    }
}
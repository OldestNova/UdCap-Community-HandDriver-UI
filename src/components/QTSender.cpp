//
// Created by max_3 on 2025/6/14.
//

#include "QTSender.h"
#include "QingTongPose.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

uint32_t QTSender::getNextFd(std::string fName) {
    std::lock_guard<std::mutex> lock(fdMutex);
    if (usedFd.find(fName) != usedFd.end()) {
        return usedFd[fName];
    }
    const uint32_t fd = nextFd++;
    usedFd[fName] = fd;
    return fd;
}

QTSender::QTSender(std::string _host, uint16_t _port): host(_host), port(_port), io_context(), socket(io_context), endpoints() {
    socket.open(boost::asio::ip::udp::v4());
    boost::asio::ip::udp::resolver resolver(io_context);
    auto addr = boost::asio::ip::make_address(host);
    endpoints = resolver.resolve(boost::asio::ip::udp::endpoint{addr, port});
    sendThread = std::thread([this]() {
        auto last = std::chrono::system_clock::now();
        while (running) {
            auto now = std::chrono::system_clock::now() - last;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now);
            if (ms.count() < 8) {
                std::this_thread::sleep_for(std::chrono::milliseconds(8) - ms);
            }
            last = std::chrono::system_clock::now();
            std::vector<std::string> outgoing;
            std::unique_lock lk(mtx);
            for (auto& status: statusMap) {
                if (!status.second->hasPose || status.second->sentFrame == status.second->frame ||
                    std::chrono::steady_clock::now() - status.second->poseTime > std::chrono::seconds(2)) continue;
                status.second->doc["Frame"] = status.second->frame;
                status.second->doc["CalibrationStatus"] = (status.second->calibrateStat == 4 ? 3 : 0);
                status.second->doc["Battery"] = status.second->battery;
                status.second->doc["joyX"] = status.second->mJoyX;
                status.second->doc["joyY"] = status.second->mJoyY;
                status.second->doc["aButton"] = status.second->mButtonA != 0;
                status.second->doc["bButton"] = status.second->mButtonB != 0;
                status.second->doc["joyButton"] = status.second->mButtonJoy != 0;
                status.second->doc["menu"] = status.second->mButtonMenu != 0;
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][0];
                    bone[0] = status.second->mBones.indexFinger.proximal.x;
                    bone[1] = status.second->mBones.indexFinger.proximal.y;
                    bone[2] = status.second->mBones.indexFinger.proximal.z;
                    bone[3] = status.second->mBones.indexFinger.proximal.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][1];
                    bone[0] = status.second->mBones.indexFinger.intermediate.x;
                    bone[1] = status.second->mBones.indexFinger.intermediate.y;
                    bone[2] = status.second->mBones.indexFinger.intermediate.z;
                    bone[3] = status.second->mBones.indexFinger.intermediate.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][2];
                    bone[0] = status.second->mBones.indexFinger.distal.x;
                    bone[1] = status.second->mBones.indexFinger.distal.y;
                    bone[2] = status.second->mBones.indexFinger.distal.z;
                    bone[3] = status.second->mBones.indexFinger.distal.w;
                }

                {
                    rapidjson::Value& bone = status.second->doc["Bones"][3];
                    bone[0] = status.second->mBones.middleFinger.proximal.x;
                    bone[1] = status.second->mBones.middleFinger.proximal.y;
                    bone[2] = status.second->mBones.middleFinger.proximal.z;
                    bone[3] = status.second->mBones.middleFinger.proximal.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][4];
                    bone[0] = status.second->mBones.middleFinger.intermediate.x;
                    bone[1] = status.second->mBones.middleFinger.intermediate.y;
                    bone[2] = status.second->mBones.middleFinger.intermediate.z;
                    bone[3] = status.second->mBones.middleFinger.intermediate.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][5];
                    bone[0] = status.second->mBones.middleFinger.distal.x;
                    bone[1] = status.second->mBones.middleFinger.distal.y;
                    bone[2] = status.second->mBones.middleFinger.distal.z;
                    bone[3] = status.second->mBones.middleFinger.distal.w;
                }

                {
                    rapidjson::Value& bone = status.second->doc["Bones"][6];
                    bone[0] = status.second->mBones.ringFinger.proximal.x;
                    bone[1] = status.second->mBones.ringFinger.proximal.y;
                    bone[2] = status.second->mBones.ringFinger.proximal.z;
                    bone[3] = status.second->mBones.ringFinger.proximal.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][7];
                    bone[0] = status.second->mBones.ringFinger.intermediate.x;
                    bone[1] = status.second->mBones.ringFinger.intermediate.y;
                    bone[2] = status.second->mBones.ringFinger.intermediate.z;
                    bone[3] = status.second->mBones.ringFinger.intermediate.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][8];
                    bone[0] = status.second->mBones.ringFinger.distal.x;
                    bone[1] = status.second->mBones.ringFinger.distal.y;
                    bone[2] = status.second->mBones.ringFinger.distal.z;
                    bone[3] = status.second->mBones.ringFinger.distal.w;
                }


                {
                    rapidjson::Value& bone = status.second->doc["Bones"][9];
                    bone[0] = status.second->mBones.littleFinger.proximal.x;
                    bone[1] = status.second->mBones.littleFinger.proximal.y;
                    bone[2] = status.second->mBones.littleFinger.proximal.z;
                    bone[3] = status.second->mBones.littleFinger.proximal.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][10];
                    bone[0] = status.second->mBones.littleFinger.intermediate.x;
                    bone[1] = status.second->mBones.littleFinger.intermediate.y;
                    bone[2] = status.second->mBones.littleFinger.intermediate.z;
                    bone[3] = status.second->mBones.littleFinger.intermediate.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][11];
                    bone[0] = status.second->mBones.littleFinger.distal.x;
                    bone[1] = status.second->mBones.littleFinger.distal.y;
                    bone[2] = status.second->mBones.littleFinger.distal.z;
                    bone[3] = status.second->mBones.littleFinger.distal.w;
                }

                {
                    rapidjson::Value& bone = status.second->doc["Bones"][12];
                    bone[0] = status.second->mBones.thumbFinger.proximal.x;
                    bone[1] = status.second->mBones.thumbFinger.proximal.y;
                    bone[2] = status.second->mBones.thumbFinger.proximal.z;
                    bone[3] = status.second->mBones.thumbFinger.proximal.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][13];
                    bone[0] = status.second->mBones.thumbFinger.intermediate.x;
                    bone[1] = status.second->mBones.thumbFinger.intermediate.y;
                    bone[2] = status.second->mBones.thumbFinger.intermediate.z;
                    bone[3] = status.second->mBones.thumbFinger.intermediate.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][14];
                    bone[0] = status.second->mBones.thumbFinger.distal.x;
                    bone[1] = status.second->mBones.thumbFinger.distal.y;
                    bone[2] = status.second->mBones.thumbFinger.distal.z;
                    bone[3] = status.second->mBones.thumbFinger.distal.w;
                }
                {
                    rapidjson::Value& bone = status.second->doc["Bones"][15];
                    bone[0] = status.second->rIMU.x;
                    bone[1] = status.second->rIMU.y;
                    bone[2] = status.second->rIMU.z;
                    bone[3] = status.second->rIMU.w;
                }
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                status.second->doc.Accept(writer);
                outgoing.emplace_back(buffer.GetString(), buffer.GetSize());
                status.second->sentFrame = status.second->frame;
            }
            lk.unlock();
            for (const auto &message : outgoing) {
                try {
                    socket.send_to(boost::asio::buffer(message), *endpoints.begin());
                } catch (std::exception& e) {
                    // Handle exception if needed
                }
            }
        }
    });
}

QTSender::~QTSender() {
    running = false;
    if (sendThread.joinable()) {
        sendThread.join();
    }
    for (auto& [fd, unfunc]: unlisten) {
        if (unfunc) {
            unfunc();
        }
    }
    try {
        socket.cancel();
        socket.close();
    } catch (std::exception& e) {
        // Handle exception if needed
    }
}

uint32_t QTSender::add(std::shared_ptr<UdCapV1Core> _core,
                       std::string receiverKey, uint32_t deviceId) {
    if (_core == nullptr) {
        throw std::invalid_argument("Core cannot be null");
    }
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto& [fd, existingCore]: core) {
        if (existingCore == _core) {
            throw std::runtime_error("Core already exists");
        }
    }
    if (receiverKey.empty()) receiverKey = _core->getUDCapSerial();
    if (receiverKey.empty()) receiverKey = std::to_string(reinterpret_cast<std::uintptr_t>(_core.get()));
    uint32_t fd = getNextFd(receiverKey);
    core[fd] = _core;
    statusMap[fd] = std::make_unique<UdCapV1QTStatus>();
    statusMap[fd]->receiverKey = receiverKey;
    statusMap[fd]->calibrateStat = _core->getHandCalibrationStatus();
    if (const auto battery = _core->getBattery()) statusMap[fd]->battery = battery->first;
    {
        statusMap[fd]->doc.SetObject();
        rapidjson::Document::AllocatorType &allocator = statusMap[fd]->doc.GetAllocator();
        statusMap[fd]->doc.AddMember("DeviceID", deviceId, allocator);
        const std::string name = _core->getUDCapSerial().empty() ? receiverKey : _core->getUDCapSerial();
        rapidjson::Value deviceName;
        deviceName.SetString(name.c_str(), static_cast<rapidjson::SizeType>(name.size()), allocator);
        statusMap[fd]->doc.AddMember("DeviceName", deviceName, allocator);
        statusMap[fd]->doc.AddMember("Frame", 0, allocator);
        statusMap[fd]->doc.AddMember("CalibrationStatus", 0, allocator);
        statusMap[fd]->doc.AddMember("Battery", statusMap[fd]->battery, allocator);
        rapidjson::Value bones(rapidjson::kArrayType);
        for (int i = 0; i < (15); i++) {
            rapidjson::Value bone(rapidjson::kArrayType);
            bone.PushBack(0, allocator);
            bone.PushBack(0, allocator);
            bone.PushBack(0, allocator);
            bone.PushBack(1, allocator);
            bones.PushBack(bone, allocator);
        }
        {
            rapidjson::Value bone(rapidjson::kArrayType);
            bone.PushBack(0, allocator);
            bone.PushBack(0, allocator);
            bone.PushBack(0, allocator);
            bone.PushBack(1, allocator);
            bones.PushBack(bone, allocator);
        }
        statusMap[fd]->doc.AddMember("Bones", bones, allocator);
        statusMap[fd]->doc.AddMember("joyX", 0, allocator);
        statusMap[fd]->doc.AddMember("joyY", 0, allocator);
        statusMap[fd]->doc.AddMember("aButton", false, allocator);
        statusMap[fd]->doc.AddMember("bButton", false, allocator);
        statusMap[fd]->doc.AddMember("joyButton", false, allocator);
        statusMap[fd]->doc.AddMember("menu", false, allocator);
    }
    std::function<void()> un = _core->listen([this, fd](std::shared_ptr<UdCapV1MCUPacket> data) {
        std::lock_guard<std::mutex> lock(mtx);
        if (statusMap.find(fd) == statusMap.end()) return;
        if (data->commandType == CMD_SERIAL && !data->deviceSerialNum.empty()) {
            auto &document = statusMap[fd]->doc;
            document["DeviceName"].SetString(data->deviceSerialNum.c_str(),
                static_cast<rapidjson::SizeType>(data->deviceSerialNum.size()), document.GetAllocator());
        } else if (data->commandType == CMD_READY) {
            // CMD_READY already carries the state; keep packet handling
            // independent of a second read from Core.
            statusMap[fd]->calibrateStat = data->isReady
                ? UDCAP_V1_HAND_CALI_STAT_COMPLETED : UDCAP_V1_HAND_CALI_STAT_NONE;
            if (!data->isReady) statusMap[fd]->hasPose = false;
        } else if (data->commandType == CMD_LINK_STATE && data->udState == UD_INIT_STATE_NOT_CONNECT) {
            statusMap[fd]->hasPose = false;
            statusMap[fd]->battery = 0;
        } else if (data->commandType == CMD_BATTERY) {
            statusMap[fd]->battery = data->battery;
        } else if (data->commandType == CMD_INPUT_JOYSTICK) {
            statusMap[fd]->mJoyX = data->joystickData.joyX;
            statusMap[fd]->mJoyY = data->joystickData.joyY;
        } else if (data->commandType == CMD_INPUT_BUTTON) {
            statusMap[fd]->mButtonA = data->button.btnA;
            statusMap[fd]->mButtonB = data->button.btnB;
            statusMap[fd]->mButtonJoy = data->button.btnJoyStick;
            statusMap[fd]->mButtonMenu = data->button.btnMenu;
            statusMap[fd]->mButtonTrackpad = data->button.btnTrackpad;
            statusMap[fd]->mButtonGrip = data->button.btnGrip;
            statusMap[fd]->mButtonTrigger = data->button.btnTrigger;
            statusMap[fd]->mTrackpad = data->button.trackpad;
            statusMap[fd]->mGrip = data->button.grip;
            statusMap[fd]->mTrigger = data->button.trigger;
        } else if (data->commandType == CMD_ANGLE) {
            statusMap[fd]->mBones = poseForQingTong(data->result, data->angleHand, data->angleBoneOffset);
            statusMap[fd]->hasPose = true;
            statusMap[fd]->calibrateStat = UDCAP_V1_HAND_CALI_STAT_COMPLETED;
            statusMap[fd]->poseTime = std::chrono::steady_clock::now();
            ++statusMap[fd]->frame;
            statusMap[fd]->rIMU.x = data->result[24];
            statusMap[fd]->rIMU.y = data->result[25];
            statusMap[fd]->rIMU.z = data->result[26];
            statusMap[fd]->rIMU.w = data->result[27];
            auto &imu = statusMap[fd]->rIMU;
            const double norm = std::sqrt(imu.x*imu.x + imu.y*imu.y + imu.z*imu.z + imu.w*imu.w);
            if (!std::isfinite(norm) || norm < .0001) imu = {0, 0, 0, 1};
            else { imu.x /= norm; imu.y /= norm; imu.z /= norm; imu.w /= norm; }
        }
    });
    unlisten[fd] = un;
    statusMap[fd]->deviceName = _core->getUDCapSerial();
    statusMap[fd]->target = _core->getTarget();
    return fd;
}

void QTSender::remove(uint32_t fdDev) {
    std::function<void()> unsubscribe;
    std::shared_ptr<UdCapV1Core> keepAlive;
    {
        std::lock_guard lock(mtx);
        const auto it = core.find(fdDev);
        if (it == core.end()) return;
        keepAlive = std::move(it->second);
        core.erase(it);
        unsubscribe = std::move(unlisten[fdDev]);
        unlisten.erase(fdDev);
        statusMap.erase(fdDev);
    }
    // Unsubscribe waits for an in-flight callback, which also needs mtx.
    if (unsubscribe) unsubscribe();
}

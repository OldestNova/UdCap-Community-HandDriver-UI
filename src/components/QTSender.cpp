//
// Created by max_3 on 2025/6/14.
//

#include "QTSender.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

uint32_t QTSender::getNextFd(std::string fName) {
    std::lock_guard<std::mutex> lock(fdMutex);
    if (usedFd.find(fName) != usedFd.end()) {
        return usedFd[fName];
    }
    return nextFd++;
}

QTSender::QTSender(std::string _host, uint16_t _port): host(_host), port(_port), io_context(), socket(io_context), endpoints() {
    socket.open(boost::asio::ip::udp::v4());
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
            std::unique_lock lk(mtx);
            for (auto& status: statusMap) {
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
                try {
                    socket.send_to(boost::asio::buffer(buffer.GetString(), buffer.GetSize()), *endpoints.begin());
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

uint32_t QTSender::add(std::shared_ptr<UdCapV1Core> _core) {
    if (_core == nullptr) {
        throw std::invalid_argument("Core cannot be null");
    }
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto& [fd, existingCore]: core) {
        if (existingCore == _core) {
            throw std::runtime_error("Core already exists");
        }
    }
    uint32_t fd = getNextFd(_core->getUDCapSerial());
    core[fd] = _core;
    statusMap[fd] = std::make_unique<UdCapV1QTStatus>();
    {
        statusMap[fd]->doc.SetObject();
        rapidjson::Document::AllocatorType &allocator = statusMap[fd]->doc.GetAllocator();
        statusMap[fd]->doc.AddMember("DeviceID", fd, allocator);
        statusMap[fd]->doc.AddMember("Frame", 0, allocator);
        statusMap[fd]->doc.AddMember("CalibrationStatus", 0, allocator);
        statusMap[fd]->doc.AddMember("Battery", 100, allocator);
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
    std::function<void()> un = _core->listen([this, fd, _core](std::shared_ptr<UdCapV1MCUPacket> data) {
        std::lock_guard<std::mutex> lock(mtx);
        if (data->commandType == CMD_SKELETON_QUATERNION) {
            statusMap[fd]->mBones = data->skeletonQuaternion;
        } else if (data->commandType == CMD_READY) {
            statusMap[fd]->calibrateStat = _core->getHandCalibrationStatus();
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
            statusMap[fd]->rIMU.x = data->result[24];
            statusMap[fd]->rIMU.y = data->result[25];
            statusMap[fd]->rIMU.z = data->result[26];
            statusMap[fd]->rIMU.w = data->result[27];
        }
        statusMap[fd]->frame++;
    });
    unlisten[fd] = un;
    statusMap[fd]->deviceName = _core->getUDCapSerial();
    statusMap[fd]->target = _core->getTarget();
    return fd;
}

void QTSender::remove(uint32_t fdDev) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = core.find(fdDev);
    if (it != core.end()) {
        if (unlisten.find(fdDev) != unlisten.end()) {
            unlisten[fdDev]();
            unlisten.erase(fdDev);
        }
        std::string serial = it->second->getUDCapSerial();
        core.erase(it);
        statusMap.erase(fdDev);
        usedFd[serial] = fdDev;
    }
}
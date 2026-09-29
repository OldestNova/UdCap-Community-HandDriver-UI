//
// Created by max_3 on 2025/6/19.
//

#include "UserConfig.h"

UserConfig &UserConfig::getInstance() {
    static UserConfig instance;
    return instance;
}

void UserConfig::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream file(configFilePath_);
    if (file.good()) {
        rapidjson::IStreamWrapper isw(file);
        document.ParseStream(isw);
        configExists_ = !document.HasParseError() && document.IsObject();
        if (!configExists_) document.SetObject();
    } else {
        document.SetObject();
        configExists_ = false;
    }
}

void UserConfig::save() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(configFilePath_);
    rapidjson::OStreamWrapper osw(file);
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    document.Accept(writer);
}

std::string UserConfig::getConfigDirPath() const {
    return configDir_;
}

std::string UserConfig::getConfigCoreDirPath() const {
    return configCoreDir_;
}

bool UserConfig::configExists() const {
    return configExists_;
}

bool UserConfig::configCreated() const {
    return configDirCreated_;
}

UserConfig::UserConfig() {
    document.SetObject();
    configDir_ = platformdirs::user_config_dir("UdCapCommunityDriverUI", "UdCapCommunity");
    configFilePath_ = configDir_ + "/pref.json";
    configCoreDir_ = configDir_ + "/core";

    // Ensure config directory exists
    if (!std::filesystem::exists(configDir_)) {
        configDirCreated_ = std::filesystem::create_directories(configDir_);
        if (configDirCreated_) {
            configDirCreated_ = std::filesystem::create_directories(configCoreDir_);
        }
    } else {
        configDirCreated_ = true;
        if (!std::filesystem::exists(configCoreDir_)) {
            configDirCreated_ = std::filesystem::create_directories(configCoreDir_);
        }
    }
}


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
        try {
            boost::property_tree::read_json(file, pt_);
            configExists_ = true;
        } catch (const boost::property_tree::json_parser::json_parser_error& e) {
            configExists_ = false;
        }
    } else {
        configExists_ = false;
    }
}

void UserConfig::save() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(configFilePath_);
    boost::property_tree::write_json(file, pt_);
}

boost::property_tree::ptree& UserConfig::getTree() {
    return pt_;
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


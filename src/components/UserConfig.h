//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_USERCONFIG_H
#define UDCAPCOMMUNITYDRIVERUI_USERCONFIG_H

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <platformdirs.h>
#include <fstream>
#include <mutex>
#include <filesystem>

class UserConfig {
public:
    static UserConfig& getInstance();
    void load();
    void save();

    boost::property_tree::ptree& getTree();

    std::string getConfigDirPath() const;
    std::string getConfigCoreDirPath() const;

    bool configExists() const;
    bool configCreated() const;

private:
    UserConfig();

    // Deleted to enforce singleton
    UserConfig(const UserConfig&) = delete;
    UserConfig& operator=(const UserConfig&) = delete;

    boost::property_tree::ptree pt_;
    std::string configDir_;
    std::string configCoreDir_;
    std::string configFilePath_;
    std::mutex mutex_;
    bool configExists_ = false;
    bool configDirCreated_ = false;
};

#endif //UDCAPCOMMUNITYDRIVERUI_USERCONFIG_H

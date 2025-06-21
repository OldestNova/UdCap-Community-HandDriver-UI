//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_USERCONFIG_H
#define UDCAPCOMMUNITYDRIVERUI_USERCONFIG_H

#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include "rapidjson/filewritestream.h"
#include "rapidjson/pointer.h"
#include <platformdirs.h>
#include <fstream>
#include <mutex>
#include <filesystem>

class UserConfig {
public:
    static UserConfig& getInstance();
    void load();
    void save();
    template<typename T>
    T get(const char* key, T defaultValue) {
        std::lock_guard lock(mutex_);
        auto* value = rapidjson::Pointer(key).Get(document);
        if (value == nullptr || value->IsNull()) {
            return defaultValue;
        }
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, const char*>) {
            return value->GetString();
        } else if constexpr (std::is_same_v<T, int>) {
            return value->GetInt();
        } else if constexpr (std::is_same_v<T, bool>) {
            return value->GetBool();
        } else if constexpr (std::is_same_v<T, double>) {
            return value->GetDouble();
        } else {
            return defaultValue; // Fallback for unsupported types
        }
    }

    template<typename T>
    void set(const char* key, T value) {
        std::lock_guard lock(mutex_);
        if constexpr (std::is_same_v<T, std::string>) {
            rapidjson::Pointer(key).Set(document, value.c_str());
        } else {
            rapidjson::Pointer(key).Set(document, value);
        }
    }

    template<typename T>
    T get(std::string key, T defaultValue) {
        return get<T>(key.c_str(), defaultValue);
    }

    template<typename T>
    void set(std::string key, T value) {
        set<T>(key.c_str(), value);
    }

    std::string getConfigDirPath() const;
    std::string getConfigCoreDirPath() const;

    bool configExists() const;
    bool configCreated() const;

private:
    UserConfig();

    // Deleted to enforce singleton
    UserConfig(const UserConfig&) = delete;
    UserConfig& operator=(const UserConfig&) = delete;

    rapidjson::Document document;
    std::string configDir_;
    std::string configCoreDir_;
    std::string configFilePath_;
    std::mutex mutex_;
    bool configExists_ = false;
    bool configDirCreated_ = false;


};

#endif //UDCAPCOMMUNITYDRIVERUI_USERCONFIG_H

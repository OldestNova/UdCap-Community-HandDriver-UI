#pragma once

#include "UserConfig.h"
#include "config.h"
#include <string>

inline std::string activeModeConfigPrefix() {
    return UserConfig::getInstance().get<int>("/core/driverType", 0) ==
               static_cast<int>(UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE)
        ? "/enterprise" : "/consumer";
}

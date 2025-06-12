#include <iostream>
#include <optional>
#include <filesystem>
#include "main.h"
#include "ui/ConsumerUI.h"
#include "ui/EnterpriseUI.h"
#include "ui/CalibrationUI.h"
#include "config.h"
#include <config-cxx/Config.h>

config::Config conf;
// App
int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("com.udcap.community.driver", Gio::Application::Flags::NONE);
    auto cxxConfigDir = std::getenv("CXX_CONFIG_DIR");
    if (cxxConfigDir == nullptr) {
        if (!std::filesystem::exists("./config")) {
            if (!std::filesystem::create_directory("./config")) {
                std::cerr << "Failed to create config directory" << std::endl;
                return -1;
            }
        }
    } else {
        if (!std::filesystem::exists(cxxConfigDir)) {
            if (!std::filesystem::create_directory(cxxConfigDir)) {
                std::cerr << "Failed to create config directory at " << cxxConfigDir << std::endl;
                return -1;
            }
        }
    }
    auto driverType = UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER;
    try {
        driverType = static_cast<UdCapDriverType>(conf.getOptional<int>("core.driverType").value_or(
                UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER));
    } catch (std::runtime_error&) {
        std::cerr << "Failed to read driver type from config, defaulting to consumer" << std::endl;
        driverType = UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER;
    }
    if (driverType == UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE) {
        return app->make_window_and_run<EnterpriseUI>(argc, argv);
    } else if (driverType == UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER) {
        return app->make_window_and_run<ConsumerUI>(argc, argv);
    }
    return -1;
}

#if WIN32
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Call the main function
    return main(__argc, __argv);
}
#endif
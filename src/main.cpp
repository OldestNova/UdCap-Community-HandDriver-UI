#include <iostream>
#include <optional>
#include "ui/ConsumerUI.h"
#include "ui/EnterpriseUI.h"
#include "ui/CalibrationUI.h"
#include "ui/ErrorUI.h"
#include "ui/FirstRunUI.h"
#include "config.h"
#include "components/UserConfig.h"
#include <CorePref.h>

// App
int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("com.udcap.community.driver", Gio::Application::Flags::NONE);
    UserConfig::getInstance().load();
    if (!UserConfig::getInstance().configExists()) {
        if (!UserConfig::getInstance().configCreated()) {
            return app->make_window_and_run<ErrorUI>(argc, argv, _("Failed to create config directory"));
        }
    }
    std::cout << "Config directory: " << UserConfig::getInstance().getConfigDirPath() << std::endl;

    int configVersion = UserConfig::getInstance().get<int>("/core/configVersion", 0);
    if (configVersion < 1) {
        std::shared_ptr<bool> resultFlag = std::make_shared<bool>(false);
        app->make_window_and_run<FirstRunUI>(argc, argv, resultFlag);
        app = Gtk::Application::create("com.udcap.community.driver", Gio::Application::Flags::NONE);
        if (!(*resultFlag)) {
            return 1;
        }
    } else {
        // Update config if needed
    }
    if (!UserConfig::getInstance().get<bool>("/core/shareCalibrateData", false)) {
        CorePref::getInstance().setDefaultPrefPath(UserConfig::getInstance().getConfigCoreDirPath());
    }
    UdCapDriverType driverType = static_cast<UdCapDriverType>(UserConfig::getInstance().get<int>("/core/driverType", static_cast<int>(UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER)));
    if (driverType == UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE) {
        return app->make_window_and_run<EnterpriseUI>(argc, argv, app);
    } else if (driverType == UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER) {
        return app->make_window_and_run<ConsumerUI>(argc, argv, app);
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
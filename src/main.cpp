#include <iostream>
#include <optional>
#include <cstdlib>
#include <string>
#include "ui/ConsumerUI.h"
#include "ui/EnterpriseUI.h"
#include "ui/CalibrationUI.h"
#include "ui/ErrorUI.h"
#include "ui/FirstRunUI.h"
#include "config.h"
#include "components/UserConfig.h"
#include "components/Localization.h"
#include "components/AppRestart.h"
#include <CorePref.h>
#include <filesystem>

namespace {
void migrateLegacyModeSettings() {
    auto &config = UserConfig::getInstance();
    if (config.get<bool>("/core/modeSettingsMigrated", false)) return;
    for (const std::string mode : {"/consumer", "/enterprise"}) {
        config.set<bool>(mode + "/preview3d", config.get<bool>("/core/preview3d", false));
        config.set<bool>(mode + "/oscServer/enabled", config.get<bool>("/core/oscServer/enabled", false));
        config.set<bool>(mode + "/oscServer/lan", config.get<bool>("/core/oscServer/lan", false));
        config.set<int>(mode + "/oscServer/port", config.get<int>("/core/oscServer/port", 8999));
        config.set<bool>(mode + "/optitrack/enabled", config.get<bool>("/core/optitrack/enabled", false));
    }
    for (const std::string hand : {"left", "right"}) {
        config.set<int>("/consumer/algorithm/" + hand, config.get<int>("/core/algorithm/" + hand, 0));
        for (const std::string kind : {"position", "rotation"}) {
            for (const std::string axis : {"x", "y", "z"}) {
                const std::string suffix = "/" + kind + "/" + axis;
                config.set<double>("/consumer/steamvr/" + hand + suffix,
                                   config.get<double>("/steamvr/" + hand + suffix, 0));
            }
        }
    }
    config.set<bool>("/core/modeSettingsMigrated", true);
    config.save();
}
}
#if defined(_WIN32)
#include <Windows.h>
#include <psapi.h>
#endif

// App
int main(int argc, char* argv[]) {
#if defined(_WIN32)
#if defined(NO_3DPREVIEW)
    // GTK's Win32 backend can initialize Direct3D even with the Cairo renderer.
    // A build without GL preview can use the software presentation path too.
    _putenv_s("GSK_RENDERER", "cairo");
    const char *existingGdkDisable = std::getenv("GDK_DISABLE");
    const std::string gdkDisable = existingGdkDisable && *existingGdkDisable
                                       ? std::string(existingGdkDisable) + ",d3d11,d3d12"
                                       : "d3d11,d3d12";
    _putenv_s("GDK_DISABLE", gdkDisable.c_str());
#else
    // Prefer Cairo for the mostly static UI while allowing an explicit override.
    if (!std::getenv("GSK_RENDERER")) _putenv_s("GSK_RENDERER", "cairo");
#endif
#endif
    UserConfig::getInstance().load();
    initializeUiLocalization(UserConfig::getInstance().get<std::string>("/core/language", "system"),
                             argc > 0 ? argv[0] : nullptr);
    auto app = Gtk::Application::create("com.udcap.community.driver", Gio::Application::Flags::NONE);
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
    migrateLegacyModeSettings();
#if defined(_WIN32)
    // GTK's Windows startup can leave a large working set. Once the first
    // window has settled, request one trim without imposing a permanent
    // working-set limit or repeatedly paging active UI data out.
    Glib::signal_timeout().connect([]() {
        K32EmptyWorkingSet(GetCurrentProcess());
        return false;
    }, 20000);
#endif
    app.reset();
    for (;;) {
        const auto driverType = static_cast<UdCapDriverType>(UserConfig::getInstance().get<int>(
            "/core/driverType", static_cast<int>(UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER)));
        const std::filesystem::path legacyCorePath = UserConfig::getInstance().getConfigCoreDirPath();
        const std::filesystem::path corePath = legacyCorePath /
            (driverType == UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE ? "enterprise" : "consumer");
        // Existing per-glove preferences become the initial settings for each mode.
        // After the copy, each mode writes only to its own directory.
        std::filesystem::create_directories(corePath);
        const std::string migrationKey = driverType == UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE
            ? "/core/enterprisePrefsMigrated" : "/core/consumerPrefsMigrated";
        if (!UserConfig::getInstance().get<bool>(migrationKey, false)) {
            for (const auto &entry : std::filesystem::directory_iterator(legacyCorePath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                    std::filesystem::copy_file(entry.path(), corePath / entry.path().filename(),
                                               std::filesystem::copy_options::skip_existing);
            }
            UserConfig::getInstance().set<bool>(migrationKey, true);
            UserConfig::getInstance().save();
        }
        CorePref::getInstance().setDefaultPrefPath(corePath.string());
        app = Gtk::Application::create("com.udcap.community.driver", Gio::Application::Flags::NONE);
        const int result = driverType == UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE
            ? app->make_window_and_run<EnterpriseUI>(argc, argv, app)
            : app->make_window_and_run<ConsumerUI>(argc, argv, app);
        app.reset();
        if (!AppRestart::consume()) return result;
    }
}

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Call the main function
    return main(__argc, __argv);
}
#endif

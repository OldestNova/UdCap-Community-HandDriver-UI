//
// Created by max_3 on 25-6-23.
//

#include "PreferenceDialog.h"
#include "components/UserConfig.h"
#include "config.h"
#include "components/OSCServer.h"
#include "components/AppRestart.h"
#include "components/ModeConfig.h"
#include "SteamVRSettingsBox.h"
#include <algorithm>


PreferenceDialog::PreferenceDialog(std::function<void()> onSteamVRChanged) {
    set_title(_("Preferences"));
    set_default_size(400, 300);

    mainBox.set_orientation(Gtk::Orientation::VERTICAL);
    mainBox.set_margin(15);
    mainBox.set_spacing(20);
    populatePreferences(mainBox, *this, std::move(onSteamVRChanged));
    set_child(mainBox);
}

void populatePreferences(Gtk::Box &mainBox, Gtk::Window &owner, std::function<void()> onSteamVRChanged) {
    const std::string modePrefix = activeModeConfigPrefix();
    auto modeFrame = Gtk::make_managed<Gtk::Frame>(_("Mode"));
    auto modeBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    auto switchButton = Gtk::make_managed<Gtk::Button>("");
    const bool simpleMode = static_cast<UdCapDriverType>(UserConfig::getInstance().get<int>("/core/driverType", 0))
                            == UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER;
    const auto nextMode = simpleMode ? UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE
                                     : UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER;
    switchButton->set_label(simpleMode ? _("Switch to Advanced Mode") : _("Switch to Simple Mode"));
    switchButton->signal_clicked().connect([&owner, nextMode, simpleMode]() {
        auto dialog = Gtk::AlertDialog::create(simpleMode
            ? _("Switching to advanced mode will require a restart. \nDo you want to continue?")
            : _("Switching to simple mode will require a restart. \nDo you want to continue?"));
        dialog->set_buttons({ _("No"), _("Yes") });
        dialog->set_default_button(1);
        dialog->set_cancel_button(0);
        auto mainWindow = owner.get_transient_for() ? owner.get_transient_for() : &owner;
        auto app = mainWindow->get_application();
        dialog->choose(owner, [dialog, app, mainWindow, nextMode](Glib::RefPtr<Gio::AsyncResult> &result) {
            try {
                if (dialog->choose_finish(result) != 1) return;
                UserConfig::getInstance().set<int>("/core/driverType", static_cast<int>(nextMode));
                UserConfig::getInstance().save();
                AppRestart::request();
                // Close the main window after this dialog callback returns so its normal
                // destruction path can stop receivers and senders before the app exits.
                Glib::signal_idle().connect_once([app, mainWindow]() {
                    if (app && mainWindow) {
                        const auto windows = app->get_windows();
                        if (std::find(windows.begin(), windows.end(), mainWindow) != windows.end()) {
                            mainWindow->close();
                            return;
                        }
                    }
                    if (app) app->quit();
                });
            } catch (const Glib::Error &) {
                // The confirmation was dismissed while the window was closing.
            }
        });
    });

    modeBox->append(*switchButton);
    modeFrame->set_child(*modeBox);

    auto languageFrame = Gtk::make_managed<Gtk::Frame>(_("Language"));
    auto languageBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    languageBox->set_margin(8);
    auto languageChoice = Gtk::make_managed<Gtk::ComboBoxText>();
    languageChoice->append("system", _("Follow system language"));
    languageChoice->append("en", "English");
    languageChoice->append("zh_CN", "简体中文");
    const auto savedLanguage = UserConfig::getInstance().get<std::string>("/core/language", "system");
    languageChoice->set_active_id(savedLanguage == "en" || savedLanguage == "zh_CN"
                                      ? savedLanguage : "system");
    auto languageStatus = Gtk::make_managed<Gtk::Label>();
    languageStatus->set_halign(Gtk::Align::START);
    languageChoice->signal_changed().connect([languageChoice, languageStatus]() {
        const auto selected = languageChoice->get_active_id();
        if (selected.empty()) return;
        UserConfig::getInstance().set<std::string>("/core/language", selected);
        UserConfig::getInstance().save();
        languageStatus->set_text(_("Restart the application to apply the language."));
    });
    languageBox->append(*languageChoice);
    languageBox->append(*languageStatus);
    languageFrame->set_child(*languageBox);

    auto oscFrame = Gtk::make_managed<Gtk::Frame>(_("OSC Server"));
    auto oscGrid = Gtk::make_managed<Gtk::Grid>();
    oscGrid->set_row_spacing(10);
    oscGrid->set_column_spacing(10);

    auto enableOsc = Gtk::make_managed<Gtk::CheckButton>(_("Enable"));
    enableOsc->set_active(UserConfig::getInstance().get<bool>(modePrefix + "/oscServer/enabled", false));
    enableOsc->signal_toggled().connect([enableOsc, modePrefix]() {
        UserConfig::getInstance().set<bool>(modePrefix + "/oscServer/enabled", enableOsc->get_active());
        OSCServer::getInstance().restart();
        UserConfig::getInstance().save();
    });
    auto allowLan = Gtk::make_managed<Gtk::CheckButton>(_("Allow LAN Access"));
    allowLan->set_active(UserConfig::getInstance().get<bool>(modePrefix + "/oscServer/lan", false));
    allowLan->signal_toggled().connect([allowLan, modePrefix]() {
        UserConfig::getInstance().set<bool>(modePrefix + "/oscServer/lan", allowLan->get_active());
        OSCServer::getInstance().restart();
        UserConfig::getInstance().save();
    });

    auto portLabel = Gtk::make_managed<Gtk::Label>(_("Port"));
    auto portEntry = Gtk::make_managed<Gtk::Entry>();
    portEntry->set_text(std::to_string(UserConfig::getInstance().get<int>(modePrefix + "/oscServer/port", 8999)));
    portEntry->set_width_chars(6);
    auto portStatus = Gtk::make_managed<Gtk::Label>();
    portStatus->set_halign(Gtk::Align::START);
    portStatus->set_css_classes({"error"});

    auto portEntryController = Gtk::EventControllerFocus::create();
    portEntryController->signal_leave().connect([portEntry, portStatus, modePrefix]() {
        try {
            const std::string text = portEntry->get_text();
            std::size_t parsedLength = 0;
            const int port = std::stoi(text, &parsedLength);
            if (parsedLength != text.size() || port < 1 || port > 65535)
                throw std::invalid_argument("port");
            portStatus->set_text("");
            UserConfig::getInstance().set<int>(modePrefix + "/oscServer/port", port);
            OSCServer::getInstance().restart();
            UserConfig::getInstance().save();
        } catch (...) {
            portStatus->set_text(_("Enter a port from 1 to 65535"));
        }
    });
    portEntry->add_controller(portEntryController);

    oscGrid->attach(*enableOsc, 0, 0, 2, 1);
    oscGrid->attach(*allowLan, 0, 1, 2, 1);
    oscGrid->attach(*portLabel, 0, 2);
    oscGrid->attach(*portEntry, 1, 2);
    oscGrid->attach(*portStatus, 1, 3);

    oscFrame->set_child(*oscGrid);

    auto optiTrackFrame = Gtk::make_managed<Gtk::Frame>(_("OptiTrack"));
    auto optiTrackBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    optiTrackBox->set_margin(8);
    auto optiTrackEnabled = Gtk::make_managed<Gtk::CheckButton>(_("Send gloves to OptiTrack"));
    optiTrackEnabled->set_active(
        UserConfig::getInstance().get<bool>(modePrefix + "/optitrack/enabled", false));
    optiTrackEnabled->signal_toggled().connect([optiTrackEnabled, modePrefix]() {
        auto &config = UserConfig::getInstance();
        config.set<bool>(modePrefix + "/optitrack/enabled", optiTrackEnabled->get_active());
        config.save();
    });
    optiTrackBox->append(*optiTrackEnabled);
    optiTrackFrame->set_child(*optiTrackBox);

    // 组装界面
    mainBox.append(*modeFrame);
    mainBox.append(*languageFrame);
    auto vrFrame = Gtk::make_managed<Gtk::Frame>(_("SteamVR Settings"));
    vrFrame->set_child(*Gtk::make_managed<SteamVRSettingsBox>(modePrefix, std::move(onSteamVRChanged)));
    mainBox.append(*vrFrame);
    mainBox.append(*oscFrame);
    mainBox.append(*optiTrackFrame);
}

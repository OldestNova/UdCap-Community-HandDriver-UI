//
// Created by max_3 on 25-6-23.
//

#include "PreferenceDialog.h"
#include "components/UserConfig.h"
#include "config.h"
#include "components/OSCServer.h"


PreferenceDialog::PreferenceDialog() {
    set_title(_("Preferences"));
    set_default_size(400, 300);

    mainBox.set_orientation(Gtk::Orientation::VERTICAL);
    mainBox.set_margin(15);
    mainBox.set_spacing(20);

    auto modeFrame = Gtk::make_managed<Gtk::Frame>(_("Mode"));
    auto modeBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    auto switchButton = Gtk::make_managed<Gtk::Button>("");
    if (static_cast<UdCapDriverType>(UserConfig::getInstance().get<int>("/core/driverType", 0)) == UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER) {
        switchButton->set_label(_("Switch to Advanced Mode"));
        switchButton->signal_clicked().connect([this]() {
            alertDialog = Gtk::AlertDialog::create(_("Switching to advanced mode will require a restart. \nDo you want to continue?"));
            alertDialog->set_buttons({ "No", "Yes" });
            alertDialog->set_default_button(1);
            alertDialog->set_cancel_button(0);
            alertDialog->choose(*this, [this](Glib::RefPtr<Gio::AsyncResult> &result) {
                int r = alertDialog->choose_finish(result);
                if (r == 1) {
                    UserConfig::getInstance().set<int>("/core/driverType", static_cast<int>(UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE));
                    UserConfig::getInstance().save();
                    this->get_application()->quit();
                }
            });
        });
    } else {
        switchButton->set_label(_("Switch to Simple Mode"));
        switchButton->signal_clicked().connect([this]() {
            alertDialog = Gtk::AlertDialog::create(_("Switching to simple mode will require a restart. \nDo you want to continue?"));
            alertDialog->set_buttons({ "No", "Yes" });
            alertDialog->set_default_button(1);
            alertDialog->set_cancel_button(0);
            alertDialog->choose(*this, [this](Glib::RefPtr<Gio::AsyncResult> &result) {
                int r = alertDialog->choose_finish(result);
                if (r == 1) {
                    UserConfig::getInstance().set<int>("/core/driverType", static_cast<int>(UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER));
                    UserConfig::getInstance().save();
                    this->get_application()->quit();
                }
            });
        });
    }

    modeBox->append(*switchButton);
    modeFrame->set_child(*modeBox);

    auto oscFrame = Gtk::make_managed<Gtk::Frame>(_("OSC Server"));
    auto oscGrid = Gtk::make_managed<Gtk::Grid>();
    oscGrid->set_row_spacing(10);
    oscGrid->set_column_spacing(10);

    auto enableOsc = Gtk::make_managed<Gtk::CheckButton>(_("Enable"));
    enableOsc->set_active(UserConfig::getInstance().get<bool>("/core/oscServer/enabled", false));
    enableOsc->signal_toggled().connect([this, enableOsc]() {
        UserConfig::getInstance().set<bool>("/core/oscServer/enabled", enableOsc->get_active());
        updateOSCServer();
    });
    auto allowLan = Gtk::make_managed<Gtk::CheckButton>(_("Allow LAN Access"));
    allowLan->set_active(UserConfig::getInstance().get<bool>("/core/oscServer/lan", false));
    allowLan->signal_toggled().connect([this, allowLan]() {
        UserConfig::getInstance().set<bool>("/core/oscServer/lan", allowLan->get_active());
        updateOSCServer();
    });

    auto portLabel = Gtk::make_managed<Gtk::Label>(_("Port"));
    auto portEntry = Gtk::make_managed<Gtk::Entry>();
    portEntry->set_text(std::to_string(UserConfig::getInstance().get<int>("/core/oscServer/port", 8999)));
    portEntry->set_width_chars(6);

    auto portEntryController = Gtk::EventControllerFocus::create();
    portEntryController->signal_leave().connect([this, portEntry]() {
        try {
            int port = std::stoi(portEntry->get_text());
            if (port < 1 || port > 65535) {
                portEntry->set_text("8999");
            }
        } catch (...) {
            portEntry->set_text("8999");
        }
        UserConfig::getInstance().set<int>("/core/oscServer/port", std::stoi(portEntry->get_text()));
        updateOSCServer();
    });
    portEntry->add_controller(portEntryController);

    oscGrid->attach(*enableOsc, 0, 0, 2, 1);
    oscGrid->attach(*allowLan, 0, 1, 2, 1);
    oscGrid->attach(*portLabel, 0, 2);
    oscGrid->attach(*portEntry, 1, 2);

    oscFrame->set_child(*oscGrid);

    // 组装界面
    mainBox.append(*modeFrame);
    mainBox.append(*oscFrame);
    set_child(mainBox);
}

void PreferenceDialog::updateOSCServer() {
    OSCServer::getInstance().restart();
    UserConfig::getInstance().save();
}
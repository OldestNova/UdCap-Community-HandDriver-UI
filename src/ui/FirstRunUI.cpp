//
// Created by max_3 on 2025/6/19.
//

#include "FirstRunUI.h"
#include "../components/UserConfig.h"
#include "../config.h"

void writeDefault() {
    UserConfig::getInstance().set<bool>("/consumer/vmc/enabled", false);
    UserConfig::getInstance().set<std::string>("/consumer/vmc/host", "127.0.0.1");
    UserConfig::getInstance().set<int>("/consumer/vmc/port", 39540);
    UserConfig::getInstance().set<bool>("/consumer/osc/enabled", false);
    UserConfig::getInstance().set<std::string>("/consumer/osc/host", "127.0.0.1");
    UserConfig::getInstance().set<int>("/consumer/osc/port", 9000);
    UserConfig::getInstance().set<bool>("/consumer/udCapQingTong/enabled", false);
    UserConfig::getInstance().set<std::string>("/consumer/udCapQingTong/host", "127.0.0.1");
    UserConfig::getInstance().set<int>("/consumer/udCapQingTong/port", 6666);
    UserConfig::getInstance().set<bool>("/consumer/vr/enabled", false);
}

FirstRunUI::FirstRunUI(std::shared_ptr<bool> resultFlag) {
    set_title(_("First Time Config"));
    set_default_size(600, 250);

    mainBox.set_orientation(Gtk::Orientation::VERTICAL);
    mainBox.set_margin(20);
    mainBox.set_spacing(20);

    infoLabel.set_markup(_("Welcome to use UdCap Community Driver. \n\n"
                          "You can choose to use the <b>simple mode</b> (aka Consumer Driver) or <b>advanced mode</b> (aka Enterprise Driver). \n"
                          "Simple mode is suitable for most users, it automatically find a pair of gloves and connect to them. \n"
                          "Advanced mode allows you to manually select the gloves and configure them, support multiple gloves and "
                          "it can pair by yourself. \n"
                          "Two modes are supported by the same driver, it support consumer glove and enterprise glove in every mode. \nyou can switch between them at any time. \nVMC, VRChat OSC, UdCap's QingTong UDP "
                          "Broadcast and SteamVR(WIP) available in both modes."));
    infoLabel.set_wrap(true);
    infoLabel.set_justify(Gtk::Justification::LEFT);
    infoLabel.set_halign(Gtk::Align::CENTER);

    buttonBox.set_orientation(Gtk::Orientation::HORIZONTAL);
    buttonBox.set_spacing(20);
    buttonBox.set_halign(Gtk::Align::CENTER);

    buttonSimple.set_label(_("Simple"));
    buttonAdvanced.set_label(_("Advanced"));

    buttonSimple.signal_clicked().connect([this, resultFlag]() {
        UserConfig::getInstance().set<int>("/core/configVersion", 1);
        UserConfig::getInstance().set<int>("/core/driverType", static_cast<int>(UdCapDriverType::UD_CAP_DRIVER_TYPE_CONSUMER));
        writeDefault();
        UserConfig::getInstance().save();
        *resultFlag = true;
        this->close();
    });

    buttonAdvanced.signal_clicked().connect([this, resultFlag]() {
        UserConfig::getInstance().set<int>("/core/configVersion",1);
        UserConfig::getInstance().set<int>("/core/driverType", static_cast<int>(UdCapDriverType::UD_CAP_DRIVER_TYPE_ENTERPRISE));
        writeDefault();
        UserConfig::getInstance().save();
        *resultFlag = true;
        this->close();
    });

    buttonBox.append(buttonSimple);
    buttonBox.append(buttonAdvanced);

    mainBox.append(infoLabel);
    mainBox.append(buttonBox);

    set_child(mainBox);
}
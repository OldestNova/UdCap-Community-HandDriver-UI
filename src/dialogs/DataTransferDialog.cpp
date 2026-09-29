//
// Created by max_3 on 2025/6/17.
//

#include "DataTransferDialog.h"
#include "components/UserConfig.h"
#include <regex>
#include <iostream>
#include <sstream>

class FeatureRow : public Gtk::Box {
public:
    FeatureRow(bool default_toggle ,const Glib::ustring& name, const Glib::ustring& default_ip, int default_port,
               std::function<void(bool,const std::string&, int)> _on_change)
            : Gtk::Box(Gtk::Orientation::HORIZONTAL, 10), grid(),
              on_change(_on_change) {

        set_margin(5);

        toggle.set_label(name);
        toggle.set_active(default_toggle);
        toggle.signal_toggled().connect([this]() {
            update_state();
        });

        ip_entry.set_text(default_ip);
        ip_entry.set_width_chars(15);
        auto ip_entry_controller = Gtk::EventControllerFocus::create();
        ip_entry_controller->signal_leave().connect([this]() {
            update_state();
        });
        ip_entry.add_controller(ip_entry_controller);

        port_entry.set_text(std::to_string(default_port));
        port_entry.set_width_chars(6);
        auto port_entry_controller = Gtk::EventControllerFocus::create();
        port_entry_controller->signal_leave().connect([this]() {
            update_state();
        });
        port_entry.add_controller(port_entry_controller);

        warning_label.set_visible(false);
        warning_label.set_halign(Gtk::Align::START);
        warning_label.set_margin_start(10);
        warning_label.set_css_classes({"error"});

        grid.set_column_spacing(10);
        grid.set_row_spacing(5);
        grid.set_hexpand(true);

        toggle.set_hexpand(true);
        ip_entry.set_hexpand(true);
        port_entry.set_hexpand(true);
        warning_label.set_hexpand(true);

        grid.attach(toggle, 0, 0, 1, 1);
        grid.attach(ip_entry, 1, 0, 1, 1);
        grid.attach(port_entry, 2, 0, 1, 1);
        grid.attach(warning_label, 1, 1, 2, 1);

        grid.set_column_homogeneous(true);

        append(grid);
    }

private:
    Gtk::Grid grid;
    Gtk::CheckButton toggle;
    Gtk::Entry ip_entry;
    Gtk::Entry port_entry;
    Gtk::Label warning_label;
    std::function<void(bool, const std::string&, int)> on_change;

    bool validate_ip(const std::string& ip) {
        std::regex ip_regex(R"((\d{1,3}\.){3}\d{1,3})");
        if (!std::regex_match(ip, ip_regex)) return false;

        std::istringstream iss(ip);
        std::string token;
        int i = 0;
        int parts[4];
        while (std::getline(iss, token, '.')) {
            int val = std::stoi(token);
            if (val < 0 || val > 255) return false;
            parts[i++] = val;
        }
        if (parts[0] >= 224 && parts[0] <= 239) return true; // multicast
        if (parts[3] == 255) return false; // broadcast
        return true;
    }

    void update_state() {
        std::string ip = ip_entry.get_text();
        std::string port_text = port_entry.get_text();
        bool valid = true;

        if (ip.empty() || !validate_ip(ip)) {
            warning_label.set_text(_("IP Address not valid"));
            valid = false;
        }

        int port = 0;
        try {
            std::size_t parsedLength = 0;
            int parsed = std::stoi(port_text, &parsedLength);
            if (parsedLength != port_text.size()) throw std::invalid_argument("port");
            if (parsed <= 0 || parsed > 65535) throw std::out_of_range("port");
            port = parsed;
        } catch (...) {
            warning_label.set_text(_("Invalid port"));
            valid = false;
        }

        warning_label.set_visible(!valid);

        if (valid) on_change(toggle.get_active(), ip, port);
    }
};


DataTransferDialog::DataTransferDialog(std::string configPrefix,
        std::function<void(bool enable, std::string host, uint16_t port)> _vmcCallback,
                                       std::function<void(bool enable, std::string host, uint16_t port)> _oscCallback,
                                       std::function<void(bool enable, std::string host, uint16_t port)> _broadcastCallback,
                                       std::function<void(bool enable)> _vrCallback,
                                       bool showVmc, bool showVr):
        vmcCallback(_vmcCallback),
        oscCallback(_oscCallback),
        broadcastCallback(_broadcastCallback),
        vrCallback(_vrCallback)
{
    set_title(showVmc ? _("Data Transfer Settings") : _("Selected Pair OSC and UdCapQT"));
    set_default_size(550, 260);

    vbox.set_orientation(Gtk::Orientation::VERTICAL);
    vbox.set_spacing(10);
    vbox.set_margin(10);

    if (showVmc) {
        auto row1 = Gtk::make_managed<FeatureRow>(UserConfig::getInstance().get<bool>(configPrefix + "/vmc/enabled", false),
                                                  "VMC",
                                                  UserConfig::getInstance().get<std::string>(configPrefix + "/vmc/host", "127.0.0.1"),
                                                  UserConfig::getInstance().get<int>(configPrefix + "/vmc/port", 39540), vmcCallback);
        vbox.append(*row1);
    }

    auto row2 = Gtk::make_managed<FeatureRow>(UserConfig::getInstance().get<bool>(configPrefix + "/osc/enabled", false),
                                              "VRChat OSC",
                                              UserConfig::getInstance().get<std::string>(configPrefix + "/osc/host", "127.0.0.1"),
                                              UserConfig::getInstance().get<int>(configPrefix + "/osc/port", 9000)
                                              ,oscCallback);

    auto row3 = Gtk::make_managed<FeatureRow>(UserConfig::getInstance().get<bool>(configPrefix + "/udCapQingTong/enabled", false), _("QingTong UDP"),
                                              UserConfig::getInstance().get<std::string>(configPrefix + "/udCapQingTong/host", "127.0.0.1"),
                                              UserConfig::getInstance().get<int>(configPrefix + "/udCapQingTong/port", 6666)
                                              , broadcastCallback);

    vbox.append(*row2);
    vbox.append(*row3);

    if (showVr) {
        auto vrToggle = Gtk::make_managed<Gtk::CheckButton>(_("SteamVR controllers"));
        vrToggle->set_active(UserConfig::getInstance().get<bool>(configPrefix + "/vr/enabled", false));
        vrToggle->signal_toggled().connect([this, vrToggle]() {
            vrCallback(vrToggle->get_active());
        });
        vbox.append(*vrToggle);
    }

    set_child(vbox);
}

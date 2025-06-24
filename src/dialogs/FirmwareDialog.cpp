//
// Created by max_3 on 2025/6/19.
//

#include "FirmwareDialog.h"
#include "../components/Placeholder.h"
FirmwareDialog::FirmwareDialog(std::vector<std::shared_ptr<UdCapV1Core>> _cores): cores(_cores) {
    set_title(_("Firmware"));
    set_default_size(800, 300);
    set_resizable(false);

    mainBox.set_orientation(Gtk::Orientation::VERTICAL);
    mainBox.set_spacing(10);
    mainBox.set_margin(10);

    for (const auto& core : cores) {
        auto grid = Gtk::make_managed<Gtk::Grid>();
        grid->set_column_spacing(10);
        grid->set_row_spacing(4);

        auto statusIcon = Gtk::make_managed<Gtk::Image>();
        auto serialPrefix = Gtk::make_managed<Gtk::Label>(_("Serial: "));
        auto serialValue = Gtk::make_managed<Gtk::Label>();
        auto channelLabel = Gtk::make_managed<Gtk::Label>(_("Channel:"));
        auto channelEntry = Gtk::make_managed<Gtk::Entry>();
        auto setButton = Gtk::make_managed<Gtk::Button>(_("Set"));
        auto resetButton = Gtk::make_managed<Gtk::Button>(_("Reset"));
        auto firmwarePrefix = Gtk::make_managed<Gtk::Label>(_("Firmware: "));
        auto firmwareValue = Gtk::make_managed<Gtk::Label>();
        auto pairButton = Gtk::make_managed<Gtk::Button>(_("Pair"));
        channelEntry->set_width_chars(4);
        if (core) {
            statusIcon->set(create_placeholder_green_image());
            serialValue->set_text(core->getUDCapSerial());
            firmwareValue->set_text(_("Waiting"));
            channelEntry->set_text("");
        } else {
            statusIcon->set(create_placeholder_image());
            serialValue->set_text(_("N/A"));
            firmwareValue->set_text(_("N/A"));
            channelEntry->set_text("");
            pairButton->set_sensitive(false);
        }
        pairButton->signal_clicked().connect([this, core]() {
            if (core) {
                pairDialog = std::make_unique<PairDialog>(core);
                pairDialog->set_transient_for(*this);
                pairDialog->set_modal(true);
                pairDialog->show();
            }
        });

        grid->attach(*statusIcon,    0, 0);
        grid->attach(*serialPrefix,  1, 0);
        grid->attach(*serialValue,   2, 0);
        grid->attach(*channelLabel,  3, 0);
        grid->attach(*channelEntry,  4, 0);
        grid->attach(*setButton,     5, 0);
        grid->attach(*resetButton,   6, 0);
        grid->attach(*firmwarePrefix,7, 0);
        grid->attach(*firmwareValue, 8, 0);
        grid->attach(*pairButton,   9, 0);
        mainBox.append(*grid);

        if (core) {
            unlisten.push_back(core->listen([firmwareValue, channelEntry](std::shared_ptr<UdCapV1MCUPacket> packet){
                Glib::MainContext::get_default()->invoke([firmwareValue, channelEntry, packet](){
                    if (packet->commandType == CommandType::CMD_FW_VERSION) {
                        firmwareValue->set_text(packet->fwVersion);
                    } else if (packet->commandType == CommandType::CMD_GET_CHANNEL) {
                        channelEntry->set_text(std::to_string(packet->channel));
                    } else if (packet->commandType == CommandType::CMD_SET_CHANNEL) {
                        channelEntry->set_text(std::to_string(packet->channelResult));
                    }
                    return false;
                });
            }));
            core->mcuGetChannel();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            core->mcuGetFirmwareVersion();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            setButton->signal_clicked().connect([core, channelEntry]() {
                std::string channelText = channelEntry->get_text();
                if (!channelText.empty()) {
                    try {
                        int channel = std::stoi(channelText);
                        core->mcuSetChannel(channel);
                    } catch (const std::invalid_argument&) {
                        // Handle invalid input
                    }
                }
            });
            resetButton->signal_clicked().connect([core]() {
                core->mcuReset();
            });
        } else {
            unlisten.push_back([]() {
                // No core to listen to, do nothing
            });
        }
    }

    scrolledWindow.set_child(mainBox);
    scrolledWindow.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    set_child(scrolledWindow);

    mTimerConnection = Glib::signal_timeout().connect_seconds(
            sigc::mem_fun(*this, &FirmwareDialog::onTimer), 2);
}

bool FirmwareDialog::onTimer() {

    return false;
}

FirmwareDialog::~FirmwareDialog() {
    // Cleanup if necessary
    mTimerConnection.disconnect();
    for (auto& unlistenFunc : unlisten) {
        unlistenFunc();
    }
}
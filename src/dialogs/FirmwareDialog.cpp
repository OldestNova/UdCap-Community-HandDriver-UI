//
// Created by max_3 on 2025/6/19.
//

#include "FirmwareDialog.h"
#include "../components/Placeholder.h"
#include <chrono>

namespace {
struct ChannelChangeState {
    bool pending = false;
    uint8_t requested = 0;
    std::chrono::steady_clock::time_point started;
    sigc::connection poll;
};
}
FirmwarePanel::FirmwarePanel() {
    mainBox.set_orientation(Gtk::Orientation::VERTICAL);
    mainBox.set_spacing(10);
    mainBox.set_margin(10);
    set_child(mainBox);
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
}

void FirmwarePanel::setCores(std::vector<std::shared_ptr<UdCapV1Core>> newCores) {
    *alive = false;
    for (auto &stop : unlisten) if (stop) stop();
    unlisten.clear();
    pairDialog.reset();
    while (auto *child = mainBox.get_first_child()) mainBox.remove(*child);
    alive = std::make_shared<std::atomic_bool>(true);
    cores = std::move(newCores);
    if (cores.empty()) {
        mainBox.append(*Gtk::make_managed<Gtk::Label>(_("No receivers found")));
        return;
    }

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
        auto actionStatus = Gtk::make_managed<Gtk::Label>();
        actionStatus->set_halign(Gtk::Align::START);
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
            setButton->set_sensitive(false);
            resetButton->set_sensitive(false);
        }
        pairButton->signal_clicked().connect([this, core]() {
            if (core) {
                pairDialog = std::make_unique<PairDialog>(core);
                if (auto *window = dynamic_cast<Gtk::Window *>(get_root()))
                    pairDialog->set_transient_for(*window);
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
        grid->attach(*actionStatus, 0, 1, 10, 1);
        mainBox.append(*grid);

        if (core) {
            auto channelChange = std::make_shared<ChannelChangeState>();
            const std::weak_ptr<std::atomic_bool> life = alive;
            unlisten.push_back(core->listen([firmwareValue, channelEntry, actionStatus, channelChange, life](std::shared_ptr<UdCapV1MCUPacket> packet){
                Glib::MainContext::get_default()->invoke([firmwareValue, channelEntry, actionStatus, channelChange, packet, life](){
                    const auto current = life.lock();
                    if (!current || !current->load()) return false;
                    if (packet->commandType == CommandType::CMD_FW_VERSION) {
                        firmwareValue->set_text(packet->fwVersion);
                    } else if (packet->commandType == CommandType::CMD_GET_CHANNEL) {
                        channelEntry->set_text(std::to_string(packet->channel));
                        if (channelChange->pending && packet->channel == channelChange->requested) {
                            channelChange->pending = false;
                            channelChange->poll.disconnect();
                            actionStatus->set_text(_("Channel changed"));
                        }
                    } else if (packet->commandType == CommandType::CMD_SET_CHANNEL) {
                        if (channelChange->pending && packet->channel == channelChange->requested) {
                            channelChange->pending = false;
                            channelChange->poll.disconnect();
                            actionStatus->set_text(packet->channelResult == 2
                                ? _("Channel changed")
                                : Glib::ustring::compose(_("Channel change failed (code %1)"), packet->channelResult));
                        }
                    }
                    return false;
                });
            }));
            try {
                core->mcuGetChannel();
                core->mcuGetFirmwareVersion();
            } catch (const std::exception &e) {
                actionStatus->set_text(e.what());
            }

            setButton->signal_clicked().connect([core, channelEntry, actionStatus, channelChange, life]() {
                const std::string channelText = channelEntry->get_text();
                try {
                    std::size_t parsedLength = 0;
                    const int channel = std::stoi(channelText, &parsedLength);
                    if (parsedLength != channelText.size() || channel < 0 || channel > 255)
                        throw std::invalid_argument("channel");
                    core->mcuSetChannel(channel);
                    channelChange->poll.disconnect();
                    channelChange->requested = static_cast<uint8_t>(channel);
                    channelChange->started = std::chrono::steady_clock::now();
                    channelChange->pending = true;
                    actionStatus->set_text(_("Changing channel..."));
                    channelChange->poll = Glib::signal_timeout().connect([core, actionStatus, channelChange, life]() {
                        const auto current = life.lock();
                        if (!current || !current->load() || !channelChange->pending) return false;
                        if (std::chrono::steady_clock::now() - channelChange->started >= std::chrono::seconds(10)) {
                            channelChange->pending = false;
                            actionStatus->set_text(_("Channel change timed out"));
                            return false;
                        }
                        try { core->mcuGetChannel(); }
                        catch (const std::exception &e) {
                            channelChange->pending = false;
                            actionStatus->set_text(e.what());
                            return false;
                        }
                        return true;
                    }, 300);
                } catch (const std::invalid_argument &) {
                    actionStatus->set_text(_("Channel must be from 0 to 255"));
                } catch (const std::out_of_range &) {
                    actionStatus->set_text(_("Channel must be from 0 to 255"));
                } catch (const std::exception &e) {
                    actionStatus->set_text(e.what());
                }
            });
            resetButton->signal_clicked().connect([core, actionStatus]() {
                try {
                    core->mcuReset();
                    actionStatus->set_text("");
                } catch (const std::exception &e) {
                    actionStatus->set_text(e.what());
                }
            });
        } else {
            unlisten.push_back([]() {
                // No core to listen to, do nothing
            });
        }
    }

}

FirmwarePanel::~FirmwarePanel() {
    *alive = false;
    for (auto& unlistenFunc : unlisten) {
        unlistenFunc();
    }
}

FirmwareDialog::FirmwareDialog(std::vector<std::shared_ptr<UdCapV1Core>> cores) {
    set_title(_("Firmware"));
    set_default_size(800, 300);
    set_resizable(false);
    panel.setCores(std::move(cores));
    set_child(panel);
}

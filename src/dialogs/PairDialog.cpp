//
// Created by max_3 on 2025/6/19.
//

#include "PairDialog.h"

PairDialog::PairDialog(std::shared_ptr<UdCapV1Core> _core): core(_core) {
    set_title(_("Pairing"));
    set_default_size(300, 120);
    set_resizable(false);

    mainBox.set_orientation(Gtk::Orientation::VERTICAL);
    mainBox.set_spacing(15);
    mainBox.set_margin(20);
    mainBox.set_valign(Gtk::Align::CENTER);
    mainBox.set_halign(Gtk::Align::CENTER);

    label.set_text(_("Pairing..."));
    label.set_justify(Gtk::Justification::CENTER);
    label.set_halign(Gtk::Align::CENTER);

    progressBar.set_show_text(false);
    progressBar.pulse();
    progressBar.set_pulse_step(0.05);
    progressBar.set_hexpand();

    closeButton.set_label(_("Close"));
    closeButton.set_visible(false);
    closeButton.signal_clicked().connect([this]() {
        this->close();
    });

    // 定时更新进度条的脉冲动画
    Glib::signal_timeout().connect(
            sigc::mem_fun(*this, &PairDialog::onTimeout), 100);

    mainBox.append(label);
    mainBox.append(progressBar);

    set_child(mainBox);

    conn = core->listen([this](std::shared_ptr<UdCapV1MCUPacket> packet) {
        if (packet->commandType == CommandType::CMD_PAIRING) {
            if (packet->pairing) {
                this->close();
            } else {
                label.set_text(_("Pairing failed"));
                progressBar.set_visible(false);
                closeButton.set_visible(true);
            }
        }
    });
    core->mcuStartPairing();
}
PairDialog::~PairDialog() {
    if (conn) {
        conn();
    }
    core->mcuStopPairing();
}

bool PairDialog::onTimeout() {
    progressBar.pulse();
    return true;
}
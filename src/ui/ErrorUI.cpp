//
// Created by max_3 on 2025/6/19.
//

#include "ErrorUI.h"
ErrorUI::ErrorUI(std::string errorMessage) {
    set_title(_("Error"));
    set_default_size(400, 250);

    mainBox.set_orientation(Gtk::Orientation::VERTICAL);
    mainBox.set_margin(20);
    mainBox.set_spacing(20);

    infoLabel.set_markup(errorMessage);
    infoLabel.set_wrap(true);
    infoLabel.set_justify(Gtk::Justification::CENTER);
    infoLabel.set_halign(Gtk::Align::CENTER);

    buttonBox.set_orientation(Gtk::Orientation::HORIZONTAL);
    buttonBox.set_spacing(20);
    buttonBox.set_halign(Gtk::Align::CENTER);

    buttonExit.set_label(_("Exit"));

    buttonExit.signal_clicked().connect([this]() {
        get_application()->quit();
    });

    buttonBox.append(buttonExit);

    mainBox.append(infoLabel);
    mainBox.append(buttonBox);

    set_child(mainBox);
}
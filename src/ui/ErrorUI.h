//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_ERRORUI_H
#define UDCAPCOMMUNITYDRIVERUI_ERRORUI_H

#include <cstdint>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>

class ErrorUI: public Gtk::Window {
public:
    ErrorUI(std::string errorMessage);
private:
    Gtk::Box mainBox;
    Gtk::Label infoLabel;
    Gtk::Box buttonBox;
    Gtk::Button buttonExit;
};


#endif //UDCAPCOMMUNITYDRIVERUI_ERRORUI_H

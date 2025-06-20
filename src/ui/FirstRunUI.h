//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_FIRSTRUNUI_H
#define UDCAPCOMMUNITYDRIVERUI_FIRSTRUNUI_H
#include <cstdint>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>

class FirstRunUI: public Gtk::ApplicationWindow {
public:
    FirstRunUI(std::shared_ptr<bool> resultFlag);
private:
    Gtk::Box mainBox;
    Gtk::Label infoLabel;
    Gtk::Box buttonBox;
    Gtk::Button buttonSimple;
    Gtk::Button buttonAdvanced;
};


#endif //UDCAPCOMMUNITYDRIVERUI_FIRSTRUNUI_H

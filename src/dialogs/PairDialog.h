//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_PAIRUI_H
#define UDCAPCOMMUNITYDRIVERUI_PAIRUI_H

#include <cstdint>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <UdCapV1Core.h>
class PairDialog: public Gtk::Window {
public:
    explicit PairDialog(std::shared_ptr<UdCapV1Core> _core);
    ~PairDialog();
private:
    std::shared_ptr<UdCapV1Core> core;
    std::function<void()> conn;
    Gtk::Box mainBox;
    Gtk::Label label;
    Gtk::Button closeButton;
    Gtk::ProgressBar progressBar;

    bool onTimeout();
};


#endif //UDCAPCOMMUNITYDRIVERUI_PAIRUI_H

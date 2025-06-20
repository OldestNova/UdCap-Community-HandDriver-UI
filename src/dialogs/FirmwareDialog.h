//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_FIRMWAREDIALOG_H
#define UDCAPCOMMUNITYDRIVERUI_FIRMWAREDIALOG_H

#include <cstdint>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <UdCapV1Core.h>
#include "PairDialog.h"

class FirmwareDialog: public Gtk::Window {
public:
    explicit FirmwareDialog(std::vector<std::shared_ptr<UdCapV1Core>> _cores);
    ~FirmwareDialog();
private:
    std::vector<std::shared_ptr<UdCapV1Core>> cores;
    std::vector<std::function<void()>> unlisten;
    Gtk::Box mainBox;
    Gtk::ScrolledWindow scrolledWindow;
    sigc::connection mTimerConnection;
    std::unique_ptr<PairDialog> pairDialog;
    bool onTimer();
};


#endif //UDCAPCOMMUNITYDRIVERUI_FIRMWAREDIALOG_H

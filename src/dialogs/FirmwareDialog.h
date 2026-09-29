//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_FIRMWAREDIALOG_H
#define UDCAPCOMMUNITYDRIVERUI_FIRMWAREDIALOG_H

#include <cstdint>
#include <atomic>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <UdCapV1Core.h>
#include "PairDialog.h"

class FirmwarePanel: public Gtk::ScrolledWindow {
public:
    FirmwarePanel();
    ~FirmwarePanel() override;
    void setCores(std::vector<std::shared_ptr<UdCapV1Core>> newCores);
private:
    std::vector<std::shared_ptr<UdCapV1Core>> cores;
    std::vector<std::function<void()>> unlisten;
    Gtk::Box mainBox;
    std::unique_ptr<PairDialog> pairDialog;
    std::shared_ptr<std::atomic_bool> alive = std::make_shared<std::atomic_bool>(true);
};

class FirmwareDialog: public Gtk::Window {
public:
    explicit FirmwareDialog(std::vector<std::shared_ptr<UdCapV1Core>> cores);
private:
    FirmwarePanel panel;
};


#endif //UDCAPCOMMUNITYDRIVERUI_FIRMWAREDIALOG_H

//
// Created by max_3 on 2025/6/19.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_PAIRUI_H
#define UDCAPCOMMUNITYDRIVERUI_PAIRUI_H

#include <cstdint>
#include <atomic>
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
    sigc::connection timeoutConnection;
    std::shared_ptr<std::atomic_bool> alive = std::make_shared<std::atomic_bool>(true);
    bool pairingStarted = false;
    Gtk::Box mainBox;
    Gtk::Label label;
    Gtk::Button closeButton;
    Gtk::ProgressBar progressBar;

    void stopPairing();
    bool onTimeout();
};


#endif //UDCAPCOMMUNITYDRIVERUI_PAIRUI_H

//
// Created by max_3 on 2025/6/17.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_DATATRANSFERDIALOG_H
#define UDCAPCOMMUNITYDRIVERUI_DATATRANSFERDIALOG_H

#include <cstdint>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <UdCapV1Core.h>

class DataTransferDialog: public Gtk::Window {
public:
    explicit DataTransferDialog(std::string configPrefix,
                                std::function<void(bool enable, std::string host, uint16_t port)> _vmcCallback,
                                std::function<void(bool enable, std::string host, uint16_t port)> _oscCallback,
                                std::function<void(bool enable, std::string host, uint16_t port)> _broadcastCallback,
                                std::function<void(bool enable)> _vrCallback,
                                bool showVmc = true, bool showVr = true);
private:
    std::function<void(bool enable, std::string host, uint16_t port)> vmcCallback;
    std::function<void(bool enable, std::string host, uint16_t port)> oscCallback;
    std::function<void(bool enable, std::string host, uint16_t port)> broadcastCallback;
    std::function<void(bool enable)> vrCallback;
    Gtk::Box vbox;
};


#endif //UDCAPCOMMUNITYDRIVERUI_DATATRANSFERDIALOG_H

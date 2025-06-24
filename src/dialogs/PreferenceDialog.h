//
// Created by max_3 on 25-6-23.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_PREFERENCEDIALOG_H
#define UDCAPCOMMUNITYDRIVERUI_PREFERENCEDIALOG_H
#include <cstdint>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>

class PreferenceDialog: public Gtk::Window {
public:
    explicit PreferenceDialog();
    ~PreferenceDialog() override = default;
private:
    Gtk::Box mainBox;
    std::shared_ptr<Gtk::AlertDialog> alertDialog;
    void updateOSCServer();
};


#endif //UDCAPCOMMUNITYDRIVERUI_PREFERENCEDIALOG_H

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

void populatePreferences(Gtk::Box &box, Gtk::Window &owner, std::function<void()> onSteamVRChanged = {});

class PreferenceDialog: public Gtk::Window {
public:
    explicit PreferenceDialog(std::function<void()> onSteamVRChanged = {});
    ~PreferenceDialog() override = default;
private:
    Gtk::Box mainBox;
};


#endif //UDCAPCOMMUNITYDRIVERUI_PREFERENCEDIALOG_H

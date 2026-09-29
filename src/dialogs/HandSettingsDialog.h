#ifndef UDCAP_HAND_SETTINGS_DIALOG_H
#define UDCAP_HAND_SETTINGS_DIALOG_H

#include <gtkmm.h>
#include <UdCapV1Core.h>
#include <memory>
#include <vector>

class HandSettingsDialog : public Gtk::Window {
public:
    explicit HandSettingsDialog(const std::vector<std::shared_ptr<UdCapV1Core>> &cores,
                                std::vector<std::string> algorithmKeys = {});

private:
    Gtk::Box content{Gtk::Orientation::HORIZONTAL, 12};
};

#endif

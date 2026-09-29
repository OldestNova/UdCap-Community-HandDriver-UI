#ifndef UDCAP_CONTROLLER_SETTINGS_DIALOG_H
#define UDCAP_CONTROLLER_SETTINGS_DIALOG_H

#include <gtkmm.h>
#include <UdCapV1Core.h>
#include <memory>
#include <vector>

class ControllerSettingsDialog : public Gtk::Window {
public:
    explicit ControllerSettingsDialog(const std::vector<std::shared_ptr<UdCapV1Core>> &cores);

private:
    Gtk::Box content{Gtk::Orientation::HORIZONTAL, 12};
};

#endif

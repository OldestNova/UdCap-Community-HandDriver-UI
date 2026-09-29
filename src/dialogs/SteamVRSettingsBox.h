#pragma once
#include <gtkmm.h>
#include "components/SteamVRInstallation.h"
#include <atomic>
#include <memory>

class SteamVRSettingsBox final : public Gtk::Box {
public:
    SteamVRSettingsBox(std::string configPrefix, std::function<void()> onChanged);
    ~SteamVRSettingsBox() override;
private:
    struct Work { SteamVRInstallation::Status status; std::atomic_bool done{false}; };
    void start(int operation); // 0 refresh, 1 install, 2 uninstall
    Gtk::CheckButton enabled;
    Gtk::Label status, message;
    Gtk::Box buttons{Gtk::Orientation::HORIZONTAL,8};
    Gtk::Button install, uninstall, refresh;
    SteamVRInstallation::Status detected;
    std::shared_ptr<Work> work;
    sigc::connection timer;
};

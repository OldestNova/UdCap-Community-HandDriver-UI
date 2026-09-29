#ifndef UDCAP_STEAMVR_TRACKING_DIALOG_H
#define UDCAP_STEAMVR_TRACKING_DIALOG_H

#include <array>
#include <functional>
#include <gtkmm.h>
#include "components/SteamVRTrackingPresets.h"

class SteamVRTrackingDialog final : public Gtk::Window {
public:
    explicit SteamVRTrackingDialog(std::function<void()> onSaved,
                                  std::string gloveSerial = {}, int handIndex = -1);

private:
    void saveSettings();
    void loadPreset();
    void markCustom();
    SteamVRTrackingOffset displayedOffset(int hand) const;

    std::function<void()> onSaved_;
    std::string gloveSerial_;
    int handIndex_ = -1;
    Gtk::Box content_{Gtk::Orientation::VERTICAL, 8};
    Gtk::ComboBoxText presetChoice_;
    bool changingPreset_ = false;
    Gtk::Grid grid_;
    std::array<std::array<Gtk::SpinButton, 3>, 2> position_;
    std::array<std::array<Gtk::SpinButton, 3>, 2> rotation_;
    Gtk::Label status_;
};

#endif

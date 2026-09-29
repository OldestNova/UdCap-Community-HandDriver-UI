#include "SteamVRTrackingDialog.h"
#include "components/UserConfig.h"
#include "components/GloveConfig.h"
#include <glibmm/i18n.h>

#include <array>
#include <string>
#include <utility>

SteamVRTrackingDialog::SteamVRTrackingDialog(std::function<void()> onSaved,
                                             std::string gloveSerial, int handIndex)
    : onSaved_(std::move(onSaved)), gloveSerial_(std::move(gloveSerial)), handIndex_(handIndex) {
    set_title(_("SteamVR Tracker Offsets"));
    set_default_size(660, 440);
    content_.set_margin(12);
    auto roleNote = Gtk::make_managed<Gtk::Label>(
        _("Assign left and right hand Tracker roles in SteamVR's Manage Trackers panel."));
    roleNote->set_wrap(true);
    content_.append(*roleNote);
    content_.append(*Gtk::make_managed<Gtk::Label>(
        _("Offsets below are relative to each Tracker's local axes.")));
    auto presetRow = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    presetRow->append(*Gtk::make_managed<Gtk::Label>(_("Hand tracking preset")));
    presetChoice_.append("vive", "Vive Tracker 3.0");
    presetChoice_.append("tundra", "Tundra Tracker");
    presetChoice_.append("quest", "Quest");
    presetChoice_.append("utk", "UTK");
    presetChoice_.append("custom", _("Custom"));
    presetChoice_.set_hexpand(true);
    presetRow->append(presetChoice_);
    content_.append(*presetRow);
    auto axesNote = Gtk::make_managed<Gtk::Label>(
        _("Official preset axes are shown; Z position and Y rotation are converted for SteamVR."));
    axesNote->set_wrap(true);
    content_.append(*axesNote);

    grid_.set_column_spacing(12);
    grid_.set_row_spacing(7);
    if (handIndex_ < 0 || handIndex_ == 0)
        grid_.attach(*Gtk::make_managed<Gtk::Label>(handIndex_ < 0 ? _("Left hand") : gloveSerial_), 1, 0);
    if (handIndex_ < 0 || handIndex_ == 1)
        grid_.attach(*Gtk::make_managed<Gtk::Label>(handIndex_ < 0 ? _("Right hand") : gloveSerial_),
                     handIndex_ < 0 ? 2 : 1, 0);
    constexpr std::array<const char *, 3> axes{"x", "y", "z"};
    constexpr std::array<const char *, 3> displayAxes{"X", "Y", "Z"};
    auto &config = UserConfig::getInstance();
    for (int hand = 0; hand < 2; ++hand) {
        if (handIndex_ >= 0 && hand != handIndex_) continue;
        const std::string prefix = gloveSerial_.empty() ? (hand == 0 ? "/consumer/steamvr/left" : "/consumer/steamvr/right")
                                                        : gloveConfigPrefix(gloveSerial_) + "/steamvr";
        const int column = handIndex_ < 0 ? hand + 1 : 1;
        for (int axis = 0; axis < 3; ++axis) {
            auto &pos = position_[hand][axis];
            pos.set_range(-1, 1);
            pos.set_increments(0.005, 0.05);
            pos.set_digits(3);
            const double savedPosition = config.get<double>(prefix + "/position/" + axes[axis], 0);
            pos.set_value(axis == 2 ? -savedPosition : savedPosition);
            grid_.attach(pos, column, axis + 1);
            auto &rot = rotation_[hand][axis];
            rot.set_range(-360, 360);
            rot.set_increments(1, 10);
            rot.set_digits(1);
            const double savedRotation = config.get<double>(prefix + "/rotation/" + axes[axis], 0);
            rot.set_value(axis == 1 ? -savedRotation : savedRotation);
            grid_.attach(rot, column, axis + 4);
        }
    }
    for (int axis = 0; axis < 3; ++axis) {
        grid_.attach(*Gtk::make_managed<Gtk::Label>(
            Glib::ustring::compose(_("Position %1 (m)"), displayAxes[axis])), 0, axis + 1);
        grid_.attach(*Gtk::make_managed<Gtk::Label>(
            Glib::ustring::compose(_("Rotation %1 (deg)"), displayAxes[axis])), 0, axis + 4);
    }
    content_.append(grid_);
    bool matched = false;
    for (const auto &preset : steamVRTrackingPresets) {
        bool allVisibleHandsMatch = true;
        for (int hand = 0; hand < 2; ++hand) {
            if (handIndex_ >= 0 && hand != handIndex_) continue;
            allVisibleHandsMatch &= steamVRMatchesPreset(displayedOffset(hand), steamVRPresetHand(preset, hand));
        }
        if (allVisibleHandsMatch) {
            presetChoice_.set_active_id(preset.id);
            matched = true;
            break;
        }
    }
    if (!matched) presetChoice_.set_active_id("custom");
    for (int hand = 0; hand < 2; ++hand) {
        if (handIndex_ >= 0 && hand != handIndex_) continue;
        for (int axis = 0; axis < 3; ++axis) {
            position_[hand][axis].signal_value_changed().connect([this] { markCustom(); });
            rotation_[hand][axis].signal_value_changed().connect([this] { markCustom(); });
        }
    }
    presetChoice_.signal_changed().connect([this] { loadPreset(); });
    auto apply = Gtk::make_managed<Gtk::Button>(_("Apply and save"));
    apply->signal_clicked().connect([this]() { saveSettings(); });
    content_.append(*apply);
    content_.append(status_);
    set_child(content_);
}

void SteamVRTrackingDialog::saveSettings() {
    constexpr std::array<const char *, 3> axes{"x", "y", "z"};
    auto &config = UserConfig::getInstance();
    for (int hand = 0; hand < 2; ++hand) {
        if (handIndex_ >= 0 && hand != handIndex_) continue;
        const std::string prefix = gloveSerial_.empty() ? (hand == 0 ? "/consumer/steamvr/left" : "/consumer/steamvr/right")
                                                        : gloveConfigPrefix(gloveSerial_) + "/steamvr";
        const auto bridge = steamVRDisplayToBridge(displayedOffset(hand));
        for (int axis = 0; axis < 3; ++axis) {
            config.set<double>(prefix + "/position/" + axes[axis], bridge.position[axis]);
            config.set<double>(prefix + "/rotation/" + axes[axis], bridge.rotation[axis]);
        }
    }
    config.save();
    if (onSaved_) onSaved_();
    status_.set_text(_("Saved Tracker offsets."));
}

SteamVRTrackingOffset SteamVRTrackingDialog::displayedOffset(int hand) const {
    SteamVRTrackingOffset offset;
    for (int axis = 0; axis < 3; ++axis) {
        offset.position[axis] = position_[hand][axis].get_value();
        offset.rotation[axis] = rotation_[hand][axis].get_value();
    }
    return offset;
}

void SteamVRTrackingDialog::loadPreset() {
    const auto id = presetChoice_.get_active_id();
    for (const auto &preset : steamVRTrackingPresets) {
        if (id != preset.id) continue;
        changingPreset_ = true;
        for (int hand = 0; hand < 2; ++hand) {
            if (handIndex_ >= 0 && hand != handIndex_) continue;
            const auto &offset = steamVRPresetHand(preset, hand);
            for (int axis = 0; axis < 3; ++axis) {
                position_[hand][axis].set_value(offset.position[axis]);
                rotation_[hand][axis].set_value(offset.rotation[axis]);
            }
        }
        changingPreset_ = false;
        status_.set_text(_("Preset loaded. Apply and save to use it."));
        return;
    }
}

void SteamVRTrackingDialog::markCustom() {
    if (changingPreset_) return;
    if (presetChoice_.get_active_id() != "custom") presetChoice_.set_active_id("custom");
    status_.set_text("");
}

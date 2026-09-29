#include "ControllerSettingsDialog.h"
#include <glibmm/i18n.h>
#include <array>
#include <exception>

ControllerSettingsDialog::ControllerSettingsDialog(const std::vector<std::shared_ptr<UdCapV1Core>> &cores) {
    set_title(_("Controller Settings"));
    set_default_size(980, 580);
    content.set_margin(12);
    bool hasCore = false;
    for (std::size_t handIndex = 0; handIndex < cores.size(); ++handIndex) {
        const auto &core = cores[handIndex];
        if (!core) continue;
        hasCore = true;
        auto frame = Gtk::make_managed<Gtk::Frame>(
            std::string(core->getTarget() == UD_TARGET_LEFT_HAND ? _("Left glove") : _("Right glove")) +
            ": " + core->getUDCapSerial());
        frame->set_hexpand(true);
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
        box->set_margin(10);
        auto grid = Gtk::make_managed<Gtk::Grid>();
        grid->set_row_spacing(5);
        grid->set_column_spacing(10);
        const std::array<const char *, 6> labels{
            N_("Trigger minimum"), N_("Trigger maximum"), N_("Grip minimum"), N_("Grip maximum"),
            N_("Trackpad minimum"), N_("Trackpad maximum")
        };
        const std::array<float, 6> initial{
            core->getTriggerButtonMin(), core->getTriggerButtonMax(),
            core->getGripButtonMin(), core->getGripButtonMax(),
            core->getTrackpadButtonMin(), core->getTrackpadButtonMax()
        };
        std::array<Gtk::SpinButton *, 6> spins{};
        for (std::size_t i = 0; i < spins.size(); ++i) {
            auto label = Gtk::make_managed<Gtk::Label>(_(labels[i]));
            label->set_halign(Gtk::Align::START);
            spins[i] = Gtk::make_managed<Gtk::SpinButton>();
            spins[i]->set_range(0.0, 1.0);
            spins[i]->set_increments(0.01, 0.1);
            spins[i]->set_digits(2);
            spins[i]->set_value(initial[i]);
            grid->attach(*label, 0, static_cast<int>(i));
            grid->attach(*spins[i], 1, static_cast<int>(i));
        }
        box->append(*grid);
        auto status = Gtk::make_managed<Gtk::Label>();
        auto apply = Gtk::make_managed<Gtk::Button>(_("Apply and save"));
        apply->signal_clicked().connect([core, spins, status]() {
            try {
                if (spins[0]->get_value() >= spins[1]->get_value() ||
                    spins[2]->get_value() >= spins[3]->get_value() ||
                    spins[4]->get_value() >= spins[5]->get_value()) {
                    status->set_text(_("Each minimum must be below its maximum"));
                    return;
                }
                core->setTriggerButtonMin(spins[0]->get_value());
                core->setTriggerButtonMax(spins[1]->get_value());
                core->setGripButtonMin(spins[2]->get_value());
                core->setGripButtonMax(spins[3]->get_value());
                core->setTrackpadButtonMin(spins[4]->get_value());
                core->setTrackpadButtonMax(spins[5]->get_value());
                status->set_text(core->savePref() ? _("Saved") : _("Could not save settings"));
            } catch (const std::exception &e) {
                status->set_text(e.what());
            }
        });
        box->append(*apply);

        auto start = Gtk::make_managed<Gtk::Button>(_("Start joystick calibration"));
        auto capture = Gtk::make_managed<Gtk::Button>(_("Capture centered stick"));
        auto finish = Gtk::make_managed<Gtk::Button>(_("Finish after moving stick to all edges"));
        auto reset = Gtk::make_managed<Gtk::Button>(_("Reset joystick calibration"));
        capture->set_sensitive(false);
        finish->set_sensitive(false);
        start->signal_clicked().connect([core, start, capture, status]() {
            try {
                core->runCalibration(UDCAP_V1_DEVICE_CALI_TYPE_JOYSTICK);
                start->set_sensitive(false);
                capture->set_sensitive(true);
                status->set_text(_("Release the stick to its center, then capture"));
            } catch (const std::exception &e) { status->set_text(e.what()); }
        });
        capture->signal_clicked().connect([core, capture, finish, status]() {
            try {
                core->captureJoystickData(UDCAP_V1_JOYSTICK_CALI_TYPE_CENTER);
                capture->set_sensitive(false);
                finish->set_sensitive(true);
                status->set_text(_("Move the stick around its full range, then finish"));
            } catch (const std::exception &e) { status->set_text(e.what()); }
        });
        finish->signal_clicked().connect([core, start, finish, status]() {
            try {
                core->completeCalibration(UDCAP_V1_DEVICE_CALI_TYPE_JOYSTICK);
                finish->set_sensitive(false);
                start->set_sensitive(true);
                status->set_text(core->savePref() ? _("Joystick calibration saved") : _("Could not save settings"));
            } catch (const std::exception &e) { status->set_text(e.what()); }
        });
        reset->signal_clicked().connect([core, start, capture, finish, status]() {
            try {
                if (core->getJoystickCalibrationStatus() == UDCAP_V1_JOYSTICK_CALI_STAT_OK)
                    core->runCalibration(UDCAP_V1_DEVICE_CALI_TYPE_JOYSTICK);
                core->clearJoystickData(UDCAP_V1_JOYSTICK_CALI_TYPE_ALL);
                capture->set_sensitive(false);
                finish->set_sensitive(false);
                start->set_sensitive(true);
                status->set_text(core->savePref() ? _("Joystick calibration reset") : _("Could not save settings"));
            } catch (const std::exception &e) { status->set_text(e.what()); }
        });
        box->append(*start);
        box->append(*capture);
        box->append(*finish);
        box->append(*reset);
        auto vibration = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        for (int motor = 1; motor <= 2; ++motor) {
            auto button = Gtk::make_managed<Gtk::Button>(motor == 1
                ? _("Test vibration motor 1") : _("Test vibration motor 2"));
            button->signal_clicked().connect([core, status, motor]() {
                try {
                    core->mcuSendVibration(motor, 0.2f, 10);
                    status->set_text(_("Vibration command sent"));
                } catch (const std::exception &e) {
                    status->set_text(e.what());
                }
            });
            vibration->append(*button);
        }
        box->append(*vibration);
        box->append(*status);
        frame->set_child(*box);
        content.append(*frame);
    }
    if (!hasCore) content.append(*Gtk::make_managed<Gtk::Label>(_("No receivers found")));
    set_child(content);
}

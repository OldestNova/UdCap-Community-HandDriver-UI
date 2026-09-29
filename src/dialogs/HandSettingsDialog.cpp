#include "HandSettingsDialog.h"
#include "HandAdjustmentEditor.h"
#include "components/UserConfig.h"
#include <glibmm/i18n.h>
#include <array>
#include <exception>

HandSettingsDialog::HandSettingsDialog(const std::vector<std::shared_ptr<UdCapV1Core>> &cores,
                                       std::vector<std::string> algorithmKeys) {
    set_title(_("Hand Settings"));
    set_default_size(1080, 640);
    content.set_margin(12);
    bool hasCore = false;
    for (std::size_t handIndex = 0; handIndex < cores.size(); ++handIndex) {
        const auto &core = cores[handIndex];
        if (!core) continue;
        hasCore = true;
        auto frame = Gtk::make_managed<Gtk::Frame>(
            std::string(core->getTarget() == UD_TARGET_LEFT_HAND ? _("Left glove") : _("Right glove")) + ": " + core->getUDCapSerial());
        frame->set_hexpand(true);
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
        box->set_margin(10);

        auto algorithmRow = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        auto algorithmLabel = Gtk::make_managed<Gtk::Label>(_("Calibration algorithm"));
        auto algorithmChoice = Gtk::make_managed<Gtk::ComboBoxText>();
        algorithmChoice->append("0", _("Default (0, Original V1)"));
        algorithmChoice->append("1", _("Original V1 (1)"));
        algorithmChoice->append("20", _("Community V1 (20, adaptive)"));
        algorithmChoice->set_active_id(std::to_string(static_cast<int>(core->getAlgorithm())));
        algorithmChoice->set_hexpand(true);
        algorithmChoice->set_tooltip_text(_("Community V1 continuously corrects drift after the initial calibration. Occasionally open, close and spread your fingers naturally; no fixed sequence is required. Default and Original V1 do not auto-calibrate."));
        const std::string algorithmKey = handIndex < algorithmKeys.size() && !algorithmKeys[handIndex].empty()
            ? algorithmKeys[handIndex] : std::string("/consumer/algorithm") + (handIndex == 0 ? "/left" : "/right");
        algorithmChoice->signal_changed().connect([core, algorithmChoice, algorithmKey]() {
            const auto id = algorithmChoice->get_active_id();
            UdCapV1Algorithm value;
            if (id == "0") value = UdCapV1Algorithm::DEFAULT;
            else if (id == "1") value = UdCapV1Algorithm::ORIGINAL_V1;
            else if (id == "20") value = UdCapV1Algorithm::COUMMUNITY_V1;
            else return;
            core->setAlgorithm(value);
            UserConfig::getInstance().set<int>(algorithmKey, static_cast<int>(value));
            UserConfig::getInstance().save();
        });
        algorithmRow->append(*algorithmLabel);
        algorithmRow->append(*algorithmChoice);
        box->append(*algorithmRow);

        box->append(createHandAdjustmentEditor(core));

        auto save = Gtk::make_managed<Gtk::Button>(_("Save hand settings"));
        auto status = Gtk::make_managed<Gtk::Label>();
        save->signal_clicked().connect([core, status]() {
            try {
                status->set_text(core->savePref() ? _("Saved") : _("Could not save settings"));
            } catch (const std::exception &e) {
                status->set_text(e.what());
            }
        });
        box->append(*save);
        box->append(*status);
        frame->set_child(*box);
        content.append(*frame);
    }
    if (!hasCore) content.append(*Gtk::make_managed<Gtk::Label>(_("No receivers found")));
    set_child(content);
}

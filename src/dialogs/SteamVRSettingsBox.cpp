#include "SteamVRSettingsBox.h"
#include "components/UserConfig.h"
#include <glibmm/i18n.h>
#include <thread>

SteamVRSettingsBox::SteamVRSettingsBox(std::string prefix,std::function<void()> onChanged)
    : Gtk::Box(Gtk::Orientation::VERTICAL,8), enabled(_("Enable SteamVR")),
      install(_("Install SteamVR driver")),uninstall(_("Uninstall SteamVR driver")),refresh(_("Refresh")) {
    set_margin(8);
    enabled.set_active(UserConfig::getInstance().get<bool>(prefix+"/vr/enabled",false));
    enabled.signal_toggled().connect([this,prefix,onChanged]() {
        auto &config=UserConfig::getInstance(); config.set<bool>(prefix+"/vr/enabled",enabled.get_active());config.save();
        if(onChanged) onChanged();
    });
    status.set_halign(Gtk::Align::START);status.set_wrap(true);
    message.set_halign(Gtk::Align::START);message.set_wrap(true);message.set_selectable(true);
    append(enabled);append(status);
    buttons.append(install);buttons.append(uninstall);buttons.append(refresh);append(buttons);append(message);
    install.signal_clicked().connect([this](){start(1);});
    uninstall.signal_clicked().connect([this](){start(2);});
    refresh.signal_clicked().connect([this](){start(0);});
    start(0);
}
SteamVRSettingsBox::~SteamVRSettingsBox() {timer.disconnect();}
void SteamVRSettingsBox::start(int operation) {
    if(work) return;
    buttons.set_sensitive(false);message.set_text(_("Checking SteamVR…"));
    work=std::make_shared<Work>(); auto pending=work; auto previous=detected;
    // The worker holds data only. Closing the preferences window never leaves
    // a callback or worker with a dangling Gtk widget pointer.
    std::thread([pending,previous,operation]() {
        try {pending->status=operation?SteamVRInstallation::change(previous,operation==1):SteamVRInstallation::detect();}
        catch(const std::exception &e) {pending->status.error=e.what();}
        pending->done.store(true,std::memory_order_release);
    }).detach();
    timer=Glib::signal_timeout().connect([this,operation]() {
        if(!work->done.load(std::memory_order_acquire)) return true;
        detected=std::move(work->status);work.reset();buttons.set_sensitive(true);
        const bool available=!detected.tool.empty(), registered=!detected.installedPaths.empty();
        install.set_sensitive(available && detected.bundleAvailable && !registered && detected.error.empty());
        uninstall.set_sensitive(available && registered && detected.error.empty());
        status.set_text(!available?_("SteamVR was not detected."):
            registered?_("SteamVR detected · udcapc driver installed"):
            detected.bundleAvailable?_("SteamVR detected · udcapc driver not installed"):
            _("SteamVR detected · this build does not include the udcapc driver"));
        std::string paths=SteamVRInstallation::utf8(detected.tool)+"\n"+SteamVRInstallation::utf8(detected.bundle);
        for(const auto &p:detected.installedPaths) paths+="\n"+SteamVRInstallation::utf8(p);
        status.set_tooltip_text(paths);
        if(!detected.error.empty()) message.set_text(std::string(_("SteamVR operation failed: "))+detected.error);
        else {
            std::string text=operation?_("Done. Restart SteamVR to apply the driver change."):"";
            if(detected.officialDriverPresent) {
                if(!text.empty()) text+="\n";
                text+=_("The official udcap driver is also registered. Manage enabled add-ons in SteamVR to avoid duplicate controllers.");
            }
            message.set_text(text);
        }
        return false;
    },100);
}

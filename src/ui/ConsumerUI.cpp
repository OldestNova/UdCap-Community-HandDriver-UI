//
// Created by max_3 on 2025/6/6.
//

#include <iostream>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include "ConsumerUI.h"
#include "../components/UserConfig.h"
#include "UsbEnumerate.h"
#include "components/GloveConfig.h"

namespace {

void setOutputStatus(Gtk::Label *label, const Glib::ustring &message) {
    label->set_text(message);
    label->set_visible(!message.empty());
}

void attachOutputRow(Gtk::Grid &grid, int row, Gtk::CheckButton &enabled,
                     Gtk::Entry &host, Gtk::Entry &port, Gtk::Button &apply,
                     Gtk::Label &status) {
    enabled.set_halign(Gtk::Align::START);
    host.set_hexpand(true);
    auto hostLabel = Gtk::make_managed<Gtk::Label>(_("Host"));
    auto portLabel = Gtk::make_managed<Gtk::Label>(_("Port"));
    grid.attach(enabled, 0, row * 2);
    grid.attach(*hostLabel, 1, row * 2);
    grid.attach(host, 2, row * 2);
    grid.attach(*portLabel, 3, row * 2);
    grid.attach(port, 4, row * 2);
    grid.attach(apply, 5, row * 2);
    status.set_halign(Gtk::Align::START);
    status.set_visible(false);
    grid.attach(status, 0, row * 2 + 1, 6);
}

UdCapV1Algorithm algorithmFromConfig(int saved) {
    switch (saved) {
        case static_cast<int>(UdCapV1Algorithm::ORIGINAL_V1):
            return UdCapV1Algorithm::ORIGINAL_V1;
        case static_cast<int>(UdCapV1Algorithm::COUMMUNITY_V1):
            return UdCapV1Algorithm::COUMMUNITY_V1;
        default:
            return UdCapV1Algorithm::DEFAULT;
    }
}

} // namespace

ConsumerUI::ConsumerUI(std::shared_ptr<Gtk::Application> app, bool advanced):
    mMainBox(Gtk::Orientation::VERTICAL),
    mStatus(_("Not Connect"), Gtk::Align::START),
    mDescription(_("Please connect your UdCap gloves."), Gtk::Align::START),
    mCalibrate(_("Calibrate")),
    mSettings(),
    mSettingsMenu(),
    mTopBox(Gtk::Orientation::VERTICAL, 5),
    mFirstRow(Gtk::Orientation::HORIZONTAL, 10),
    mSecondRow(Gtk::Orientation::HORIZONTAL, 10),
    mThirdRow(Gtk::Orientation::HORIZONTAL, 10),
    mLeftBox(Gtk::Orientation::HORIZONTAL, 5),
    mRightBox(Gtk::Orientation::HORIZONTAL, 5) {
    advancedMode = advanced;
    set_application(app);
    set_title(advancedMode ? _("UdCap Community Driver - Advanced Mode")
                           : _("UdCap Community Driver - Consumer Edition"));
    set_default_size(advancedMode ? 950 : 600, advancedMode ? 700 : 550);
    set_resizable(true);
    signal_close_request().connect([this]() {
        stopGloveStreams();
        return false;
    }, false);

    mMainBox.set_margin(10);
    set_child(mMainBox);
    if (!advancedMode) {
        mMainBox.append(mTopBox);
    } else {
        mAdvancedRoot.set_hexpand(true);
        mAdvancedRoot.set_vexpand(true);
        mMainBox.append(mAdvancedRoot);
        mAdvancedRail.set_size_request(58, -1);
        mAdvancedRail.set_margin_end(8);
        mAdvancedRoot.append(mAdvancedRail);
        mAdvancedRoot.append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
        mAdvancedPages.set_hexpand(true);
        mAdvancedPages.set_vexpand(true);
        mAdvancedRoot.append(mAdvancedPages);

        mAdvancedDeviceLayout.set_hexpand(true);
        mAdvancedDeviceLayout.set_vexpand(true);
        mAdvancedListPane.set_size_request(300, -1);
        mAdvancedListPane.set_margin(10);
        mAdvancedDeviceLayout.append(mAdvancedListPane);
        mAdvancedDeviceLayout.append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
        mAdvancedDevicePage.set_hexpand(true);
        mAdvancedDevicePage.set_vexpand(true);
        mAdvancedDeviceLayout.append(mAdvancedDevicePage);
    }

    mTopBox.set_margin(10);
    mTopBox.set_spacing(15);

    mStatus.set_markup(_("<span font='18' weight='bold'>Not Found</span>"));
    mStatus.set_halign(Gtk::Align::START);
    mStatus.set_hexpand(true);

    mFirstRow.set_spacing(10);

    mSettings.set_icon_name("open-menu-symbolic");
    mSettings.set_valign(Gtk::Align::END);
    buildMenu();
    mSettings.set_menu_model(mSettingsMenu);
    mSettings.set_visible(!advancedMode);

    mFirstRow.append(mStatus);
    mFirstRow.append(mSettings);
    mTopBox.append(mFirstRow);

    mSecondRow.set_spacing(10);
    mDescription.set_text(_("Please connect your UdCap receiver."));
    mDescription.set_halign(Gtk::Align::START);
    mDescription.set_hexpand(true);

    mCalibrate.set_label(_("Calibrate"));
    mCalibrate.signal_clicked().connect(sigc::mem_fun(*this, &ConsumerUI::on_calibrate_button_clicked));
    mCalibrate.set_halign(Gtk::Align::END);
    mCalibrate.set_sensitive(false);

    mSecondRow.append(mDescription);
#ifndef NO_3DPREVIEW
    mPreviewToggle.set_tooltip_text(_("Show or hide the 3D hand preview. Restart after hiding to reclaim graphics driver memory."));
    mSecondRow.append(mPreviewToggle);
#endif
    if (!advancedMode) mSecondRow.append(mCalibrate);
    mTopBox.append(mSecondRow);

    if (advancedMode) {
        mAdvancedAddDialog.set_title(_("Create glove group"));
        mAdvancedAddDialog.set_default_size(560, 360);
        mAdvancedAddDialog.set_transient_for(*this);
        mAdvancedAddDialog.set_modal(true);
        mAdvancedAddDialog.signal_close_request().connect([this]() {
            mAdvancedAddDialog.hide();
            return true;
        }, false);
        mAdvancedControls.set_margin(18);
        mAdvancedAddDialog.set_child(mAdvancedControls);

        const std::string assignments = UserConfig::getInstance().get<std::string>("/enterprise/assignments", "");
        for (std::size_t begin = 0; begin < assignments.size();) {
            const auto end = assignments.find(',', begin);
            const auto id = assignments.substr(begin, end == std::string::npos ? end : end - begin);
            if (decodePairId(id)) savedPairIds.insert(id);
            if (end == std::string::npos) break;
            begin = end + 1;
        }
        // The original per-pair QingTong configuration becomes one destination.
        // Preserve the first enabled pair when upgrading an existing profile.
        auto &settings = UserConfig::getInstance();
        const std::string qtGlobal = "/enterprise/udCapQingTong";
        if (!settings.get<bool>(qtGlobal + "/migrated", false)) {
            if (settings.get<int>(qtGlobal + "/port", -1) == -1) {
                for (const auto &id : savedPairIds) {
                    const std::string old = "/enterprise/pairs/" + id + "/udCapQingTong";
                    if (!settings.get<bool>(old + "/enabled", false)) continue;
                    settings.set<bool>(qtGlobal + "/enabled", true);
                    settings.set<std::string>(qtGlobal + "/host",
                        settings.get<std::string>(old + "/host", "127.0.0.1"));
                    settings.set<int>(qtGlobal + "/port", settings.get<int>(old + "/port", 6666));
                    break;
                }
            }
            settings.set<bool>(qtGlobal + "/migrated", true);
            settings.save();
        }
        const std::string exclusions = UserConfig::getInstance().get<std::string>("/enterprise/excludedAutoPairs", "");
        for (std::size_t begin = 0; begin < exclusions.size();) {
            const auto end = exclusions.find(',', begin);
            const auto id = exclusions.substr(begin, end == std::string::npos ? end : end - begin);
            if (decodePairId(id)) excludedAutoPairs.insert(id);
            if (end == std::string::npos) break;
            begin = end + 1;
        }
        mAutoPair.set_active(UserConfig::getInstance().get<bool>("/enterprise/autoPair", true));
        mAutoPair.signal_toggled().connect([this]() {
            UserConfig::getInstance().set<bool>("/enterprise/autoPair", mAutoPair.get_active());
            UserConfig::getInstance().save();
            if (mAutoPair.get_active()) tryAutoPair();
        });
        mAdvancedControls.append(mAutoPair);

        auto leftReceiverLabel = Gtk::make_managed<Gtk::Label>(_("Left receiver"));
        leftReceiverLabel->set_halign(Gtk::Align::START);
        mAdvancedBox.append(*leftReceiverLabel);
        mAdvancedLeft.set_hexpand(true);
        mAdvancedBox.append(mAdvancedLeft);
        auto rightReceiverLabel = Gtk::make_managed<Gtk::Label>(_("Right receiver"));
        rightReceiverLabel->set_halign(Gtk::Align::START);
        mAdvancedBox.append(*rightReceiverLabel);
        mAdvancedRight.set_hexpand(true);
        mAdvancedBox.append(mAdvancedRight);
        mAdvancedConnect.signal_clicked().connect([this]() {
            const std::string left = mAdvancedLeft.get_active_id().raw();
            const std::string right = mAdvancedRight.get_active_id().raw();
            if (left.empty() || right.empty() || left == right) {
                mAdvancedAddError.set_text(_("Choose distinct left and right receivers."));
                mAdvancedAddError.set_visible(true);
                return;
            }
            const std::string id = pairId(left, right);
            std::vector<std::pair<std::string, std::string>> replacedPairs;
            if (!advancedPairs.contains(id)) {
                for (const auto &[oldId, pair] : advancedPairs) {
                    if (pair->leftSerial == left || pair->rightSerial == right)
                        replacedPairs.emplace_back(pair->leftSerial, pair->rightSerial);
                }
                for (const auto &[oldLeft, oldRight] : replacedPairs)
                    removeAdvancedPair(pairId(oldLeft, oldRight));
            }
            if (advancedPairs.contains(id) || addAdvancedPair(left, right)) {
                mAdvancedAddError.set_visible(false);
                selectedGloveSerial.clear();
                selectedSavedPairId.clear();
                mAdvancedActivePair.set_active_id(id);
                refreshAdvancedDetail();
                queueAdvancedDeviceListRefresh();
                showAdvancedPage("devices");
                mAdvancedAddDialog.hide();
            } else {
                const std::string error = mDescription.get_text();
                for (const auto &[oldLeft, oldRight] : replacedPairs)
                    addAdvancedPair(oldLeft, oldRight);
                mAdvancedAddError.set_text(error);
                mAdvancedAddError.set_visible(true);
            }
        });
        mAdvancedControls.append(mAdvancedBox);
        auto reassignmentHint = Gtk::make_managed<Gtk::Label>(_("Selecting a receiver in another group will dissolve that group."));
        reassignmentHint->set_halign(Gtk::Align::START);
        reassignmentHint->set_wrap(true);
        mAdvancedControls.append(*reassignmentHint);
        mAdvancedAddError.set_halign(Gtk::Align::START);
        mAdvancedAddError.set_visible(false);
        mAdvancedControls.append(mAdvancedAddError);

        auto savedSection = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
        auto savedLabel = Gtk::make_managed<Gtk::Label>(_("Saved pairs not connected"));
        savedLabel->set_halign(Gtk::Align::START);
        savedSection->append(*savedLabel);
        auto savedRow = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        mSavedPairSelector.set_hexpand(true);
        savedRow->append(mSavedPairSelector);
        mForgetSavedPair.set_sensitive(false);
        mSavedPairSelector.signal_changed().connect([this]() {
            mForgetSavedPair.set_sensitive(!mSavedPairSelector.get_active_id().empty());
        });
        mForgetSavedPair.signal_clicked().connect([this]() {
            forgetSavedPair(mSavedPairSelector.get_active_id().raw());
        });
        savedRow->append(mForgetSavedPair);
        savedSection->append(*savedRow);
        mAdvancedControls.append(*savedSection);
        auto addActions = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        addActions->set_halign(Gtk::Align::END);
        mAdvancedAddClose.signal_clicked().connect([this]() { mAdvancedAddDialog.hide(); });
        addActions->append(mAdvancedAddClose);
        addActions->append(mAdvancedConnect);
        mAdvancedControls.append(*addActions);

        mAdvancedPairsScroll.set_min_content_height(300);
        mAdvancedPairsScroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
        mAdvancedPairsScroll.set_child(mAdvancedPairsBox);
        mAdvancedPairDetailFrame.set_child(mAdvancedPairsScroll);
        mTopBox.append(mAdvancedPairDetailFrame);

        auto activeRow = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        activeRow->append(*Gtk::make_managed<Gtk::Label>(_("Pair for preview and controls")));
        activeRow->append(mAdvancedActivePair);
        mAdvancedActivePair.signal_changed().connect([this]() {
            if (updatingAdvancedSelectors) return;
            selectAdvancedPair(mAdvancedActivePair.get_active_id().raw());
        });
        // The selected list row controls the active pair; keep the combo as the
        // existing selection model without showing a second selector.

        mAdvancedAddButton.set_label(_("Add group"));
        mAdvancedAddButton.set_tooltip_text(_("Create glove group"));
        mAdvancedAddButton.signal_clicked().connect([this]() {
            refreshAdvancedSelectors();
            mAdvancedAddError.set_visible(false);
            mAdvancedAddDialog.present();
        });
        auto listTitle = Gtk::make_managed<Gtk::Label>(_("Gloves and groups"));
        listTitle->set_halign(Gtk::Align::START);
        listTitle->add_css_class("title-3");
        mAdvancedListPane.append(*listTitle);
        auto toolbar = Gtk::make_managed<Gtk::Frame>();
        auto toolbarActions = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
        toolbarActions->set_margin(6);
        mAdvancedCalibrateAll.set_tooltip_text(_("Calibrate all connected glove groups"));
        mAdvancedCalibrateAll.set_sensitive(false);
        mAdvancedCalibrateAll.signal_clicked().connect(
            sigc::mem_fun(*this, &ConsumerUI::on_calibrate_all_button_clicked));
        toolbarActions->append(mAdvancedCalibrateAll);
        toolbarActions->append(mAdvancedAddButton);
        toolbar->set_child(*toolbarActions);
        mAdvancedListPane.append(*toolbar);
        mAdvancedListScroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
        mAdvancedListScroll.set_vexpand(true);
        mAdvancedListScroll.set_child(mAdvancedDeviceList);
        mAdvancedListPane.append(mAdvancedListScroll);

        auto singleBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
        singleBox->set_margin(10);
        mAdvancedSingleSide.set_halign(Gtk::Align::START);
        mAdvancedSingleStatus.set_halign(Gtk::Align::START);
        mAdvancedSingleGroup.set_halign(Gtk::Align::START);
        mAdvancedSingleRate.set_halign(Gtk::Align::START);
        mAdvancedSingleSignal.set_halign(Gtk::Align::START);
        singleBox->append(mAdvancedSingleSide);
        singleBox->append(mAdvancedSingleStatus);
        singleBox->append(mAdvancedSingleGroup);
        singleBox->append(mAdvancedSingleRate);
        singleBox->append(mAdvancedSingleSignal);
        auto singleActions = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        mAdvancedSingleController.signal_clicked().connect([this]() { activate_action("win.settings.controller"); });
        mAdvancedSingleHands.signal_clicked().connect([this]() { activate_action("win.settings.hands"); });
        mAdvancedSingleTracking.signal_clicked().connect([this]() { activate_action("win.settings.steamvr_tracking"); });
        singleActions->append(mAdvancedSingleController);
        singleActions->append(mAdvancedSingleHands);
        singleActions->append(mAdvancedSingleTracking);
        mAdvancedSingleRestart.signal_clicked().connect([this]() {
            if (selectedGloveSerial.empty()) return;
            const auto serial = selectedGloveSerial;
            if (const auto core = coreForSerial(serial))
                confirmRestartGloves({core}, serial);
        });
        singleActions->append(mAdvancedSingleRestart);
        singleBox->append(*singleActions);
        mAdvancedSingleFrame.set_child(*singleBox);
        mAdvancedSingleFrame.set_visible(false);
        mTopBox.append(mAdvancedSingleFrame);
        auto savedBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
        savedBox->set_margin(10);
        mAdvancedSavedStatus.set_halign(Gtk::Align::START);
        savedBox->append(mAdvancedSavedStatus);
        mAdvancedSavedVRSelect.set_halign(Gtk::Align::START);
        mAdvancedSavedVRSelect.signal_clicked().connect([this]() {
            const auto id = selectedSavedPairId;
            if (!savedPairIds.contains(id) || advancedPairs.contains(id)) return;
            UserConfig::getInstance().set<std::string>("/enterprise/vr/pairId", id);
            UserConfig::getInstance().save();
            setupVRSender();
            refreshAdvancedDetail();
        });
        savedBox->append(mAdvancedSavedVRSelect);
        auto savedOutputGrid = Gtk::make_managed<Gtk::Grid>();
        savedOutputGrid->set_column_spacing(8);
        savedOutputGrid->set_row_spacing(4);
        constexpr std::array<const char *, 2> outputLabels = {"VMC", "VRChat OSC"};
        for (std::size_t index = 0; index < mSavedOutputs.size(); ++index) {
            auto &controls = mSavedOutputs[index];
            controls.enabled = Gtk::make_managed<Gtk::CheckButton>(outputLabels[index]);
            controls.host = Gtk::make_managed<Gtk::Entry>();
            controls.host->set_width_chars(15);
            controls.port = Gtk::make_managed<Gtk::Entry>();
            controls.port->set_width_chars(6);
            controls.status = Gtk::make_managed<Gtk::Label>();
            auto apply = Gtk::make_managed<Gtk::Button>(_("Apply"));
            apply->signal_clicked().connect([this, index]() { applySavedPairOutput(index); });
            attachOutputRow(*savedOutputGrid, static_cast<int>(index), *controls.enabled,
                            *controls.host, *controls.port, *apply, *controls.status);
        }
        savedBox->append(*savedOutputGrid);
        mAdvancedSavedFrame.set_child(*savedBox);
        mAdvancedSavedFrame.set_visible(false);
        mTopBox.append(mAdvancedSavedFrame);

        mAdvancedDevicePage.append(mTopBox);
        mAdvancedPages.add(mAdvancedDeviceLayout, "devices");

        mAdvancedFirmwareContent.set_margin(20);
        mAdvancedFirmwareContent.set_vexpand(true);
        mAdvancedFirmwarePanel.set_vexpand(true);
        auto firmwareHeading = Gtk::make_managed<Gtk::Label>(_("Firmware"));
        firmwareHeading->set_halign(Gtk::Align::START);
        firmwareHeading->add_css_class("title-2");
        mAdvancedFirmwareContent.append(*firmwareHeading);
        mAdvancedFirmwareContent.append(mAdvancedFirmwarePanel);
        mAdvancedPages.add(mAdvancedFirmwareContent, "firmware");

        mAdvancedPreferenceContent.set_margin(20);
        auto preferenceHeading = Gtk::make_managed<Gtk::Label>(_("Preference"));
        preferenceHeading->set_halign(Gtk::Align::START);
        preferenceHeading->add_css_class("title-2");
        mAdvancedPreferenceContent.append(*preferenceHeading);
        populatePreferences(mAdvancedPreferenceContent, *this, [this]() { setupVRSender(); });
        auto qtFrame = Gtk::make_managed<Gtk::Frame>(_("QingTong UDP"));
        auto qtGrid = Gtk::make_managed<Gtk::Grid>();
        qtGrid->set_margin(8);
        qtGrid->set_column_spacing(8);
        qtGrid->set_row_spacing(4);
        mAdvancedQtEnabled.set_active(settings.get<bool>(qtGlobal + "/enabled", false));
        mAdvancedQtHost.set_width_chars(15);
        mAdvancedQtHost.set_text(settings.get<std::string>(qtGlobal + "/host", "127.0.0.1"));
        mAdvancedQtPort.set_width_chars(6);
        mAdvancedQtPort.set_text(std::to_string(settings.get<int>(qtGlobal + "/port", 6666)));
        auto qtApply = Gtk::make_managed<Gtk::Button>(_("Apply"));
        attachOutputRow(*qtGrid, 0, mAdvancedQtEnabled, mAdvancedQtHost,
                        mAdvancedQtPort, *qtApply, mAdvancedQtStatus);
        qtApply->signal_clicked().connect([this, qtGlobal]() {
            try {
                const std::string host = mAdvancedQtHost.get_text();
                boost::asio::ip::make_address_v4(host);
                const std::string text = mAdvancedQtPort.get_text();
                std::size_t length = 0;
                const int port = std::stoi(text, &length);
                if (length != text.size() || port < 1 || port > 65535)
                    throw std::invalid_argument("port");
                auto &config = UserConfig::getInstance();
                config.set<bool>(qtGlobal + "/enabled", mAdvancedQtEnabled.get_active());
                config.set<std::string>(qtGlobal + "/host", host);
                config.set<int>(qtGlobal + "/port", port);
                config.save();
                setOutputStatus(&mAdvancedQtStatus, "");
                setupUdcapQTSender();
            } catch (const std::exception &) {
                setOutputStatus(&mAdvancedQtStatus, _("Enter a valid IPv4 address and port (1-65535)."));
            }
        });
        qtFrame->set_child(*qtGrid);
        mAdvancedPreferenceContent.append(*qtFrame);
        mAdvancedPreferenceScroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
        mAdvancedPreferenceScroll.set_child(mAdvancedPreferenceContent);
        mAdvancedPages.add(mAdvancedPreferenceScroll, "preferences");

        const auto addNav = [this](const char *page, const char *icon, const char *tip) {
            auto button = Gtk::make_managed<Gtk::Button>();
            button->set_icon_name(icon);
            button->set_tooltip_text(tip);
            button->set_size_request(46, 46);
            button->signal_clicked().connect([this, page]() { showAdvancedPage(page); });
            mAdvancedRail.append(*button);
            mAdvancedNavButtons.emplace_back(page, button);
        };
        addNav("devices", "view-list-symbolic", _("Gloves and groups"));
        addNav("firmware", "drive-harddisk-symbolic", _("Firmware"));
        addNav("preferences", "preferences-system-symbolic", _("Preference"));
        showAdvancedPage("devices");
        mStatus.set_markup(_("<span font='18' weight='bold'>No pair selected</span>"));
        mDescription.set_text(_("Add a glove pair to begin."));
    }

    auto separator = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
    mTopBox.append(*separator);

    mThirdRow.set_spacing(30);
    mLeftBox.set_spacing(10);
    mRightBox.set_spacing(10);

    mLeftBox.append(mLeftHand);
    mLeftBox.append(mRightHand);
    mRightBox.append(mVR);
    mRightBox.append(mOSC);
    mRightBox.append(mVMC);
    mRightBox.append(mBroadcast);
    installIndicatorTooltip(mLeftHand, Indicator::LeftGlove);
    installIndicatorTooltip(mRightHand, Indicator::RightGlove);
    installIndicatorTooltip(mVR, Indicator::SteamVR);
    installIndicatorTooltip(mOSC, Indicator::OSC);
    installIndicatorTooltip(mVMC, Indicator::VMC);
    installIndicatorTooltip(mBroadcast, Indicator::QingTong);
    mLeftBox.set_valign(Gtk::Align::START);
    mRightBox.set_valign(Gtk::Align::END);
    auto spacer = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    spacer->set_hexpand(true);
    mThirdRow.append(mLeftBox);
    mThirdRow.append(*spacer);
    mThirdRow.append(mRightBox);
    mTopBox.append(mThirdRow);

#ifndef NO_3DPREVIEW
    const std::string previewKey = advancedMode ? "/enterprise/preview3d" : "/consumer/preview3d";
    mPreviewToggle.set_active(UserConfig::getInstance().get<bool>(previewKey, false));
    mPreviewToggle.signal_toggled().connect([this, previewKey]() {
        UserConfig::getInstance().set<bool>(previewKey, mPreviewToggle.get_active());
        UserConfig::getInstance().save();
        updatePreviewVisibility();
    });
    updatePreviewVisibility();
#endif

    if (advancedMode) refreshAdvancedSelectors();

    probeThread = std::thread([this]() {
        std::map<std::string, std::string> connected;
        while (probeRunning) {
             if (retryProbe.exchange(false)) connected.clear();
             std::map<std::string, std::shared_ptr<PortAccessor>> receiver;
             try {
                 UsbEnumerate::getInstance()->refresh(
                     UsbEnumerateRefreshType::USB_ENUMERATE_REFRESH_SERIAL,
                     [](uint16_t vid, uint16_t pid) {
                         return vid == 0x1A86 && (pid == 0x7523 || pid == 0x0001);
                     },
                     true);
                 const auto devices = UsbEnumerate::getInstance()->findPorts([](const SerialDevice &device) {
                     return device.vid == 0x1A86 && (device.pid == 0x7523 || device.pid == 0x0001);
                 });
                 for (auto it = connected.begin(); it != connected.end();) {
                     const bool present = std::any_of(devices.begin(), devices.end(), [&it](const SerialDevice &device) {
                         return device.portName == it->first;
                     });
                     if (present) { ++it; continue; }
                     if (advancedMode) {
                         runOnUIThread([this, serial = it->second]() {
                             availableReceivers.erase(serial);
                             bool grouped = false;
                             for (const auto &[id, pair] : advancedPairs)
                                 grouped |= pair->leftSerial == serial || pair->rightSerial == serial;
                             if (!grouped) standaloneCores.erase(serial);
                             refreshAdvancedSelectors();
                             return false;
                         });
                     }
                     it = connected.erase(it);
                 }
                 for (const auto &device: devices) {
                     if (!probeRunning) break;
                     if (connected.contains(device.portName)) continue;
                     try {
                         std::cout << "Probing: " << device.portName << std::endl;
                         auto portAccessor = std::make_shared<PortAccessor>(device);
                         UdCapProbe prober(portAccessor);
                         if (prober.probe() != UDCAP_PROBE_HAND_V1) continue;
                         receiver[prober.getUDCapSerial()] = portAccessor;
                         connected[device.portName] = prober.getUDCapSerial();
                         std::cout << "Found UdCap Hand V1 device: " << prober.getUDCapSerial() << std::endl;
                     } catch (const std::exception &e) {
                         std::cerr << "Could not probe " << device.portName << ": " << e.what() << std::endl;
                     }
                 }
             } catch (const std::exception &e) {
                 std::cerr << "Could not enumerate receivers: " << e.what() << std::endl;
             }
             if (!receiver.empty()) {
                 if (advancedMode) {
                     for (const auto &recv : receiver) {
                         runOnUIThread([this, serial = recv.first, accessor = recv.second]() {
                             availableReceivers[serial] = accessor;
                             refreshAdvancedSelectors();
                             restoreSavedPairs();
                             tryAutoPair();
                             if (activePairId.empty()) {
                                 mStatus.set_markup(_("<span font='18' weight='bold'>Receivers found</span>"));
                                 mDescription.set_text(_("Select the receivers to connect."));
                             }
                             return false;
                         });
                     }
                 } else {
                 for (const auto& recv: receiver) {
                     runOnUIThread([this, serial = recv.first, accessor = recv.second]() {
                         if (serial.ends_with('L') && !leftHandSerial) {
                             leftHandSerial = accessor;
                             simpleSerials[0] = serial;
                             mLeftHand.setState(StatusIndicator::State::Waiting);
                         } else if (serial.ends_with('R') && !rightHandSerial) {
                             rightHandSerial = accessor;
                             simpleSerials[1] = serial;
                             mRightHand.setState(StatusIndicator::State::Waiting);
                         }
                         if (leftHandSerial || rightHandSerial) initializeCores();
                         return false;
                     });
                 }
                 }
             }
             std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    setupVMCSender();
    setupOSCSender();
    setupUdcapQTSender();
    setupVRSender();
    OSCServer::getInstance().setCallback([this](std::string address, std::vector<std::any> args){
        runOnUIThread([this, address = std::move(address), args = std::move(args)]() {
        if (address == "/udcap/device/calibrate") {
            if (args.size() < 3) return false;
            std::vector<std::shared_ptr<UdCapV1Core>> caliCores;
            std::vector<std::shared_ptr<UdCapV1Core>> connectedCores;
            if (advancedMode) {
                for (const auto &[id, pair] : advancedPairs) {
                    if (pair->leftState == UD_INIT_STATE_LINKED) connectedCores.push_back(pair->left);
                    if (pair->rightState == UD_INIT_STATE_LINKED) connectedCores.push_back(pair->right);
                }
            } else {
                if (mHandCoreLeft && mLeftState == UD_INIT_STATE_LINKED) connectedCores.push_back(mHandCoreLeft);
                if (mHandCoreRight && mRightState == UD_INIT_STATE_LINKED) connectedCores.push_back(mHandCoreRight);
            }
            bool isRegex = false;
            bool autoStart = false;
            if (args[0].has_value() && args[0].type() == typeid(int32_t)) {
                if (any_cast<int32_t>(args[0]) == 1) {
                    isRegex = true;
                }
            }
            if (args[1].has_value() && args[1].type() == typeid(int32_t)) {
                if (any_cast<int32_t>(args[1]) == 1) {
                    autoStart = true;
                }
            }
            if (args[2].has_value() && args[2].type() == typeid(std::vector<std::string>)) {
                const auto serials = any_cast<std::vector<std::string>>(args[2]);
                for (const auto &serial : serials) {
                    std::optional<std::regex> pattern;
                    if (isRegex) {
                        try { pattern.emplace(serial); }
                        catch (const std::regex_error &) { continue; }
                    }
                    for (const auto &core : connectedCores) {
                        const bool match = pattern ? std::regex_match(core->getUDCapSerial(), *pattern)
                                                   : core->getUDCapSerial() == serial;
                        if (match && std::find(caliCores.begin(), caliCores.end(), core) == caliCores.end())
                            caliCores.push_back(core);
                    }
                }
            }

            if (!caliCores.empty()) {
                runOnUIThread([this, caliCores, autoStart](){
                    try {
                        mCalibrationCores = caliCores;
                        mCalibrationUI = std::make_unique<CalibrationUI>(caliCores);
                        mCalibrationUI->set_transient_for(*this);
                        mCalibrationUI->show();
                        if (autoStart) {
                            mCalibrationUI->startProcess();
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "Error in calibration UI: " << e.what() << std::endl;
                    }
                    return false;
                });
            }
        }
        return false;
        });
    });
    OSCServer::getInstance().restart();
    lastTelemetrySample = std::chrono::steady_clock::now();
    mTelemetryTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &ConsumerUI::sampleFrameRates), 1000);
    mSteamVRHapticTimer = Glib::signal_timeout().connect([this]() {
        if (mSteamVRSender) mSteamVRSender->pollHaptics();
        return true;
    }, 20);
}

void ConsumerUI::installIndicatorTooltip(Gtk::Widget &widget, Indicator indicator) {
    widget.set_has_tooltip(true);
    widget.signal_query_tooltip().connect([this, indicator](int, int, bool,
            const Glib::RefPtr<Gtk::Tooltip> &tooltip) {
        tooltip->set_text(indicatorTooltip(indicator));
        return true;
    }, false);
}

std::string ConsumerUI::indicatorTooltip(Indicator indicator) const {
    std::ostringstream text;
    if (indicator == Indicator::LeftGlove || indicator == Indicator::RightGlove) {
        const int hand = indicator == Indicator::LeftGlove ? 0 : 1;
        const AdvancedPair *pair = nullptr;
        if (advancedMode) {
            const auto it = advancedPairs.find(activePairId);
            if (it != advancedPairs.end()) pair = it->second.get();
        }
        const std::string &serial = advancedMode
            ? (pair ? (hand == 0 ? pair->leftSerial : pair->rightSerial) : simpleSerials[hand])
            : simpleSerials[hand];
        const UdState state = pair ? (hand == 0 ? pair->leftState.load() : pair->rightState.load())
                                   : (advancedMode ? UD_INIT_STATE_INIT
                                                   : (hand == 0 ? mLeftState.load() : mRightState.load()));
        const bool ready = pair ? (hand == 0 ? pair->leftReady.load() : pair->rightReady.load())
                                : (!advancedMode && (hand == 0 ? leftReady.load() : rightReady.load()));
        const double fps = pair ? pair->telemetry[hand].frameRate :
                                  (advancedMode ? 0.0 : simpleTelemetry[hand].frameRate);
        const auto &telemetry = pair ? pair->telemetry[hand] : simpleTelemetry[hand];
        const int batteryLevel = telemetry.batteryLevel.load(std::memory_order_relaxed);
        const int batteryRaw = telemetry.batteryRaw.load(std::memory_order_relaxed);
        const int rssiDbm = telemetry.rssiDbm.load(std::memory_order_relaxed);
        text << (hand == 0 ? _("Left glove") : _("Right glove"))
             << '\n' << _("Serial number") << ": " << (serial.empty() ? _("Unknown") : serial)
             << '\n' << _("Connection") << ": "
             << (state == UD_INIT_STATE_CONNECTED || state == UD_INIT_STATE_LINKED
                     ? _("Connected") : _("Disconnected"))
             << '\n' << _("Calibration") << ": " << (ready ? _("Ready") : _("Required"))
             << '\n' << _("Battery") << ": ";
        if (batteryLevel > 0 && state != UD_INIT_STATE_NOT_CONNECT) {
            text << batteryLevel << "/5 (" << _("raw") << ' ' << batteryRaw << ')';
        } else {
            text << _("Unknown");
        }
        text
             << '\n' << _("Signal strength") << ": ";
        if (rssiDbm != 0 && state != UD_INIT_STATE_NOT_CONNECT) text << rssiDbm << " dBm";
        else text << _("Unknown");
        text
             << '\n' << _("Frame rate") << ": " << std::fixed << std::setprecision(1)
             << (state == UD_INIT_STATE_CONNECTED || state == UD_INIT_STATE_LINKED ? fps : 0.0)
             << ' ' << _("fps");
        return text.str();
    }

    auto &config = UserConfig::getInstance();
    if (advancedMode && (indicator == Indicator::VMC || indicator == Indicator::OSC)) {
        const char *name = indicator == Indicator::VMC ? "VMC" : "VRChat OSC";
        const char *suffix = indicator == Indicator::VMC ? "/vmc" : "/osc";
        const int defaultPort = indicator == Indicator::VMC ? 39540 : 9000;
        int enabledCount = 0;
        int sendingCount = 0;
        text << name;
        for (const auto &id : savedPairIds) {
            const std::string prefix = "/enterprise/pairs/" + id + suffix;
            if (!config.get<bool>(prefix + "/enabled", false)) continue;
            ++enabledCount;
            const auto it = advancedPairs.find(id);
            const auto pair = it == advancedPairs.end() ? nullptr : it->second;
            const bool sending = pair && (indicator == Indicator::VMC ? static_cast<bool>(pair->vmcSender)
                                                                      : static_cast<bool>(pair->oscSender));
            if (sending) ++sendingCount;
            const auto serials = decodePairId(id);
            if (!serials) continue;
            text << '\n' << serials->first << " / " << serials->second << " → "
                 << config.get<std::string>(prefix + "/host", "127.0.0.1") << ':'
                 << config.get<int>(prefix + "/port", defaultPort)
                 << " (" << (sending ? _("Sending") : _("Waiting for gloves")) << ')';
        }
        text << '\n' << _("Status") << ": "
             << (enabledCount == 0 ? _("Disabled") : sendingCount > 0 ? _("Sending") : _("Waiting for gloves"))
             << '\n' << _("Active pairs") << ": " << sendingCount << '/' << enabledCount;
        return text.str();
    }

    const char *name;
    const char *prefix;
    bool active;
    bool enabled;
    switch (indicator) {
        case Indicator::SteamVR:
            name = "SteamVR";
            prefix = advancedMode ? "/enterprise/vr" : "/consumer/vr";
            active = static_cast<bool>(mSteamVRSender);
            enabled = config.get<bool>(std::string(prefix) + "/enabled", false);
            break;
        case Indicator::OSC:
            name = "VRChat OSC";
            prefix = "/consumer/osc";
            active = static_cast<bool>(mOSCSender);
            enabled = config.get<bool>(std::string(prefix) + "/enabled", false);
            break;
        case Indicator::VMC:
            name = "VMC";
            prefix = "/consumer/vmc";
            active = static_cast<bool>(mVMCSender);
            enabled = config.get<bool>(std::string(prefix) + "/enabled", false);
            break;
        case Indicator::QingTong:
            name = "QingTong UDP";
            prefix = advancedMode ? "/enterprise/udCapQingTong" : "/consumer/udCapQingTong";
            active = static_cast<bool>(mUdCapQTSender);
            enabled = config.get<bool>(std::string(prefix) + "/enabled", false);
            break;
        default:
            return {};
    }
    text << name << '\n' << _("Status") << ": "
         << (!enabled ? _("Disabled") : active ? _("Sending") : _("Waiting for gloves"));
    if (advancedMode && indicator != Indicator::QingTong) {
        const std::string id = indicator == Indicator::SteamVR
            ? config.get<std::string>("/enterprise/vr/pairId", "none") : activePairId;
        const auto it = advancedPairs.find(id);
        text << '\n' << _("Selected pair") << ": "
             << (it == advancedPairs.end() ? _("None") : it->second->leftSerial + " / " + it->second->rightSerial);
    } else if (!advancedMode) {
        text << '\n' << _("Glove pair") << ": "
             << (simpleSerials[0].empty() && simpleSerials[1].empty()
                     ? _("None") : (simpleSerials[0].empty() ? _("Unknown") : simpleSerials[0]) +
                                      " / " + (simpleSerials[1].empty() ? _("Unknown") : simpleSerials[1]));
    }
    if (indicator != Indicator::SteamVR) {
        const int defaultPort = indicator == Indicator::VMC ? 39540 : indicator == Indicator::OSC ? 9000 : 6666;
        text << '\n' << _("Destination") << ": "
             << config.get<std::string>(std::string(prefix) + "/host", "127.0.0.1") << ':'
             << config.get<int>(std::string(prefix) + "/port", defaultPort);
    }
    return text.str();
}

bool ConsumerUI::sampleFrameRates() {
    syncOptiTrackSender();
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - lastTelemetrySample).count();
    if (elapsed <= 0.0) return true;
    const auto sample = [elapsed](GloveTelemetry &telemetry) {
        telemetry.frameRate = static_cast<double>(telemetry.frames.exchange(0, std::memory_order_relaxed)) / elapsed;
    };
    for (auto &telemetry : simpleTelemetry) sample(telemetry);
    for (auto &[id, pair] : advancedPairs)
        for (auto &telemetry : pair->telemetry) sample(telemetry);
    lastTelemetrySample = now;
    if (advancedMode && UserConfig::getInstance().get<bool>(
            "/enterprise/udCapQingTong/enabled", false)) setupUdcapQTSender();
    if (advancedMode && !selectedGloveSerial.empty()) refreshAdvancedDetail();
    // GTK queries the tooltip on this display, including the other indicators.
    mLeftHand.trigger_tooltip_query();
    return true;
}

void ConsumerUI::syncOptiTrackSender() {
    if (!UserConfig::getInstance().get<bool>(
            advancedMode ? "/enterprise/optitrack/enabled" : "/consumer/optitrack/enabled", false)) {
        mOptiTrackSender.reset();
        optiTrackSenderErrorShown = false;
        return;
    }
    try {
        if (!mOptiTrackSender) mOptiTrackSender = std::make_unique<OptiTrackSender>();
        std::set<UdCapV1Core *> active;
        if (mHandCoreLeft) active.insert(mHandCoreLeft.get());
        if (mHandCoreRight) active.insert(mHandCoreRight.get());
        mOptiTrackSender->add(mHandCoreLeft, simpleSerials[0]);
        mOptiTrackSender->add(mHandCoreRight, simpleSerials[1]);
        for (const auto &[serial, core] : standaloneCores) {
            if (core) active.insert(core.get());
            mOptiTrackSender->add(core, serial);
        }
        for (const auto &[id, pair] : advancedPairs) {
            if (pair->left) active.insert(pair->left.get());
            if (pair->right) active.insert(pair->right.get());
            mOptiTrackSender->add(pair->left, pair->leftSerial);
            mOptiTrackSender->add(pair->right, pair->rightSerial);
        }
        mOptiTrackSender->retainOnly(active);
        optiTrackSenderErrorShown = false;
    } catch (const std::exception &error) {
        mOptiTrackSender.reset();
        if (!optiTrackSenderErrorShown) {
            mDescription.set_text(std::string(_("OptiTrack connection failed: ")) + error.what());
            optiTrackSenderErrorShown = true;
        }
    }
}

void ConsumerUI::initializeCores() {
    const bool addLeft = leftHandSerial && !mHandCoreLeft;
    const bool addRight = rightHandSerial && !mHandCoreRight;
    if (!addLeft && !addRight) return;
    try {
        auto configuredAlgorithm = [](const char *key) {
            const int saved = UserConfig::getInstance().get<int>(key, 0);
            return algorithmFromConfig(saved);
        };
        if (addLeft) {
            mHandCoreLeft = std::make_shared<UdCapV1Core>(leftHandSerial);
            mHandCoreLeft->setFallbackTarget(UD_TARGET_LEFT_HAND);
            mHandCoreLeft->setAlgorithm(configuredAlgorithm("/consumer/algorithm/left"));
        }
        if (addRight) {
            mHandCoreRight = std::make_shared<UdCapV1Core>(rightHandSerial);
            mHandCoreRight->setFallbackTarget(UD_TARGET_RIGHT_HAND);
            mHandCoreRight->setAlgorithm(configuredAlgorithm("/consumer/algorithm/right"));
        }
    } catch (const std::exception &e) {
        mDescription.set_text(e.what());
        if (addLeft) { mHandCoreLeft.reset(); leftHandSerial.reset(); }
        if (addRight) { mHandCoreRight.reset(); rightHandSerial.reset(); }
        retryProbe = true;
        return;
    }
    if (mHandCoreLeft && mHandCoreRight) probeRunning = false;
    mStatus.set_markup(_("<span font='18' weight='bold'>Found</span>"));
    mDescription.set_text(_("Waiting for UdCap Gloves..."));
    mCalibrate.set_sensitive(false);

    auto watch = [this](const std::shared_ptr<UdCapV1Core> &core, bool left) {
        return core->listen([this, weak = std::weak_ptr<UdCapV1Core>(core), left](std::shared_ptr<UdCapV1MCUPacket> data) {
            if (data->commandType == CMD_DATA) {
                simpleTelemetry[left ? 0 : 1].frames.fetch_add(1, std::memory_order_relaxed);
                return;
            }
            if (data->commandType == CMD_BATTERY) {
                auto &telemetry = simpleTelemetry[left ? 0 : 1];
                telemetry.batteryLevel.store(data->battery, std::memory_order_relaxed);
                telemetry.batteryRaw.store(data->batteryRaw, std::memory_order_relaxed);
                telemetry.rssiDbm.store(data->rssiDbm, std::memory_order_relaxed);
                return;
            }
            if (data->commandType == CMD_LINK_STATE) {
                if (data->udState == UD_INIT_STATE_NOT_CONNECT) {
                    simpleTelemetry[left ? 0 : 1].batteryLevel.store(0, std::memory_order_relaxed);
                    simpleTelemetry[left ? 0 : 1].batteryRaw.store(0, std::memory_order_relaxed);
                    simpleTelemetry[left ? 0 : 1].rssiDbm.store(0, std::memory_order_relaxed);
                }
                if (left) mLeftState = data->udState;
                else mRightState = data->udState;
                if (data->udState == UD_INIT_STATE_LINKED) {
                    if (auto current = weak.lock()) {
                        try { current->tryRestoreHandCalibration(); }
                        catch (const std::exception &) { /* Manual calibration remains available. */ }
                    }
                } else if (data->udState == UD_INIT_STATE_NOT_CONNECT) {
                    if (left) leftReady = false;
                    else rightReady = false;
                    allReady();
                }
                runOnUIThread([this, left, state = data->udState]() {
                    auto &icon = left ? mLeftHand : mRightHand;
                    icon.setState(state == UD_INIT_STATE_LINKED ? StatusIndicator::State::Active
                                                                 : StatusIndicator::State::Waiting);
                    return false;
                });
                initConnectReceiver();
            } else if (data->commandType == CMD_READY) {
                if (left) leftReady = data->isReady;
                else rightReady = data->isReady;
                allReady();
#ifndef NO_3DPREVIEW
            } else if (data->commandType == CMD_SKELETON_QUATERNION) {
                if (!mPreviewActive.load(std::memory_order_relaxed)) return;
                std::lock_guard lock(previewMutex);
                if (left) {
                    previewLeft = data->skeletonQuaternion;
                    hasPreviewLeft = true;
                } else {
                    previewRight = data->skeletonQuaternion;
                    hasPreviewRight = true;
                }
#endif
            }
        });
    };
    if (addLeft) {
        unlistenLeft = watch(mHandCoreLeft, true);
        mHandCoreLeft->mcuGetLinkState();
    }
    if (addRight) {
        unlistenRight = watch(mHandCoreRight, false);
        mHandCoreRight->mcuGetLinkState();
    }
}

std::string ConsumerUI::pairId(const std::string &left, const std::string &right) {
    auto encode = [](const std::string &serial) {
        constexpr char digits[] = "0123456789abcdef";
        std::string encoded;
        encoded.reserve(serial.size() * 2);
        for (const unsigned char byte : serial) {
            encoded.push_back(digits[byte >> 4]);
            encoded.push_back(digits[byte & 15]);
        }
        return encoded;
    };
    return encode(left) + "-" + encode(right);
}

std::optional<std::pair<std::string, std::string>> ConsumerUI::decodePairId(const std::string &id) {
    const auto separator = id.find('-');
    if (separator == std::string::npos || separator == 0 || separator + 1 >= id.size() ||
        separator % 2 != 0 || (id.size() - separator - 1) % 2 != 0) return std::nullopt;
    auto decode = [](const std::string &encoded) -> std::optional<std::string> {
        auto nibble = [](const char ch) -> int {
            if (ch >= '0' && ch <= '9') return ch - '0';
            if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            return -1;
        };
        std::string result;
        result.reserve(encoded.size() / 2);
        for (std::size_t i = 0; i < encoded.size(); i += 2) {
            const int high = nibble(encoded[i]);
            const int low = nibble(encoded[i + 1]);
            if (high < 0 || low < 0) return std::nullopt;
            result.push_back(static_cast<char>((high << 4) | low));
        }
        return result;
    };
    auto left = decode(id.substr(0, separator));
    auto right = decode(id.substr(separator + 1));
    if (!left || !right || !left->ends_with('L') || !right->ends_with('R')) return std::nullopt;
    return std::pair{*left, *right};
}

void ConsumerUI::savePairAssignments() {
    std::string assignments;
    for (const auto &id : savedPairIds) {
        if (!assignments.empty()) assignments += ',';
        assignments += id;
    }
    UserConfig::getInstance().set<std::string>("/enterprise/assignments", assignments);
    UserConfig::getInstance().save();
}

void ConsumerUI::saveExcludedAutoPairs() {
    std::string exclusions;
    for (const auto &id : excludedAutoPairs) {
        if (!exclusions.empty()) exclusions += ',';
        exclusions += id;
    }
    UserConfig::getInstance().set<std::string>("/enterprise/excludedAutoPairs", exclusions);
    UserConfig::getInstance().save();
}

void ConsumerUI::restoreSavedPairs() {
    if (!advancedMode) return;
    for (const auto &id : savedPairIds) {
        if (advancedPairs.contains(id) || excludedAutoPairs.contains(id)) continue;
        const auto serials = decodePairId(id);
        if (serials && availableReceivers.contains(serials->first) &&
            availableReceivers.contains(serials->second))
            addAdvancedPair(serials->first, serials->second);
    }
}

void ConsumerUI::refreshAdvancedSelectors() {
    if (!advancedMode) return;
    refreshAdvancedGlobalCalibration();
    const std::string previousLeft = mAdvancedLeft.get_active_id().raw();
    const std::string previousRight = mAdvancedRight.get_active_id().raw();
    updatingAdvancedSelectors = true;
    mAdvancedLeft.remove_all();
    mAdvancedRight.remove_all();
    mSavedPairSelector.remove_all();
    mAdvancedActivePair.remove_all();
    for (const auto &[serial, accessor] : availableReceivers) {
        if (serial.ends_with('L')) mAdvancedLeft.append(serial, serial);
        if (serial.ends_with('R')) mAdvancedRight.append(serial, serial);
    }
    for (const auto &[id, pair] : advancedPairs) {
        const std::string name = pair->leftSerial + " / " + pair->rightSerial;
        mAdvancedActivePair.append(id, name);
    }
    for (const auto &id : savedPairIds) {
        if (advancedPairs.contains(id)) continue;
        const auto serials = decodePairId(id);
        if (serials) mSavedPairSelector.append(id, serials->first + " / " + serials->second);
    }
    if (!previousLeft.empty()) mAdvancedLeft.set_active_id(previousLeft);
    if (!previousRight.empty()) mAdvancedRight.set_active_id(previousRight);
    if (mAdvancedLeft.get_active_id().empty()) mAdvancedLeft.set_active(0);
    if (mAdvancedRight.get_active_id().empty()) mAdvancedRight.set_active(0);
    if (advancedPairs.contains(activePairId)) mAdvancedActivePair.set_active_id(activePairId);
    else if (!advancedPairs.empty()) mAdvancedActivePair.set_active(0);
    mForgetSavedPair.set_sensitive(false);
    updatingAdvancedSelectors = false;
    selectAdvancedPair(mAdvancedActivePair.get_active_id().raw());
    refreshAdvancedDeviceList();
    refreshAdvancedDetail();
    if (mAdvancedPages.get_visible_child_name() == "firmware") refreshAdvancedFirmware();
    setupUdcapQTSender();
}

void ConsumerUI::refreshAdvancedGlobalCalibration() {
    if (!advancedMode) return;
    const bool anyLinked = std::any_of(advancedPairs.begin(), advancedPairs.end(),
        [](const auto &entry) {
            const auto &pair = entry.second;
            return pair->leftState == UD_INIT_STATE_LINKED &&
                   pair->rightState == UD_INIT_STATE_LINKED;
        });
    mAdvancedCalibrateAll.set_sensitive(anyLinked);
}

void ConsumerUI::showAdvancedPage(const std::string &page) {
    if (!advancedMode) return;
    mAdvancedPages.set_visible_child(page);
    if (page == "firmware") refreshAdvancedFirmware();
    for (const auto &[name, button] : mAdvancedNavButtons) {
        if (name == page) button->add_css_class("suggested-action");
        else button->remove_css_class("suggested-action");
    }
#ifndef NO_3DPREVIEW
    updatePreviewVisibility();
#endif
}

void ConsumerUI::queueAdvancedDeviceListRefresh() {
    const std::weak_ptr<std::atomic_bool> life = alive;
    Glib::signal_idle().connect([this, life]() {
        const auto current = life.lock();
        if (current && current->load()) refreshAdvancedDeviceList();
        return false;
    });
}

void ConsumerUI::refreshAdvancedDeviceList() {
    if (!advancedMode) return;
    while (auto *child = mAdvancedDeviceList.get_first_child())
        mAdvancedDeviceList.remove(*child);

    const auto makeGroupMenu = [](const char *actionLabel, std::function<void()> action) {
        auto menu = Gtk::make_managed<Gtk::MenuButton>();
        menu->set_label("⋯");
        menu->set_tooltip_text(actionLabel);
        menu->add_css_class("flat");
        menu->set_size_request(36, 30);
        auto popover = Gtk::make_managed<Gtk::Popover>();
        auto item = Gtk::make_managed<Gtk::Button>(actionLabel);
        item->add_css_class("flat");
        item->set_margin(6);
        item->signal_clicked().connect([popover, action = std::move(action)]() {
            popover->popdown();
            action();
        });
        popover->set_child(*item);
        menu->set_popover(*popover);
        return menu;
    };

    for (const auto &[id, pair] : advancedPairs) {
        auto frame = Gtk::make_managed<Gtk::Frame>();
        auto grid = Gtk::make_managed<Gtk::Grid>();
        grid->set_column_homogeneous(true);
        grid->set_row_spacing(4);
        grid->set_margin(6);
        auto header = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
        auto title = Gtk::make_managed<Gtk::Label>(_("Glove group"));
        title->set_halign(Gtk::Align::START);
        title->set_hexpand(true);
        header->append(*title);
        header->append(*makeGroupMenu(_("Dissolve group"), [this, id]() {
            scheduleRemoveAdvancedPair(id);
        }));
        grid->attach(*header, 0, 0, 3, 1);
        auto group = Gtk::make_managed<Gtk::Button>(pair->leftSerial + " / " + pair->rightSerial);
        group->set_tooltip_text(_("Show group details"));
        if (selectedGloveSerial.empty() && selectedSavedPairId.empty() && activePairId == id)
            group->add_css_class("suggested-action");
        group->signal_clicked().connect([this, id]() {
            selectedGloveSerial.clear();
            selectedSavedPairId.clear();
            mAdvancedActivePair.set_active_id(id);
            queueAdvancedDeviceListRefresh();
            refreshAdvancedDetail();
            showAdvancedPage("devices");
        });
        grid->attach(*group, 0, 1, 3, 1);
        int row = 2;
        for (const auto &serial : {pair->leftSerial, pair->rightSerial}) {
            auto glove = Gtk::make_managed<Gtk::Button>(serial);
            if (selectedGloveSerial == serial) glove->add_css_class("suggested-action");
            glove->signal_clicked().connect([this, serial]() { selectAdvancedGlove(serial); });
            grid->attach(*glove, 1, row++, 2, 1);
        }
        frame->set_child(*grid);
        mAdvancedDeviceList.append(*frame);
    }

    for (const auto &id : savedPairIds) {
        if (advancedPairs.contains(id)) continue;
        const auto serials = decodePairId(id);
        if (!serials) continue;
        auto frame = Gtk::make_managed<Gtk::Frame>();
        auto content = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
        content->set_margin(6);
        auto header = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
        auto title = Gtk::make_managed<Gtk::Label>(_("Glove group"));
        title->set_halign(Gtk::Align::START);
        title->set_hexpand(true);
        header->append(*title);
        header->append(*makeGroupMenu(_("Forget saved pair"), [this, id]() {
            const std::weak_ptr<std::atomic_bool> life = alive;
            Glib::signal_timeout().connect([this, life, id]() {
                const auto current = life.lock();
                if (current && current->load()) forgetSavedPair(id);
                return false;
            }, 1);
        }));
        content->append(*header);
        auto button = Gtk::make_managed<Gtk::Button>(serials->first + " / " + serials->second);
        button->set_tooltip_text(_("Saved pair disconnected"));
        if (selectedSavedPairId == id) button->add_css_class("suggested-action");
        button->signal_clicked().connect([this, id]() {
            selectedGloveSerial.clear();
            selectedSavedPairId = id;
            queueAdvancedDeviceListRefresh();
            refreshAdvancedDetail();
            showAdvancedPage("devices");
        });
        content->append(*button);
        frame->set_child(*content);
        mAdvancedDeviceList.append(*frame);
    }

    bool hasUnpaired = false;
    for (const auto &[serial, accessor] : availableReceivers) {
        bool grouped = false;
        for (const auto &[id, pair] : advancedPairs)
            if (pair->leftSerial == serial || pair->rightSerial == serial) { grouped = true; break; }
        if (grouped) continue;
        if (!hasUnpaired) {
            auto title = Gtk::make_managed<Gtk::Label>(_("Ungrouped gloves"));
            title->set_halign(Gtk::Align::START);
            title->add_css_class("heading");
            mAdvancedDeviceList.append(*title);
            hasUnpaired = true;
        }
        auto glove = Gtk::make_managed<Gtk::Button>(serial);
        if (selectedGloveSerial == serial) glove->add_css_class("suggested-action");
        glove->signal_clicked().connect([this, serial]() { selectAdvancedGlove(serial); });
        mAdvancedDeviceList.append(*glove);
    }
    if (advancedPairs.empty() && savedPairIds.empty() && !hasUnpaired) {
        auto empty = Gtk::make_managed<Gtk::Label>(_("No receivers found"));
        empty->set_wrap(true);
        mAdvancedDeviceList.append(*empty);
    }
}

void ConsumerUI::selectAdvancedGlove(const std::string &serial) {
    if (!availableReceivers.contains(serial)) return;
    selectedGloveSerial = serial;
    selectedSavedPairId.clear();
    for (const auto &[id, pair] : advancedPairs) {
        if (pair->leftSerial == serial || pair->rightSerial == serial) {
            mAdvancedActivePair.set_active_id(id);
            break;
        }
    }
    queueAdvancedDeviceListRefresh();
    refreshAdvancedDetail();
    showAdvancedPage("devices");
}

std::shared_ptr<UdCapV1Core> ConsumerUI::coreForSerial(const std::string &serial) {
    for (const auto &[id, pair] : advancedPairs) {
        if (pair->leftSerial == serial) return pair->left;
        if (pair->rightSerial == serial) return pair->right;
    }
    if (const auto it = standaloneCores.find(serial); it != standaloneCores.end()) return it->second;
    const auto receiver = availableReceivers.find(serial);
    if (receiver == availableReceivers.end()) return nullptr;
    try {
        auto core = std::make_shared<UdCapV1Core>(receiver->second);
        core->setFallbackTarget(serial.ends_with('L') ? UD_TARGET_LEFT_HAND : UD_TARGET_RIGHT_HAND);
        const int configured = UserConfig::getInstance().get<int>(gloveConfigPrefix(serial) + "/algorithm", 0);
        core->setAlgorithm(algorithmFromConfig(configured));
        standaloneCores.emplace(serial, core);
        return core;
    } catch (const std::exception &e) {
        mDescription.set_text(e.what());
        return nullptr;
    }
}

void ConsumerUI::confirmRestartGloves(const std::vector<std::shared_ptr<UdCapV1Core>> &cores,
                                      const std::string &description) {
    std::vector<std::shared_ptr<UdCapV1Core>> connected;
    for (const auto &core : cores) {
        if (!core) continue;
        const auto state = core->getState();
        if (state == UD_INIT_STATE_CONNECTED || state == UD_INIT_STATE_LINKED)
            connected.push_back(core);
    }
    if (connected.empty()) {
        Gtk::AlertDialog::create(_("No connected gloves to restart."))->show(*this);
        return;
    }

    auto dialog = Gtk::AlertDialog::create(connected.size() == 1
        ? _("Restart this glove?") : _("Restart these gloves?"));
    dialog->set_detail(description + "\n" + _("Data transmission will stop and resume after 5 seconds."));
    dialog->set_buttons({_("Cancel"), _("Restart")});
    dialog->set_default_button(1);
    dialog->set_cancel_button(0);
    const std::weak_ptr<std::atomic_bool> life = alive;
    dialog->choose(*this, [this, dialog, life, connected = std::move(connected)]
                   (Glib::RefPtr<Gio::AsyncResult> &result) {
        const auto current = life.lock();
        if (!current || !current->load()) return;
        try {
            if (dialog->choose_finish(result) != 1) return;
        } catch (const std::exception &) {
            return;
        }
        std::vector<std::weak_ptr<UdCapV1Core>> stopped;
        for (const auto &core : connected) {
            const std::weak_ptr<UdCapV1Core> weak = core;
            if (restartingCores.contains(weak)) continue;
            try {
                core->mcuStopData();
                restartingCores.insert(weak);
                stopped.push_back(weak);
            } catch (const std::exception &e) {
                mDescription.set_text(e.what());
            }
        }
        if (stopped.empty()) return;
        Glib::signal_timeout().connect([this, life, stopped = std::move(stopped)]() {
            const auto current = life.lock();
            if (!current || !current->load()) return false;
            for (const auto &weak : stopped) {
                restartingCores.erase(weak);
                if (const auto core = weak.lock()) {
                    try { core->mcuResumeData(); }
                    catch (const std::exception &e) { mDescription.set_text(e.what()); }
                }
            }
            return false;
        }, 5000);
    });
}

std::vector<std::shared_ptr<UdCapV1Core>> ConsumerUI::selectedAdvancedCores() {
    if (advancedMode && !selectedSavedPairId.empty()) return {nullptr, nullptr};
    if (!advancedMode || selectedGloveSerial.empty()) return {mHandCoreLeft, mHandCoreRight};
    for (const auto &[id, pair] : advancedPairs) {
        if (pair->leftSerial == selectedGloveSerial) return {pair->left, nullptr};
        if (pair->rightSerial == selectedGloveSerial) return {nullptr, pair->right};
    }
    return selectedGloveSerial.ends_with('L')
        ? std::vector<std::shared_ptr<UdCapV1Core>>{coreForSerial(selectedGloveSerial), nullptr}
        : std::vector<std::shared_ptr<UdCapV1Core>>{nullptr, coreForSerial(selectedGloveSerial)};
}

void ConsumerUI::refreshAdvancedFirmware() {
    if (!advancedMode) return;
    std::vector<std::shared_ptr<UdCapV1Core>> cores;
    for (const auto &[serial, accessor] : availableReceivers) {
        if (auto core = coreForSerial(serial)) cores.push_back(core);
    }
    mAdvancedFirmwarePanel.setCores(std::move(cores));
}

void ConsumerUI::loadSavedPairSettings(const std::string &id) {
    constexpr std::array<const char *, 2> suffixes = {"/vmc", "/osc"};
    constexpr std::array<int, 2> defaultPorts = {39540, 9000};
    auto &config = UserConfig::getInstance();
    for (std::size_t index = 0; index < mSavedOutputs.size(); ++index) {
        const std::string prefix = "/enterprise/pairs/" + id + suffixes[index];
        auto &controls = mSavedOutputs[index];
        controls.enabled->set_active(config.get<bool>(prefix + "/enabled", false));
        controls.host->set_text(config.get<std::string>(prefix + "/host", "127.0.0.1"));
        controls.port->set_text(std::to_string(config.get<int>(prefix + "/port", defaultPorts[index])));
        setOutputStatus(controls.status, "");
    }
    loadedSavedPairId = id;
}

bool ConsumerUI::vmcDestinationInUse(const std::string &exceptId,
                                      const std::string &host, int port) const {
    auto &config = UserConfig::getInstance();
    for (const auto &id : savedPairIds) {
        if (id == exceptId) continue;
        const std::string prefix = "/enterprise/pairs/" + id + "/vmc";
        if (config.get<bool>(prefix + "/enabled", false) &&
            config.get<std::string>(prefix + "/host", "127.0.0.1") == host &&
            config.get<int>(prefix + "/port", 39540) == port)
            return true;
    }
    return false;
}

void ConsumerUI::applySavedPairOutput(std::size_t index) {
    if (index >= mSavedOutputs.size()) return;
    const std::string id = selectedSavedPairId;
    if (!savedPairIds.contains(id) || advancedPairs.contains(id) || loadedSavedPairId != id) return;
    constexpr std::array<const char *, 2> suffixes = {"/vmc", "/osc"};
    auto &controls = mSavedOutputs[index];
    try {
        const std::string host = controls.host->get_text();
        boost::asio::ip::make_address_v4(host);
        const std::string text = controls.port->get_text();
        std::size_t length = 0;
        const int port = std::stoi(text, &length);
        if (length != text.size() || port < 1 || port > 65535)
            throw std::invalid_argument("port");
        if (index == 0 && controls.enabled->get_active() && vmcDestinationInUse(id, host, port)) {
            setOutputStatus(controls.status, _("Another pair already uses this VMC destination."));
            return;
        }
        const std::string prefix = "/enterprise/pairs/" + id + suffixes[index];
        auto &config = UserConfig::getInstance();
        config.set<bool>(prefix + "/enabled", controls.enabled->get_active());
        config.set<std::string>(prefix + "/host", host);
        config.set<int>(prefix + "/port", port);
        config.save();
        setOutputStatus(controls.status, "");
        if (index == 0) setupVMCSender();
        else setupOSCSender();
    } catch (const std::exception &) {
        setOutputStatus(controls.status, _("Enter a valid IPv4 address and port (1-65535)."));
    }
}

void ConsumerUI::refreshAdvancedDetail() {
    if (!advancedMode) return;
    if (!selectedSavedPairId.empty() && advancedPairs.contains(selectedSavedPairId))
        selectedSavedPairId.clear();
    const auto saved = decodePairId(selectedSavedPairId);
    const bool savedOnly = saved && savedPairIds.contains(selectedSavedPairId);
    if (!savedOnly) selectedSavedPairId.clear();
    const bool single = !selectedGloveSerial.empty() && availableReceivers.contains(selectedGloveSerial);
    if (!single && !selectedGloveSerial.empty()) selectedGloveSerial.clear();
    mAdvancedSingleFrame.set_visible(single);
    mAdvancedSavedFrame.set_visible(savedOnly);
    mAdvancedPairDetailFrame.set_visible(!single && !savedOnly && !activePairId.empty());
    const std::string steamVRPair = UserConfig::getInstance().get<std::string>("/enterprise/vr/pairId", "none");
    for (const auto &[id, pair] : advancedPairs) {
        pair->frame->set_visible(!single && !savedOnly && id == activePairId);
        if (pair->vrSelect) pair->vrSelect->set_sensitive(id != steamVRPair);
    }
    if (savedOnly) {
        const std::string name = saved->first + " / " + saved->second;
        if (loadedSavedPairId != selectedSavedPairId) loadSavedPairSettings(selectedSavedPairId);
        mAdvancedSavedVRSelect.set_sensitive(selectedSavedPairId != steamVRPair);
        mStatus.set_markup(_("<span font='18' weight='bold'>Glove group</span>"));
        mDescription.set_text(name);
        mAdvancedSavedFrame.set_label(name);
        mAdvancedSavedStatus.set_text(_("Saved pair disconnected"));
        mCalibrate.set_sensitive(false);
#ifndef NO_3DPREVIEW
        updatePreviewVisibility();
#endif
        return;
    }
    loadedSavedPairId.clear();
    if (!single) {
        const auto it = advancedPairs.find(activePairId);
        if (it != advancedPairs.end()) {
            const auto &pair = it->second;
            const bool linked = pair->leftState == UD_INIT_STATE_LINKED &&
                                pair->rightState == UD_INIT_STATE_LINKED;
            const bool ready = linked && pair->leftReady && pair->rightReady;
            mStatus.set_markup(ready ? _("<span font='18' weight='bold'>Ready</span>")
                : linked ? _("<span font='18' weight='bold'>Wait for Calibration</span>")
                         : _("<span font='18' weight='bold'>Found</span>"));
            mDescription.set_text(pair->leftSerial + " / " + pair->rightSerial);
            mCalibrate.set_sensitive(linked);
        } else {
            mStatus.set_markup(_("<span font='18' weight='bold'>No pair selected</span>"));
            mDescription.set_text(_("Add a glove pair to begin."));
            mCalibrate.set_sensitive(false);
        }
#ifndef NO_3DPREVIEW
        updatePreviewVisibility();
#endif
        return;
    }

    mAdvancedSingleFrame.set_label(selectedGloveSerial);
    mStatus.set_markup(_("<span font='18' weight='bold'>Glove</span>"));
    mDescription.set_text(selectedGloveSerial);
    const bool left = selectedGloveSerial.ends_with('L');
    mAdvancedSingleSide.set_text(left ? _("Left glove") : _("Right glove"));
    const std::shared_ptr<AdvancedPair> *owner = nullptr;
    for (const auto &[id, pair] : advancedPairs) {
        if (pair->leftSerial == selectedGloveSerial || pair->rightSerial == selectedGloveSerial) {
            owner = &pair;
            break;
        }
    }
    if (owner) {
        const auto &pair = *owner;
        const UdState state = left ? pair->leftState.load() : pair->rightState.load();
        const bool linked = state == UD_INIT_STATE_LINKED;
        const bool ready = left ? pair->leftReady.load() : pair->rightReady.load();
        mAdvancedSingleStatus.set_text(std::string(_("Connection")) + ": " +
            (linked ? (ready ? _("Ready") : _("Waiting for calibration"))
                    : state == UD_INIT_STATE_CONNECTED ? _("Connected") : _("Waiting for gloves")));
        mAdvancedSingleGroup.set_text(std::string(_("Glove group")) + ": " + pair->leftSerial + " / " + pair->rightSerial);
        std::ostringstream rate;
        rate << std::fixed << std::setprecision(1) << pair->telemetry[left ? 0 : 1].frameRate;
        mAdvancedSingleRate.set_text(std::string(_("Frame rate")) + ": " + rate.str() + " FPS");
        mCalibrate.set_sensitive(linked);
    } else {
        mAdvancedSingleStatus.set_text(std::string(_("Connection")) + ": " + _("Receiver detected"));
        mAdvancedSingleGroup.set_text(std::string(_("Glove group")) + ": " + _("None"));
        mAdvancedSingleRate.set_text(_("Create a glove group to read live hand data."));
        mCalibrate.set_sensitive(false);
    }
    int battery = 0;
    int batteryRaw = 0;
    int rssiDbm = 0;
    if (owner) {
        const auto &telemetry = (*owner)->telemetry[left ? 0 : 1];
        battery = telemetry.batteryLevel.load(std::memory_order_relaxed);
        batteryRaw = telemetry.batteryRaw.load(std::memory_order_relaxed);
        rssiDbm = telemetry.rssiDbm.load(std::memory_order_relaxed);
    }
    mAdvancedSingleSignal.set_text(std::string(_("Battery")) + ": " +
        (battery > 0 ? std::to_string(battery) + "/5 (" + _("raw") + " " +
                       std::to_string(batteryRaw) + ")"
                      : _("Unknown")) + "  ·  " + _("Signal strength") + ": " +
        (rssiDbm != 0 ? std::to_string(rssiDbm) + " dBm" : _("Unknown")));
#ifndef NO_3DPREVIEW
    updatePreviewVisibility();
#endif
}

void ConsumerUI::tryAutoPair() {
    if (!advancedMode || !mAutoPair.get_active()) return;
    std::vector<std::pair<std::string, std::string>> candidates;
    for (const auto &[serial, accessor] : availableReceivers) {
        if (!serial.ends_with('L')) continue;
        std::string right = serial;
        right.back() = 'R';
        if (!availableReceivers.contains(right)) continue;
        const auto id = pairId(serial, right);
        bool reserved = false;
        for (const auto &saved : savedPairIds) {
            if (saved == id) continue;
            const auto assignment = decodePairId(saved);
            if (assignment && (assignment->first == serial || assignment->second == right)) {
                reserved = true;
                break;
            }
        }
        if (reserved) continue;
        if (!advancedPairs.contains(id) && !excludedAutoPairs.contains(id))
            candidates.emplace_back(serial, right);
    }
    for (const auto &[left, right] : candidates) addAdvancedPair(left, right);
}

bool ConsumerUI::addAdvancedPair(const std::string &left, const std::string &right) {
    if (!advancedMode || !left.ends_with('L') || !right.ends_with('R') ||
        !availableReceivers.contains(left) || !availableReceivers.contains(right)) {
        mDescription.set_text(_("Choose an available left and right receiver."));
        return false;
    }
    for (const auto &[id, pair] : advancedPairs) {
        if (pair->leftSerial == left || pair->rightSerial == right) {
            mDescription.set_text(_("A receiver is already assigned to another pair."));
            return false;
        }
    }
    auto pair = std::make_shared<AdvancedPair>();
    pair->id = pairId(left, right);
    pair->leftSerial = left;
    pair->rightSerial = right;
    try {
        pair->left = coreForSerial(left);
        pair->right = coreForSerial(right);
        if (!pair->left || !pair->right) throw std::runtime_error(_("Could not open receiver."));
        const std::string algorithmPrefix = "/enterprise/pairs/" + pair->id + "/algorithm";
        const auto algorithm = [&algorithmPrefix](const std::string &serial, const char *hand) {
            auto &config = UserConfig::getInstance();
            const int legacy = UserConfig::getInstance().get<int>(algorithmPrefix + hand, 0);
            const std::string key = gloveConfigPrefix(serial) + "/algorithm";
            const int saved = config.get<int>(key, -1);
            if (saved == -1 && legacy != 0) {
                config.set<int>(key, static_cast<int>(algorithmFromConfig(legacy)));
                config.save();
            }
            const int selected = saved == -1 ? legacy : saved;
            return algorithmFromConfig(selected);
        };
        pair->left->setAlgorithm(algorithm(left, "/left"));
        pair->right->setAlgorithm(algorithm(right, "/right"));
    } catch (const std::exception &e) {
        mDescription.set_text(e.what());
        return false;
    }

    const std::string config = "/enterprise/pairs/" + pair->id + "/vmc";
    pair->frame = Gtk::make_managed<Gtk::Frame>(left + " / " + right);
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    box->set_margin(8);
    auto heading = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    pair->status = Gtk::make_managed<Gtk::Label>(_("Waiting for gloves"));
    pair->status->set_hexpand(true);
    pair->status->set_halign(Gtk::Align::START);
    auto calibrate = Gtk::make_managed<Gtk::Button>(_("Calibrate group"));
    calibrate->set_sensitive(false);
    pair->calibrate = calibrate;
    auto restart = Gtk::make_managed<Gtk::Button>(_("Restart gloves"));
    restart->signal_clicked().connect([this, id = pair->id]() {
        const auto it = advancedPairs.find(id);
        if (it == advancedPairs.end()) return;
        const auto &current = it->second;
        confirmRestartGloves({current->left, current->right},
                             current->leftSerial + " / " + current->rightSerial);
    });
    heading->append(*pair->status);
    heading->append(*restart);
    heading->append(*calibrate);
    box->append(*heading);
    auto settingsRow = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto controller = Gtk::make_managed<Gtk::Button>(_("Controller"));
    auto hands = Gtk::make_managed<Gtk::Button>(_("Hand Settings"));
    pair->vrSelect = Gtk::make_managed<Gtk::Button>(_("Use as SteamVR gloves"));
    controller->signal_clicked().connect([this, id = pair->id]() {
        mAdvancedActivePair.set_active_id(id);
        selectedGloveSerial.clear();
        activate_action("win.settings.controller");
    });
    hands->signal_clicked().connect([this, id = pair->id]() {
        mAdvancedActivePair.set_active_id(id);
        selectedGloveSerial.clear();
        activate_action("win.settings.hands");
    });
    pair->vrSelect->signal_clicked().connect([this, id = pair->id]() {
        UserConfig::getInstance().set<std::string>("/enterprise/vr/pairId", id);
        UserConfig::getInstance().save();
        setupVRSender();
        refreshAdvancedDetail();
    });
    settingsRow->append(*controller);
    settingsRow->append(*hands);
    settingsRow->append(*pair->vrSelect);
    box->append(*settingsRow);

    auto outputGrid = Gtk::make_managed<Gtk::Grid>();
    outputGrid->set_column_spacing(8);
    outputGrid->set_row_spacing(4);
    box->append(*outputGrid);
    pair->vmcEnabled = Gtk::make_managed<Gtk::CheckButton>(_("VMC"));
    pair->vmcEnabled->set_active(UserConfig::getInstance().get<bool>(config + "/enabled", false));
    pair->vmcHost = Gtk::make_managed<Gtk::Entry>();
    pair->vmcHost->set_width_chars(15);
    pair->vmcHost->set_text(UserConfig::getInstance().get<std::string>(config + "/host", "127.0.0.1"));
    pair->vmcPort = Gtk::make_managed<Gtk::Entry>();
    pair->vmcPort->set_width_chars(6);
    pair->vmcPort->set_text(std::to_string(UserConfig::getInstance().get<int>(config + "/port",
        39540 + static_cast<int>(advancedPairs.size()))));
    auto vmcApply = Gtk::make_managed<Gtk::Button>(_("Apply"));
    pair->vmcStatus = Gtk::make_managed<Gtk::Label>();
    attachOutputRow(*outputGrid, 0, *pair->vmcEnabled, *pair->vmcHost, *pair->vmcPort,
                    *vmcApply, *pair->vmcStatus);
    const auto addOutputRow = [this, &pair, outputGrid](int rowIndex,
                                                   const char *label, const char *suffix, int defaultPort,
                                                   Gtk::CheckButton *&enabled, Gtk::Entry *&host,
                                                   Gtk::Entry *&port, Gtk::Label *&status) {
        const std::string outputConfig = "/enterprise/pairs/" + pair->id + suffix;
        enabled = Gtk::make_managed<Gtk::CheckButton>(label);
        enabled->set_active(UserConfig::getInstance().get<bool>(outputConfig + "/enabled", false));
        host = Gtk::make_managed<Gtk::Entry>();
        host->set_width_chars(15);
        host->set_text(UserConfig::getInstance().get<std::string>(outputConfig + "/host", "127.0.0.1"));
        port = Gtk::make_managed<Gtk::Entry>();
        port->set_width_chars(6);
        port->set_text(std::to_string(UserConfig::getInstance().get<int>(outputConfig + "/port", defaultPort)));
        auto apply = Gtk::make_managed<Gtk::Button>(_("Apply"));
        status = Gtk::make_managed<Gtk::Label>();
        attachOutputRow(*outputGrid, rowIndex, *enabled, *host, *port, *apply, *status);
        const std::weak_ptr<AdvancedPair> weak = pair;
        apply->signal_clicked().connect([this, weak, outputConfig, enabled, host, port, status]() {
            const auto current = weak.lock();
            if (!current) return;
            try {
                const std::string address = host->get_text();
                boost::asio::ip::make_address_v4(address);
                const std::string text = port->get_text();
                std::size_t length = 0;
                const int value = std::stoi(text, &length);
                if (length != text.size() || value < 1 || value > 65535)
                    throw std::invalid_argument("port");
                auto &config = UserConfig::getInstance();
                config.set<bool>(outputConfig + "/enabled", enabled->get_active());
                config.set<std::string>(outputConfig + "/host", address);
                config.set<int>(outputConfig + "/port", value);
                config.save();
                current->oscSender.reset();
                setupOSCSender();
            } catch (const std::exception &) {
                setOutputStatus(status, _("Enter a valid IPv4 address and port (1-65535)."));
            }
        });
    };
    addOutputRow(1, "VRChat OSC", "/osc", 9000, pair->oscEnabled, pair->oscHost,
                 pair->oscPort, pair->oscStatus);
    pair->frame->set_child(*box);
    mAdvancedPairsBox.append(*pair->frame);
    advancedPairs[pair->id] = pair;
    if (excludedAutoPairs.erase(pair->id) != 0) saveExcludedAutoPairs();
    bool assignmentsChanged = false;
    for (auto it = savedPairIds.begin(); it != savedPairIds.end();) {
        const auto serials = decodePairId(*it);
        if (*it != pair->id && serials &&
            (serials->first == left || serials->second == right)) {
            it = savedPairIds.erase(it);
            assignmentsChanged = true;
        } else ++it;
    }
    assignmentsChanged = savedPairIds.insert(pair->id).second || assignmentsChanged;
    if (assignmentsChanged) savePairAssignments();

    const std::weak_ptr<AdvancedPair> weak = pair;
    calibrate->signal_clicked().connect([this, weak]() {
        if (const auto current = weak.lock()) {
            if (current->leftState != UD_INIT_STATE_LINKED ||
                current->rightState != UD_INIT_STATE_LINKED) return;
            mCalibrationCores = {current->left, current->right};
            mCalibrationUI = std::make_unique<CalibrationUI>(mCalibrationCores);
            mCalibrationUI->set_transient_for(*this);
            mCalibrationUI->show();
        }
    });
    vmcApply->signal_clicked().connect([this, weak, config]() {
        const auto current = weak.lock();
        if (!current) return;
        try {
            const std::string host = current->vmcHost->get_text();
            boost::asio::ip::make_address_v4(host);
            const std::string text = current->vmcPort->get_text();
            std::size_t length = 0;
            const int port = std::stoi(text, &length);
            if (length != text.size() || port < 1 || port > 65535)
                throw std::invalid_argument("port");
            if (current->vmcEnabled->get_active() && vmcDestinationInUse(current->id, host, port)) {
                setOutputStatus(current->vmcStatus, _("Another pair already uses this VMC destination."));
                return;
            }
            auto &preferences = UserConfig::getInstance();
            const bool enabled = current->vmcEnabled->get_active();
            const bool changed = preferences.get<bool>(config + "/enabled", false) != enabled ||
                                 preferences.get<std::string>(config + "/host", "127.0.0.1") != host ||
                                 preferences.get<int>(config + "/port", 39540) != port;
            if (changed) {
                preferences.set<bool>(config + "/enabled", enabled);
                preferences.set<std::string>(config + "/host", host);
                preferences.set<int>(config + "/port", port);
                preferences.save();
                current->vmcSender.reset();
            }
            setOutputStatus(current->vmcStatus, "");
            if (changed || (enabled && !current->vmcSender)) setupVMCSender();
        } catch (const std::exception &) {
            setOutputStatus(current->vmcStatus, _("Enter a valid IPv4 address and port (1-65535)."));
        }
    });

    auto watch = [this, weak](const std::shared_ptr<UdCapV1Core> &core, bool isLeft) {
        return core->listen([this, weak, core, isLeft](std::shared_ptr<UdCapV1MCUPacket> data) {
            const auto current = weak.lock();
            if (!current) return;
            if (data->commandType == CMD_DATA) {
                current->telemetry[isLeft ? 0 : 1].frames.fetch_add(1, std::memory_order_relaxed);
                return;
            }
            if (data->commandType == CMD_BATTERY) {
                auto &telemetry = current->telemetry[isLeft ? 0 : 1];
                telemetry.batteryLevel.store(data->battery, std::memory_order_relaxed);
                telemetry.batteryRaw.store(data->batteryRaw, std::memory_order_relaxed);
                telemetry.rssiDbm.store(data->rssiDbm, std::memory_order_relaxed);
                return;
            }
#ifndef NO_3DPREVIEW
            if (data->commandType == CMD_SKELETON_QUATERNION) {
                if (!mPreviewActive.load(std::memory_order_relaxed)) return;
                std::lock_guard lock(current->poseMutex);
                if (isLeft) {
                    current->leftPose = data->skeletonQuaternion;
                    current->hasLeftPose = true;
                } else {
                    current->rightPose = data->skeletonQuaternion;
                    current->hasRightPose = true;
                }
                return;
            }
#endif
            if (data->commandType == CMD_LINK_STATE) {
                if (data->udState == UD_INIT_STATE_NOT_CONNECT) {
                    current->telemetry[isLeft ? 0 : 1].batteryLevel.store(0, std::memory_order_relaxed);
                    current->telemetry[isLeft ? 0 : 1].batteryRaw.store(0, std::memory_order_relaxed);
                    current->telemetry[isLeft ? 0 : 1].rssiDbm.store(0, std::memory_order_relaxed);
                }
                if (isLeft) current->leftState = data->udState;
                else current->rightState = data->udState;
                if (data->udState == UD_INIT_STATE_LINKED) {
                    try { core->tryRestoreHandCalibration(); }
                    catch (const std::exception &) {}
                } else {
                    if (isLeft) current->leftReady = false;
                    else current->rightReady = false;
                }
            } else if (data->commandType == CMD_READY) {
                if (isLeft) current->leftReady = data->isReady;
                else current->rightReady = data->isReady;
            } else return;
            const bool linkChanged = data->commandType == CMD_LINK_STATE;
            runOnUIThread([this, weak, linkChanged]() {
                const auto p = weak.lock();
                if (!p || !advancedPairs.contains(p->id) || advancedPairs.at(p->id) != p) return false;
                updateAdvancedPairStatus(p);
                if (linkChanged) {
                    setupOSCSender();
                    setupUdcapQTSender();
                }
                return false;
            });
        });
    };
    pair->unlistenLeft = watch(pair->left, true);
    pair->unlistenRight = watch(pair->right, false);
    refreshAdvancedSelectors();
    setupAdvancedPairVMC(pair);
    setupOSCSender();
    setupUdcapQTSender();
    try {
        pair->left->mcuGetLinkState();
        pair->right->mcuGetLinkState();
    } catch (const std::exception &e) {
        pair->status->set_text(e.what());
    }
    return true;
}

void ConsumerUI::scheduleRemoveAdvancedPair(const std::string &id) {
    const std::weak_ptr<std::atomic_bool> life = alive;
    Glib::signal_timeout().connect([this, life, id]() {
        const auto current = life.lock();
        if (current && current->load()) removeAdvancedPair(id);
        return false;
    }, 1);
}

void ConsumerUI::forgetSavedPair(const std::string &id) {
    if (id.empty() || !savedPairIds.contains(id) || advancedPairs.contains(id)) return;
    // Forgetting a group must not let automatic pairing recreate it on the next scan.
    excludedAutoPairs.insert(id);
    saveExcludedAutoPairs();
    savedPairIds.erase(id);
    savePairAssignments();
    if (UserConfig::getInstance().get<std::string>("/enterprise/vr/pairId", "none") == id) {
        UserConfig::getInstance().set<std::string>("/enterprise/vr/pairId", "none");
        UserConfig::getInstance().save();
    }
    if (selectedSavedPairId == id) selectedSavedPairId.clear();
    refreshAdvancedSelectors();
    setupVMCSender();
    setupOSCSender();
    setupUdcapQTSender();
    setupVRSender();
}

void ConsumerUI::removeAdvancedPair(const std::string &id) {
    const auto it = advancedPairs.find(id);
    if (it == advancedPairs.end()) return;
    auto pair = it->second;
    excludedAutoPairs.insert(id);
    saveExcludedAutoPairs();
    if (savedPairIds.erase(id) != 0) savePairAssignments();
    if (selectedGloveSerial == pair->leftSerial || selectedGloveSerial == pair->rightSerial)
        selectedGloveSerial.clear();
    if (selectedSavedPairId == id) selectedSavedPairId.clear();
    if (std::find(mCalibrationCores.begin(), mCalibrationCores.end(), pair->left) != mCalibrationCores.end() ||
        std::find(mCalibrationCores.begin(), mCalibrationCores.end(), pair->right) != mCalibrationCores.end()) {
        mCalibrationUI.reset();
        mCalibrationCores.clear();
    }
    if (activeVRSenderPairId == id) {
        mSteamVRSender.reset();
        activeVRSenderPairId.clear();
    }
    pair->vmcSender.reset();
    pair->oscSender.reset();
    if (pair->unlistenLeft) pair->unlistenLeft();
    if (pair->unlistenRight) pair->unlistenRight();
    if (activePairId == id) {
        mOSCSender.reset();
        mHandCoreLeft.reset();
        mHandCoreRight.reset();
    }
    if (UserConfig::getInstance().get<std::string>("/enterprise/vr/pairId", "none") == id) {
        UserConfig::getInstance().set<std::string>("/enterprise/vr/pairId", "none");
        UserConfig::getInstance().save();
    }
    mAdvancedPairsBox.remove(*pair->frame);
    advancedPairs.erase(it);
    refreshAdvancedSelectors();
    setupVMCSender();
    setupOSCSender();
    setupUdcapQTSender();
    setupVRSender();
}

void ConsumerUI::selectAdvancedPair(const std::string &id) {
    if (!advancedMode) return;
    if (id == activePairId) return;
    mFirmwareDialog.reset();
    mHandSettingsDialog.reset();
    mControllerSettingsDialog.reset();
    mDataTransferDialog.reset();
    const auto it = advancedPairs.find(id);
    activePairId = it == advancedPairs.end() ? "" : id;
#ifndef NO_3DPREVIEW
    if (mThreeppContext) {
        mThreeppContext->setHandPose(UD_TARGET_LEFT_HAND, HandQuaternion{});
        mThreeppContext->setHandPose(UD_TARGET_RIGHT_HAND, HandQuaternion{});
    }
#endif
    mHandCoreLeft = it == advancedPairs.end() ? nullptr : it->second->left;
    mHandCoreRight = it == advancedPairs.end() ? nullptr : it->second->right;
    if (it == advancedPairs.end()) {
        mLeftState = UD_INIT_STATE_INIT;
        mRightState = UD_INIT_STATE_INIT;
        leftReady = false;
        rightReady = false;
        mStatus.set_markup(_("<span font='18' weight='bold'>No pair selected</span>"));
        mDescription.set_text(_("Add a glove pair to begin."));
        mCalibrate.set_sensitive(false);
        mLeftHand.setState(StatusIndicator::State::Off);
        mRightHand.setState(StatusIndicator::State::Off);
    } else {
        updateAdvancedPairStatus(it->second);
    }
    setupOSCSender();
    setupUdcapQTSender();
    refreshAdvancedDetail();
}

void ConsumerUI::updateAdvancedPairStatus(const std::shared_ptr<AdvancedPair> &pair) {
    const bool linked = pair->leftState == UD_INIT_STATE_LINKED && pair->rightState == UD_INIT_STATE_LINKED;
    const bool ready = linked && pair->leftReady && pair->rightReady;
    pair->status->set_text(ready ? _("Ready") : linked ? _("Waiting for calibration") : _("Waiting for gloves"));
    if (pair->calibrate) pair->calibrate->set_sensitive(linked);
    refreshAdvancedGlobalCalibration();
    setupAdvancedPairVMC(pair);
    if (activePairId == pair->id) {
        mLeftState = pair->leftState.load();
        mRightState = pair->rightState.load();
        leftReady = pair->leftReady.load();
        rightReady = pair->rightReady.load();
        mStatus.set_markup(ready ? _("<span font='18' weight='bold'>Ready</span>")
            : linked ? _("<span font='18' weight='bold'>Wait for Calibration</span>")
                     : _("<span font='18' weight='bold'>Found</span>"));
        mDescription.set_text(pair->leftSerial + " / " + pair->rightSerial);
        mCalibrate.set_sensitive(linked);
        mLeftHand.setState(pair->leftState == UD_INIT_STATE_LINKED ? StatusIndicator::State::Active
                                                                   : StatusIndicator::State::Waiting);
        mRightHand.setState(pair->rightState == UD_INIT_STATE_LINKED ? StatusIndicator::State::Active
                                                                     : StatusIndicator::State::Waiting);
    }
    setupVRSender();
    refreshAdvancedDetail();
}

void ConsumerUI::setupAdvancedPairVMC(const std::shared_ptr<AdvancedPair> &pair) {
    const std::string config = "/enterprise/pairs/" + pair->id + "/vmc";
    const bool enabled = UserConfig::getInstance().get<bool>(config + "/enabled", false);
    if (!enabled) {
        pair->vmcSender.reset();
        setOutputStatus(pair->vmcStatus, "");
    } else if (pair->leftState != UD_INIT_STATE_LINKED || pair->rightState != UD_INIT_STATE_LINKED) {
        pair->vmcSender.reset();
        setOutputStatus(pair->vmcStatus, "");
    } else if (!pair->vmcSender) {
        try {
            const std::string host = UserConfig::getInstance().get<std::string>(config + "/host", "127.0.0.1");
            const int port = UserConfig::getInstance().get<int>(config + "/port", 39540);
            if (port < 1 || port > 65535) throw std::out_of_range("VMC port");
            for (const auto &[id, other] : advancedPairs) {
                if (id == pair->id || !other->vmcSender) continue;
                const std::string otherConfig = "/enterprise/pairs/" + id + "/vmc";
                if (UserConfig::getInstance().get<std::string>(otherConfig + "/host", "127.0.0.1") == host &&
                    UserConfig::getInstance().get<int>(otherConfig + "/port", 39540) == port) {
                    throw std::runtime_error(_("Another pair already uses this VMC destination."));
                }
            }
            pair->vmcSender = std::make_unique<VMCSender>(host, static_cast<uint16_t>(port));
            pair->vmcSender->add(pair->left, pair->left->getTarget());
            pair->vmcSender->add(pair->right, pair->right->getTarget());
            setOutputStatus(pair->vmcStatus, "");
        } catch (const std::exception &e) {
            pair->vmcSender.reset();
            setOutputStatus(pair->vmcStatus, e.what());
        }
    }
    bool anySending = false;
    bool anyEnabled = false;
    for (const auto &id : savedPairIds)
        anyEnabled |= UserConfig::getInstance().get<bool>(
            "/enterprise/pairs/" + id + "/vmc/enabled", false);
    for (const auto &[id, current] : advancedPairs) {
        anySending |= static_cast<bool>(current->vmcSender);
    }
    mVMC.setState(anySending ? StatusIndicator::State::Active
                             : anyEnabled ? StatusIndicator::State::Waiting : StatusIndicator::State::Off);
}

void ConsumerUI::stopGloveStreams() {
    if (gloveStreamsStopped) return;
    gloveStreamsStopped = true;
    *alive = false;
    probeRunning = false;
    if (probeThread.joinable()) probeThread.join();

    std::vector<std::shared_ptr<UdCapV1Core>> cores;
    std::set<UdCapV1Core *> seen;
    const auto addCore = [&cores, &seen](const std::shared_ptr<UdCapV1Core> &core) {
        if (core && seen.insert(core.get()).second) cores.push_back(core);
    };
    addCore(mHandCoreLeft);
    addCore(mHandCoreRight);
    for (const auto &[serial, core] : standaloneCores) addCore(core);
    for (const auto &[id, pair] : advancedPairs) {
        addCore(pair->left);
        addCore(pair->right);
    }
    for (const auto &core : mCalibrationCores) addCore(core);

    for (const auto &core : cores) {
        try { core->stopStreamingForShutdown(); }
        catch (const std::exception &e) { std::cerr << "Could not stop glove: " << e.what() << std::endl; }
    }
    for (const auto &core : cores) {
        if (!core->waitForPendingWrites(std::chrono::milliseconds(2000)))
            std::cerr << "Timed out waiting for glove stop packet" << std::endl;
    }
}

ConsumerUI::~ConsumerUI() {
    stopGloveStreams();
    *alive = false;
    if (advancedMode) mAdvancedAddDialog.hide();
    mTelemetryTimer.disconnect();
    mSteamVRHapticTimer.disconnect();
#ifndef NO_3DPREVIEW
    releasePreview();
#endif
    OSCServer::getInstance().setCallback(nullptr);
    probeRunning = false;
    if (probeThread.joinable()) probeThread.join();
    mOptiTrackSender.reset();
    mSteamVRSender.reset();
    mVMCSender.reset();
    mOSCSender.reset();
    mUdCapQTSender.reset();
    for (auto &[id, pair] : advancedPairs) {
        pair->vmcSender.reset();
        pair->oscSender.reset();
        if (pair->unlistenLeft) pair->unlistenLeft();
        if (pair->unlistenRight) pair->unlistenRight();
    }
    advancedPairs.clear();
    if (unlistenLeft) unlistenLeft();
    if (unlistenRight) unlistenRight();
}

void ConsumerUI::runOnUIThread(std::function<bool(void)> callEvent) {
    const std::weak_ptr<std::atomic_bool> life = alive;
    // MainContext::invoke() may run inline on a worker thread when it can
    // acquire the context. Always dispatch through an idle source so Core's
    // packet callback never performs GTK work or registers another listener
    // while holding Core's callback mutex.
    auto source = Glib::IdleSource::create();
    source->connect([life, callEvent = std::move(callEvent)]() {
        const auto current = life.lock();
        if (!current || !current->load()) return false;
        return callEvent();
    });
    source->attach(Glib::MainContext::get_default());
}

void ConsumerUI::allReady() {
    runOnUIThread([this]() {
        if (leftReady && rightReady) {
            mStatus.set_markup(_("<span font='18' weight='bold'>Ready</span>"));
            mDescription.set_text(_("UdCap running normally."));
        } else {
            mStatus.set_markup(_("<span font='18' weight='bold'>Found</span>"));
            mDescription.set_text(_("Waiting for UdCap Gloves..."));
            if (!leftReady) {
                mLeftHand.setState(StatusIndicator::State::Waiting);
            }
            if (!rightReady) {
                mRightHand.setState(StatusIndicator::State::Waiting);
            }
        };
        return false;
    });
    setupVMCSender();
    setupOSCSender();
    setupUdcapQTSender();
    setupVRSender();
}

void ConsumerUI::buildMenu() {
    mSettingsMenu = Gio::Menu::create();
    mSettingsMenu->append(_("Controller"), "win.settings.controller");
    mSettingsMenu->append(_("Hand Settings"), "win.settings.hands");
    mSettingsMenu->append(_("Data Transfer"),
                          "win.settings.data_transfer");
    mSettingsMenu->append(_("SteamVR Tracker Offsets"), "win.settings.steamvr_tracking");
    mSettingsMenu->append(_("Firmware"), "win.settings.firmware");
    mSettingsMenu->append(_("Preference"), "app.settings.preference");

    auto actionFirmware = Gio::SimpleAction::create("settings.firmware");
    actionFirmware->signal_activate().connect([this](const Glib::VariantBase&) {
        if (advancedMode) {
            showAdvancedPage("firmware");
            return;
        }
        const auto cores = selectedAdvancedCores();
        mFirmwareDialog = std::make_unique<FirmwareDialog>(cores);
        mFirmwareDialog->set_transient_for(*this);
        mFirmwareDialog->show();
    });
    add_action(actionFirmware);
    auto actionDataTransfer = Gio::SimpleAction::create("settings.data_transfer");
    actionDataTransfer->signal_activate().connect([this](const Glib::VariantBase&) {
        if (advancedMode) {
            showAdvancedPage("devices");
            return;
        }
        mDataTransferDialog = std::make_unique<DataTransferDialog>("/consumer", [this](bool enable, std::string host, uint16_t port) {
            UserConfig::getInstance().set<bool>("/consumer/vmc/enabled", enable);
            UserConfig::getInstance().set<std::string>("/consumer/vmc/host", host);
            UserConfig::getInstance().set<int>("/consumer/vmc/port", port);
            UserConfig::getInstance().save();
            setupVMCSender();
        },[this](bool enable, std::string host, uint16_t port) {
            UserConfig::getInstance().set<bool>("/consumer/osc/enabled", enable);
            UserConfig::getInstance().set<std::string>("/consumer/osc/host", host);
            UserConfig::getInstance().set<int>("/consumer/osc/port", port);
            UserConfig::getInstance().save();
            setupOSCSender();
        },[this](bool enable, std::string host, uint16_t port) {
            UserConfig::getInstance().set<bool>("/consumer/udCapQingTong/enabled", enable);
            UserConfig::getInstance().set<std::string>("/consumer/udCapQingTong/host", host);
            UserConfig::getInstance().set<int>("/consumer/udCapQingTong/port", port);
            UserConfig::getInstance().save();
            setupUdcapQTSender();
        },[this](bool enable) {
            UserConfig::getInstance().set<bool>("/consumer/vr/enabled", enable);
            UserConfig::getInstance().save();
            setupVRSender();
        }, !advancedMode, !advancedMode);
        mDataTransferDialog->set_transient_for(*this);
        mDataTransferDialog->show();
    });
    add_action(actionDataTransfer);
    auto actionHands = Gio::SimpleAction::create("settings.hands");
    actionHands->signal_activate().connect([this](const Glib::VariantBase &) {
        const auto cores = selectedAdvancedCores();
        if (advancedMode && !cores[0] && !cores[1]) {
            mDescription.set_text(_("Select a glove pair first."));
            showAdvancedPage("devices");
            return;
        }
        mHandSettingsDialog = std::make_unique<HandSettingsDialog>(
            cores, [&]() {
                std::vector<std::string> keys(2);
                if (advancedMode) {
                    const auto pair = advancedPairs.find(activePairId);
                    for (std::size_t i = 0; i < 2; ++i) {
                        if (!cores[i]) continue;
                        const std::string serial = !selectedGloveSerial.empty() ? selectedGloveSerial
                            : pair != advancedPairs.end()
                                ? (i == 0 ? pair->second->leftSerial : pair->second->rightSerial)
                                : std::string{};
                        if (!serial.empty()) keys[i] = gloveConfigPrefix(serial) + "/algorithm";
                    }
                } else {
                    keys = {"/consumer/algorithm/left", "/consumer/algorithm/right"};
                }
                return keys;
            }());
        mHandSettingsDialog->set_transient_for(*this);
        mHandSettingsDialog->show();
    });
    add_action(actionHands);
    auto actionController = Gio::SimpleAction::create("settings.controller");
    actionController->signal_activate().connect([this](const Glib::VariantBase &) {
        const auto cores = selectedAdvancedCores();
        if (advancedMode && !cores[0] && !cores[1]) {
            mDescription.set_text(_("Select a glove pair first."));
            showAdvancedPage("devices");
            return;
        }
        mControllerSettingsDialog = std::make_unique<ControllerSettingsDialog>(
            cores);
        mControllerSettingsDialog->set_transient_for(*this);
        mControllerSettingsDialog->show();
    });
    add_action(actionController);
    auto actionTracking = Gio::SimpleAction::create("settings.steamvr_tracking");
    actionTracking->signal_activate().connect([this](const Glib::VariantBase &) {
        if (advancedMode && (selectedGloveSerial.empty() || !availableReceivers.contains(selectedGloveSerial))) {
            mDescription.set_text(_("Select a glove first."));
            showAdvancedPage("devices");
            return;
        }
        mSteamVRTrackingDialog = std::make_unique<SteamVRTrackingDialog>([this]() {
            if (mSteamVRSender) mSteamVRSender->reloadTrackingOffsets();
        }, advancedMode ? selectedGloveSerial : std::string{},
           advancedMode ? (selectedGloveSerial.ends_with('L') ? 0 : 1) : -1);
        mSteamVRTrackingDialog->set_transient_for(*this);
        mSteamVRTrackingDialog->show();
    });
    add_action(actionTracking);
    auto actionPreference = Gio::SimpleAction::create("settings.preference");
    actionPreference->signal_activate().connect([this](const Glib::VariantBase&) {
        if (advancedMode) {
            showAdvancedPage("preferences");
            return;
        }
        mPreferenceDialog = std::make_unique<PreferenceDialog>([this]() { setupVRSender(); });
        mPreferenceDialog->set_transient_for(*this);
        mPreferenceDialog->set_modal(true);
        mPreferenceDialog->show();
    });
    get_application()->add_action(actionPreference);
}

void ConsumerUI::setupVMCSender() {
    if (advancedMode) {
        runOnUIThread([this]() {
            for (const auto &[id, pair] : advancedPairs) setupAdvancedPairVMC(pair);
            bool anyEnabled = false;
            bool anySending = false;
            for (const auto &id : savedPairIds)
                anyEnabled |= UserConfig::getInstance().get<bool>(
                    "/enterprise/pairs/" + id + "/vmc/enabled", false);
            for (const auto &[id, pair] : advancedPairs)
                anySending |= static_cast<bool>(pair->vmcSender);
            mVMC.setState(anySending ? StatusIndicator::State::Active
                                     : anyEnabled ? StatusIndicator::State::Waiting : StatusIndicator::State::Off);
            return false;
        });
        return;
    }
    runOnUIThread([this]() {
        bool enable = UserConfig::getInstance().get<bool>("/consumer/vmc/enabled", false);
        std::string host = UserConfig::getInstance().get<std::string>("/consumer/vmc/host", "127.0.0.1");
        int port = UserConfig::getInstance().get<int>("/consumer/vmc/port", 39540);
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mVMCSender.reset();
                mVMC.setState(StatusIndicator::State::Waiting);
                return false;
            }
            if (mVMCSender) {
                mVMCSender.reset(nullptr);
            }
            try {
                mVMCSender = std::make_unique<VMCSender>(host, port);
                mVMCSender->add(mHandCoreLeft, mHandCoreLeft->getTarget());
                mVMCSender->add(mHandCoreRight, mHandCoreRight->getTarget());
                mVMC.setState(StatusIndicator::State::Active);
            } catch (const std::exception &e) {
                mVMCSender.reset();
                mDescription.set_text(e.what());
                mVMC.setState(StatusIndicator::State::Waiting);
            }
        } else {
            mVMCSender.reset();
            mVMC.setState(StatusIndicator::State::Off);
        }
        return false;
    });
}

void ConsumerUI::setupOSCSender() {
    if (advancedMode) {
        runOnUIThread([this]() {
            bool anyEnabled = false;
            bool anySending = false;
            for (const auto &id : savedPairIds)
                anyEnabled |= UserConfig::getInstance().get<bool>(
                    "/enterprise/pairs/" + id + "/osc/enabled", false);
            for (const auto &[id, pair] : advancedPairs) {
                const std::string prefix = "/enterprise/pairs/" + id + "/osc";
                const bool enabled = UserConfig::getInstance().get<bool>(prefix + "/enabled", false);
                anyEnabled |= enabled;
                if (!enabled) {
                    pair->oscSender.reset();
                    setOutputStatus(pair->oscStatus, "");
                } else if (pair->leftState != UD_INIT_STATE_LINKED || pair->rightState != UD_INIT_STATE_LINKED) {
                    pair->oscSender.reset();
                    setOutputStatus(pair->oscStatus, "");
                } else {
                    try {
                        if (!pair->oscSender) {
                            const auto host = UserConfig::getInstance().get<std::string>(prefix + "/host", "127.0.0.1");
                            const int port = UserConfig::getInstance().get<int>(prefix + "/port", 9000);
                            if (port < 1 || port > 65535) throw std::out_of_range("OSC port");
                            pair->oscSender = std::make_unique<OSCSender>(host, static_cast<uint16_t>(port));
                            pair->oscSender->add(pair->left);
                            pair->oscSender->add(pair->right);
                        }
                        setOutputStatus(pair->oscStatus, "");
                        anySending = true;
                    } catch (const std::exception &e) {
                        pair->oscSender.reset();
                        setOutputStatus(pair->oscStatus, e.what());
                    }
                }
            }
            mOSC.setState(anySending ? StatusIndicator::State::Active
                                    : anyEnabled ? StatusIndicator::State::Waiting : StatusIndicator::State::Off);
            return false;
        });
        return;
    }
    runOnUIThread([this](){
        bool enable = UserConfig::getInstance().get<bool>("/consumer/osc/enabled", false);
        std::string host = UserConfig::getInstance().get<std::string>("/consumer/osc/host", "127.0.0.1");
        int port = UserConfig::getInstance().get<int>("/consumer/osc/port", 9000);
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mOSCSender.reset();
                mOSC.setState(StatusIndicator::State::Waiting);
                return false;
            }
            if (mOSCSender) {
                mOSCSender.reset(nullptr);
            }
            try {
                mOSCSender = std::make_unique<OSCSender>(host, port);
                mOSCSender->add(mHandCoreLeft);
                mOSCSender->add(mHandCoreRight);
                mOSC.setState(StatusIndicator::State::Active);
            } catch (const std::exception &e) {
                mOSCSender.reset();
                mDescription.set_text(e.what());
                mOSC.setState(StatusIndicator::State::Waiting);
            }
        } else {
            mOSCSender.reset();
            mOSC.setState(StatusIndicator::State::Off);
        }
        return false;
    });
}

void ConsumerUI::setupUdcapQTSender() {
    if (advancedMode) {
        runOnUIThread([this]() {
            const std::string prefix = "/enterprise/udCapQingTong";
            auto &config = UserConfig::getInstance();
            if (!config.get<bool>(prefix + "/enabled", false)) {
                mUdCapQTSender.reset();
                activeQtBindings.clear();
                activeQtDestination.clear();
                setOutputStatus(&mAdvancedQtStatus, "");
                mBroadcast.setState(StatusIndicator::State::Off);
                return false;
            }

            struct DeviceOutput {
                std::string serial;
                std::shared_ptr<UdCapV1Core> core;
                uint32_t deviceId;
            };
            std::vector<DeviceOutput> devices;
            std::map<std::string, std::pair<uint32_t, const UdCapV1Core *>> desired;
            std::set<std::string> grouped;
            uint32_t deviceId = 0;
            const auto include = [this, &devices, &desired](const std::string &serial,
                                                             const std::shared_ptr<UdCapV1Core> &core,
                                                             uint32_t id) {
                if (!availableReceivers.contains(serial) || !core ||
                    core->getState() != UD_INIT_STATE_LINKED) return;
                desired[serial] = {id, core.get()};
                devices.push_back({serial, core, id});
            };
            // The original QingTong sender uses one DeviceID for both hands of
            // a role and distinguishes the gloves by DeviceName.
            for (const auto &[id, pair] : advancedPairs) {
                grouped.insert(pair->leftSerial);
                grouped.insert(pair->rightSerial);
                include(pair->leftSerial, pair->left, deviceId);
                include(pair->rightSerial, pair->right, deviceId);
                ++deviceId;
            }
            for (const auto &[serial, accessor] : availableReceivers) {
                if (grouped.contains(serial)) continue;
                include(serial, coreForSerial(serial), deviceId++);
            }
            if (devices.empty()) {
                mUdCapQTSender.reset();
                activeQtBindings.clear();
                activeQtDestination.clear();
                setOutputStatus(&mAdvancedQtStatus, "");
                mBroadcast.setState(StatusIndicator::State::Waiting);
                return false;
            }
            try {
                const std::string host = config.get<std::string>(prefix + "/host", "127.0.0.1");
                const int port = config.get<int>(prefix + "/port", 6666);
                if (port < 1 || port > 65535) throw std::out_of_range("UdCapQT port");
                const std::string destination = host + ':' + std::to_string(port);
                if (!mUdCapQTSender || activeQtDestination != destination || activeQtBindings != desired) {
                    mUdCapQTSender.reset();
                    activeQtBindings.clear();
                    activeQtDestination.clear();
                    auto sender = std::make_unique<QTSender>(host, static_cast<uint16_t>(port));
                    for (const auto &device : devices)
                        sender->add(device.core, device.serial, device.deviceId);
                    mUdCapQTSender = std::move(sender);
                    activeQtBindings = std::move(desired);
                    activeQtDestination = destination;
                }
                setOutputStatus(&mAdvancedQtStatus, "");
                mBroadcast.setState(StatusIndicator::State::Active);
            } catch (const std::exception &e) {
                mUdCapQTSender.reset();
                activeQtBindings.clear();
                activeQtDestination.clear();
                setOutputStatus(&mAdvancedQtStatus, e.what());
                mBroadcast.setState(StatusIndicator::State::Waiting);
            }
            return false;
        });
        return;
    }
    runOnUIThread([this](){
        bool enable = UserConfig::getInstance().get<bool>("/consumer/udCapQingTong/enabled", false);
        std::string host = UserConfig::getInstance().get<std::string>("/consumer/udCapQingTong/host", "127.0.0.1");
        int port = UserConfig::getInstance().get<int>("/consumer/udCapQingTong/port", 6666);
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mUdCapQTSender.reset();
                mBroadcast.setState(StatusIndicator::State::Waiting);
                return false;
            }
            if (mUdCapQTSender) {
                mUdCapQTSender.reset(nullptr);
            }
            try {
                mUdCapQTSender = std::make_unique<QTSender>(host, port);
                mUdCapQTSender->add(mHandCoreLeft);
                mUdCapQTSender->add(mHandCoreRight);
                mBroadcast.setState(StatusIndicator::State::Active);
            } catch (const std::exception &e) {
                mUdCapQTSender.reset();
                mDescription.set_text(e.what());
                mBroadcast.setState(StatusIndicator::State::Waiting);
            }
        } else {
            mUdCapQTSender.reset();
            mBroadcast.setState(StatusIndicator::State::Off);
        }
        return false;
    });
}

void ConsumerUI::setupVRSender() {
    if (advancedMode) {
        runOnUIThread([this]() {
            const bool enabled = UserConfig::getInstance().get<bool>("/enterprise/vr/enabled", false);
            const std::string id = UserConfig::getInstance().get<std::string>("/enterprise/vr/pairId", "none");
            const auto it = advancedPairs.find(id);
            if (!enabled) {
                mSteamVRSender.reset();
                activeVRSenderPairId.clear();
                mVR.setState(StatusIndicator::State::Off);
            } else if (it == advancedPairs.end() ||
                       it->second->leftState != UD_INIT_STATE_LINKED ||
                       it->second->rightState != UD_INIT_STATE_LINKED) {
                mSteamVRSender.reset();
                activeVRSenderPairId.clear();
                mVR.setState(StatusIndicator::State::Waiting);
            } else {
                try {
                    if (!mSteamVRSender || activeVRSenderPairId != id) {
                        mSteamVRSender.reset();
                        activeVRSenderPairId.clear();
                        mSteamVRSender = std::make_unique<SteamVRSender>();
                        mSteamVRSender->add(it->second->left);
                        mSteamVRSender->add(it->second->right);
                        activeVRSenderPairId = id;
                    }
                    mVR.setState(StatusIndicator::State::Active);
                } catch (const std::exception &e) {
                    mSteamVRSender.reset();
                    activeVRSenderPairId.clear();
                    mDescription.set_text(e.what());
                    mVR.setState(StatusIndicator::State::Waiting);
                }
            }
            return false;
        });
        return;
    }
    runOnUIThread([this]() {
        const bool enabled = UserConfig::getInstance().get<bool>("/consumer/vr/enabled", false);
        if (!enabled) {
            mSteamVRSender.reset();
            mVR.setState(StatusIndicator::State::Off);
        } else if (!mHandCoreLeft || !mHandCoreRight ||
                   mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
            mSteamVRSender.reset();
            mVR.setState(StatusIndicator::State::Waiting);
        } else {
            try {
                if (!mSteamVRSender) {
                    mSteamVRSender = std::make_unique<SteamVRSender>();
                    mSteamVRSender->add(mHandCoreLeft);
                    mSteamVRSender->add(mHandCoreRight);
                }
                mVR.setState(StatusIndicator::State::Active);
            } catch (const std::exception &e) {
                mSteamVRSender.reset();
                mDescription.set_text(e.what());
                mVR.setState(StatusIndicator::State::Waiting);
            }
        }
        return false;
    });
}

void ConsumerUI::on_calibrate_button_clicked() {
    std::vector<std::shared_ptr<UdCapV1Core>> cores;
    for (const auto &core : selectedAdvancedCores())
        if (core) cores.push_back(core);
    if (cores.empty()) return;
    mCalibrationCores = cores;
    mCalibrationUI = std::make_unique<CalibrationUI>(cores);
    mCalibrationUI->set_transient_for(*this);
    mCalibrationUI->show();
}

void ConsumerUI::on_calibrate_all_button_clicked() {
    if (!advancedMode) return;
    std::vector<std::shared_ptr<UdCapV1Core>> cores;
    for (const auto &[id, pair] : advancedPairs) {
        if (pair->leftState != UD_INIT_STATE_LINKED ||
            pair->rightState != UD_INIT_STATE_LINKED) continue;
        cores.push_back(pair->left);
        cores.push_back(pair->right);
    }
    if (cores.empty()) return;
    mCalibrationCores = std::move(cores);
    mCalibrationUI = std::make_unique<CalibrationUI>(mCalibrationCores);
    mCalibrationUI->set_transient_for(*this);
    mCalibrationUI->show();
}

void ConsumerUI::initConnectReceiver() {
    runOnUIThread([this](){
        if (mLeftState == UD_INIT_STATE_LINKED && mRightState == UD_INIT_STATE_LINKED) {
            mStatus.set_markup(_("<span font='18' weight='bold'>Wait for Calibration</span>"));
            mDescription.set_text(_("UdCap Gloves are waiting for calibration."));
            mCalibrate.set_sensitive(true);
        } else {
            mStatus.set_markup(_("<span font='18' weight='bold'>Found</span>"));
            mDescription.set_text(_("Waiting for UdCap Gloves..."));
            mCalibrate.set_sensitive(false);
        }
        return false;
    });
}

#ifndef NO_3DPREVIEW
void ConsumerUI::updatePreviewVisibility() {
    const bool wanted = mPreviewToggle.get_active() &&
        (!advancedMode || (!activePairId.empty() && selectedGloveSerial.empty() &&
                           selectedSavedPairId.empty() &&
                           mAdvancedPages.get_visible_child_name() == "devices"));
    if (!wanted) {
        releasePreview();
        return;
    }
    if (mGLArea) return;

    auto *area = Gtk::make_managed<Gtk::GLArea>();
    mGLArea = area;
    area->set_hexpand(true);
    area->set_vexpand(true);
    area->set_auto_render(false);
    mGLRealizeConnection = area->signal_realize().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_realize));
    mGLUnrealizeConnection = area->signal_unrealize().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_unrealize), false);
    mGLRenderConnection = area->signal_render().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_render), false);
    mGLResizeConnection = area->signal_resize().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_resize));

    auto motion = Gtk::EventControllerMotion::create();
    motion->signal_motion().connect([this](double x, double y) {
        mPreviewLastPointer = {static_cast<float>(x), static_cast<float>(y)};
        if (mThreeppContext) onMouseMoveEvent({static_cast<float>(x), static_cast<float>(y)});
    });
    area->add_controller(motion);

    auto click = Gtk::GestureClick::create();
    click->set_button(0);
    const auto mapButton = [](unsigned int button) {
        return button == 1 ? 0 : button == 3 ? 1 : button == 2 ? 2 : -1;
    };
    // The signal is owned by click; capturing its RefPtr here would keep the
    // controller (and potentially its GLArea) alive after hiding the preview.
    click->signal_pressed().connect([this, controller = click.get(), mapButton](int, double x, double y) {
        if (!mThreeppContext) return;
        const int button = mapButton(controller->get_current_button());
        mPreviewLastPointer = {static_cast<float>(x), static_cast<float>(y)};
        if (mPreviewPressedButton >= 0)
            onMousePressedEvent(mPreviewPressedButton, {mPreviewLastPointer[0], mPreviewLastPointer[1]}, MouseAction::RELEASE);
        mPreviewPressedButton = button;
        if (button >= 0)
            onMousePressedEvent(button, {static_cast<float>(x), static_cast<float>(y)}, MouseAction::PRESS);
    });
    click->signal_released().connect([this](int, double x, double y) {
        const int button = mPreviewPressedButton;
        mPreviewPressedButton = -1;
        mPreviewLastPointer = {static_cast<float>(x), static_cast<float>(y)};
        if (!mThreeppContext) return;
        if (button >= 0)
            onMousePressedEvent(button, {static_cast<float>(x), static_cast<float>(y)}, MouseAction::RELEASE);
    });
    click->signal_cancel().connect([this](Gdk::EventSequence *) {
        const int button = mPreviewPressedButton;
        mPreviewPressedButton = -1;
        if (mThreeppContext && button >= 0)
            onMousePressedEvent(button, {mPreviewLastPointer[0], mPreviewLastPointer[1]}, MouseAction::RELEASE);
    });
    area->add_controller(click);

    auto scroll = Gtk::EventControllerScroll::create();
    scroll->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
    scroll->signal_scroll().connect([this](double x, double y) {
        // GTK reports scrolling down as positive; threepp expects positive for scrolling up.
        if (mThreeppContext) onMouseWheelEvent({static_cast<float>(x), static_cast<float>(-y)});
        return true;
    }, false);
    area->add_controller(scroll);

    if (advancedMode) mAdvancedDevicePage.append(*area);
    else mMainBox.append(*area);
}

void ConsumerUI::releasePreview() {
    if (!mGLArea) return;
    mPreviewActive.store(false, std::memory_order_relaxed);
    mPreviewPressedButton = -1;
    mTimeoutRenderer.disconnect();
    if (mThreeppContext && mGLArea->get_realized()) mGLArea->make_current();
    mThreeppContext.reset();
    mGLRealizeConnection.disconnect();
    mGLUnrealizeConnection.disconnect();
    mGLRenderConnection.disconnect();
    mGLResizeConnection.disconnect();
    auto *area = mGLArea;
    mGLArea = nullptr;
    if (advancedMode) mAdvancedDevicePage.remove(*area);
    else mMainBox.remove(*area);
}

bool ConsumerUI::on_gl_render(const Glib::RefPtr<Gdk::GLContext>& context) {
    context->make_current();
    if (mThreeppContext) {
        if (advancedMode) {
            const auto it = advancedPairs.find(activePairId);
            if (it != advancedPairs.end()) {
                std::lock_guard lock(it->second->poseMutex);
                if (it->second->hasLeftPose)
                    mThreeppContext->setHandPose(UD_TARGET_LEFT_HAND, it->second->leftPose);
                if (it->second->hasRightPose)
                    mThreeppContext->setHandPose(UD_TARGET_RIGHT_HAND, it->second->rightPose);
            }
        } else {
            std::lock_guard lock(previewMutex);
            if (hasPreviewLeft) mThreeppContext->setHandPose(UD_TARGET_LEFT_HAND, previewLeft);
            if (hasPreviewRight) mThreeppContext->setHandPose(UD_TARGET_RIGHT_HAND, previewRight);
        }
        mThreeppContext->loop();
    }
    return true;
}

void ConsumerUI::on_gl_realize() {
    if (!mGLArea) return;
    mGLArea->make_current();
    mThreeppContext = std::make_unique<HandThreeppContext>(*this);
    mPreviewActive.store(true, std::memory_order_relaxed);
    mTimeoutRenderer = Glib::signal_timeout().connect([this]() {
        if (!mGLArea) return false;
        mGLArea->queue_render();
        return true;
    }, 16);
}

void ConsumerUI::on_gl_unrealize() {
    mPreviewActive.store(false, std::memory_order_relaxed);
    if (mGLArea) mGLArea->make_current();
    mTimeoutRenderer.disconnect();
    mThreeppContext.reset();
}

void ConsumerUI::on_gl_resize(int x, int y) {
    if (mThreeppContext && x > 0 && y > 0) mThreeppContext->onWindowResize(threepp::WindowSize{x, y});
}

threepp::WindowSize ConsumerUI::size() const {
    return {mGLArea ? std::max(1, mGLArea->get_width()) : 1,
            mGLArea ? std::max(1, mGLArea->get_height()) : 1};
}
#endif

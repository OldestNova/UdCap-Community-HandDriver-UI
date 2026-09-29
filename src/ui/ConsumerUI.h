//
// Created by max_3 on 2025/6/6.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_CONSUMERUI_H
#define UDCAPCOMMUNITYDRIVERUI_CONSUMERUI_H

#include <cstdint>
#include <memory>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <array>
#include <chrono>
#include <map>
#include <set>
#include <optional>
#include <utility>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#ifndef NO_3DPREVIEW
#include "../components/HandThreeppContext.h"
#endif
#include "CalibrationUI.h"

#include <UdCapProbe.h>
#include <UdCapV1Core.h>

#include "../components/OSCSender.h"
#include "../components/VMCSender.h"
#include "dialogs/DataTransferDialog.h"
#include "dialogs/FirmwareDialog.h"
#include "components/QTSender.h"
#include "dialogs/PreferenceDialog.h"
#include "dialogs/HandSettingsDialog.h"
#include "dialogs/ControllerSettingsDialog.h"
#include "components/OSCServer.h"
#include "components/SteamVRSender.h"
#include "components/OptiTrackSender.h"
#include "components/StatusIndicator.h"
#include "dialogs/SteamVRTrackingDialog.h"

class ConsumerUI: public Gtk::ApplicationWindow
#ifndef NO_3DPREVIEW
    , public threepp::PeripheralsEventSource
#endif
{
public:
    ConsumerUI(std::shared_ptr<Gtk::Application> app, bool advancedMode = false);
    ~ConsumerUI() override;
#ifndef NO_3DPREVIEW
    threepp::WindowSize size() const override;
#endif

private:
    struct GloveTelemetry {
        std::atomic<uint64_t> frames{0};
        std::atomic<int> batteryLevel{0};
        std::atomic<int> batteryRaw{0};
        std::atomic<int> rssiDbm{0};
        double frameRate = 0.0; // Updated on the UI thread.
    };

    struct AdvancedPair {
        std::string id;
        std::string leftSerial;
        std::string rightSerial;
        std::shared_ptr<UdCapV1Core> left;
        std::shared_ptr<UdCapV1Core> right;
        std::function<void()> unlistenLeft;
        std::function<void()> unlistenRight;
        std::atomic<UdState> leftState{UD_INIT_STATE_INIT};
        std::atomic<UdState> rightState{UD_INIT_STATE_INIT};
        std::atomic_bool leftReady{false};
        std::atomic_bool rightReady{false};
#ifndef NO_3DPREVIEW
        std::mutex poseMutex;
        HandQuaternion leftPose{};
        HandQuaternion rightPose{};
        bool hasLeftPose = false;
        bool hasRightPose = false;
#endif
        std::array<GloveTelemetry, 2> telemetry;
        std::unique_ptr<VMCSender> vmcSender;
        std::unique_ptr<OSCSender> oscSender;
        Gtk::Frame *frame = nullptr;
        Gtk::Label *status = nullptr;
        Gtk::Button *calibrate = nullptr;
        Gtk::CheckButton *vmcEnabled = nullptr;
        Gtk::Entry *vmcHost = nullptr;
        Gtk::Entry *vmcPort = nullptr;
        Gtk::Label *vmcStatus = nullptr;
        Gtk::CheckButton *oscEnabled = nullptr;
        Gtk::Entry *oscHost = nullptr;
        Gtk::Entry *oscPort = nullptr;
        Gtk::Label *oscStatus = nullptr;
        Gtk::Button *vrSelect = nullptr;
    };

    struct SavedOutputControls {
        Gtk::CheckButton *enabled = nullptr;
        Gtk::Entry *host = nullptr;
        Gtk::Entry *port = nullptr;
        Gtk::Label *status = nullptr;
    };

    void initConnectReceiver();
    void initializeCores();
    void stopGloveStreams();
    void allReady();
    static std::string pairId(const std::string &left, const std::string &right);
    static std::optional<std::pair<std::string, std::string>> decodePairId(const std::string &id);
    bool addAdvancedPair(const std::string &left, const std::string &right);
    void removeAdvancedPair(const std::string &id);
    void scheduleRemoveAdvancedPair(const std::string &id);
    void forgetSavedPair(const std::string &id);
    void selectAdvancedPair(const std::string &id);
    void updateAdvancedPairStatus(const std::shared_ptr<AdvancedPair> &pair);
    void setupAdvancedPairVMC(const std::shared_ptr<AdvancedPair> &pair);
    void tryAutoPair();
    void restoreSavedPairs();
    void savePairAssignments();
    void saveExcludedAutoPairs();
    void refreshAdvancedSelectors();
    void refreshAdvancedGlobalCalibration();
    void refreshAdvancedDeviceList();
    void queueAdvancedDeviceListRefresh();
    void refreshAdvancedDetail();
    void loadSavedPairSettings(const std::string &id);
    void applySavedPairOutput(std::size_t index);
    bool vmcDestinationInUse(const std::string &exceptId, const std::string &host, int port) const;
    void refreshAdvancedFirmware();
    void confirmRestartGloves(const std::vector<std::shared_ptr<UdCapV1Core>> &cores,
                              const std::string &description);
    std::shared_ptr<UdCapV1Core> coreForSerial(const std::string &serial);
    void selectAdvancedGlove(const std::string &serial);
    void showAdvancedPage(const std::string &page);
    std::vector<std::shared_ptr<UdCapV1Core>> selectedAdvancedCores();
    enum class Indicator { LeftGlove, RightGlove, SteamVR, OSC, VMC, QingTong };
    void installIndicatorTooltip(Gtk::Widget &widget, Indicator indicator);
    std::string indicatorTooltip(Indicator indicator) const;
    bool sampleFrameRates();
    Gtk::Box mMainBox;
    Gtk::Box mTopBox;
    Gtk::Box mAdvancedBox{Gtk::Orientation::VERTICAL, 8};
    Gtk::Window mAdvancedAddDialog;
    Gtk::Box mAdvancedControls{Gtk::Orientation::VERTICAL, 8};
    Gtk::CheckButton mAutoPair{_("Automatically pair matching serials")};
    Gtk::ComboBoxText mAdvancedLeft;
    Gtk::ComboBoxText mAdvancedRight;
    Gtk::Button mAdvancedConnect{_("Add selected pair")};
    Gtk::Button mAdvancedAddClose{_("Close")};
    Gtk::Label mAdvancedAddError;
    Gtk::ComboBoxText mSavedPairSelector;
    Gtk::Button mForgetSavedPair{_("Forget saved pair")};
    Gtk::ScrolledWindow mAdvancedPairsScroll;
    Gtk::Box mAdvancedPairsBox{Gtk::Orientation::VERTICAL, 8};
    Gtk::Frame mAdvancedPairDetailFrame{_("Glove group")};
    Gtk::Box mAdvancedRoot{Gtk::Orientation::HORIZONTAL, 0};
    Gtk::Box mAdvancedRail{Gtk::Orientation::VERTICAL, 6};
    Gtk::Box mAdvancedDeviceLayout{Gtk::Orientation::HORIZONTAL, 0};
    Gtk::Box mAdvancedListPane{Gtk::Orientation::VERTICAL, 8};
    Gtk::ScrolledWindow mAdvancedListScroll;
    Gtk::Box mAdvancedDeviceList{Gtk::Orientation::VERTICAL, 8};
    Gtk::Button mAdvancedCalibrateAll{_("Calibrate all")};
    Gtk::Button mAdvancedAddButton;
    Gtk::Stack mAdvancedPages;
    Gtk::Box mAdvancedDevicePage{Gtk::Orientation::VERTICAL, 8};
    Gtk::Frame mAdvancedSingleFrame{_("Glove")};
    Gtk::Button mAdvancedSingleController{_("Controller")};
    Gtk::Button mAdvancedSingleHands{_("Hand Settings")};
    Gtk::Button mAdvancedSingleTracking{_("SteamVR Tracker Offsets")};
    Gtk::Button mAdvancedSingleRestart{_("Restart glove")};
    Gtk::Label mAdvancedSingleSide;
    Gtk::Frame mAdvancedSavedFrame{_("Glove group")};
    Gtk::Label mAdvancedSavedStatus;
    Gtk::Button mAdvancedSavedVRSelect{_("Use as SteamVR gloves")};
    std::array<SavedOutputControls, 2> mSavedOutputs;
    std::string loadedSavedPairId;
    Gtk::Label mAdvancedSingleStatus;
    Gtk::Label mAdvancedSingleGroup;
    Gtk::Label mAdvancedSingleRate;
    Gtk::Label mAdvancedSingleSignal;
    std::string selectedGloveSerial;
    std::string selectedSavedPairId;
    std::vector<std::pair<std::string, Gtk::Button *>> mAdvancedNavButtons;
    Gtk::ComboBoxText mAdvancedActivePair;
    Gtk::CheckButton mAdvancedQtEnabled{_("QingTong UDP")};
    Gtk::Entry mAdvancedQtHost;
    Gtk::Entry mAdvancedQtPort;
    Gtk::Label mAdvancedQtStatus;
    Gtk::Box mAdvancedPreferenceContent{Gtk::Orientation::VERTICAL, 16};
    Gtk::ScrolledWindow mAdvancedPreferenceScroll;
    Gtk::Box mAdvancedFirmwareContent{Gtk::Orientation::VERTICAL, 8};
    FirmwarePanel mAdvancedFirmwarePanel;
    std::string activePairId;
    std::string activeVRSenderPairId;
    std::map<std::string, std::shared_ptr<AdvancedPair>> advancedPairs;
    std::set<std::string> excludedAutoPairs;
    std::set<std::string> savedPairIds;
    bool updatingAdvancedSelectors = false;
    std::map<std::string, std::shared_ptr<PortAccessor>> availableReceivers;
    std::map<std::string, std::shared_ptr<UdCapV1Core>> standaloneCores;
    std::set<std::weak_ptr<UdCapV1Core>, std::owner_less<std::weak_ptr<UdCapV1Core>>> restartingCores;
    bool advancedMode = false;
    Gtk::Box mFirstRow;
    Gtk::Box mSecondRow;
    Gtk::Box mThirdRow;
    Gtk::Box mLeftBox;
    Gtk::Box mRightBox;
#ifndef NO_3DPREVIEW
    Gtk::CheckButton mPreviewToggle{_("3D preview")};
    Gtk::GLArea *mGLArea = nullptr;
#endif
    Gtk::Label mStatus;
    Gtk::Label mDescription;
    Gtk::Button mCalibrate;
    Gtk::MenuButton mSettings;
    std::shared_ptr<Gio::Menu> mSettingsMenu;
    StatusIndicator mLeftHand{StatusIndicator::Kind::LeftGlove};
    StatusIndicator mRightHand{StatusIndicator::Kind::RightGlove};
    StatusIndicator mVR{StatusIndicator::Kind::SteamVR};
    StatusIndicator mOSC{StatusIndicator::Kind::OSC};
    StatusIndicator mVMC{StatusIndicator::Kind::VMC};
    StatusIndicator mBroadcast{StatusIndicator::Kind::QingTong};
    std::array<GloveTelemetry, 2> simpleTelemetry;
    std::array<std::string, 2> simpleSerials;
    std::chrono::steady_clock::time_point lastTelemetrySample;
    sigc::connection mTelemetryTimer;
#ifndef NO_3DPREVIEW
    std::unique_ptr<HandThreeppContext> mThreeppContext;
    std::atomic_bool mPreviewActive{false};
    std::mutex previewMutex;
    HandQuaternion previewLeft{};
    HandQuaternion previewRight{};
    bool hasPreviewLeft = false;
    bool hasPreviewRight = false;
    int mPreviewPressedButton = -1;
    std::array<float, 2> mPreviewLastPointer{};
    sigc::connection mTimeoutRenderer;
    sigc::connection mGLRealizeConnection;
    sigc::connection mGLUnrealizeConnection;
    sigc::connection mGLRenderConnection;
    sigc::connection mGLResizeConnection;
    void updatePreviewVisibility();
    void releasePreview();
#endif
    std::thread probeThread;
    std::shared_ptr<UdCapV1Core> mHandCoreLeft;
    std::shared_ptr<UdCapV1Core> mHandCoreRight;
    std::shared_ptr<PortAccessor> leftHandSerial;
    std::shared_ptr<PortAccessor> rightHandSerial;
    std::atomic<UdState> mLeftState{UD_INIT_STATE_INIT};
    std::atomic<UdState> mRightState{UD_INIT_STATE_INIT};
    std::atomic_bool leftReady{false};
    std::atomic_bool rightReady{false};
    std::atomic_bool probeRunning{true};
    bool gloveStreamsStopped = false;
    std::atomic_bool retryProbe{false};
    std::shared_ptr<std::atomic_bool> alive = std::make_shared<std::atomic_bool>(true);
    std::function<void()> unlistenLeft;
    std::function<void()> unlistenRight;
    std::unique_ptr<OSCSender> mOSCSender;
    std::unique_ptr<VMCSender> mVMCSender;
    std::unique_ptr<QTSender> mUdCapQTSender;
    std::map<std::string, std::pair<uint32_t, const UdCapV1Core *>> activeQtBindings;
    std::string activeQtDestination;
    std::unique_ptr<SteamVRSender> mSteamVRSender;
    std::unique_ptr<OptiTrackSender> mOptiTrackSender;
    bool optiTrackSenderErrorShown = false;
    void syncOptiTrackSender();
    void buildMenu();

    void runOnUIThread(std::function<bool(void)>);
    void setupVMCSender();
    void setupOSCSender();
    void setupUdcapQTSender();
    void setupVRSender();
protected:
#ifndef NO_3DPREVIEW
    bool on_gl_render(const Glib::RefPtr<Gdk::GLContext>& context);
    void on_gl_realize();
    void on_gl_unrealize();
    void on_gl_resize(int x, int y);
#endif
    void on_calibrate_button_clicked();
    void on_calibrate_all_button_clicked();
    std::unique_ptr<CalibrationUI> mCalibrationUI;
    std::vector<std::shared_ptr<UdCapV1Core>> mCalibrationCores;
    std::unique_ptr<DataTransferDialog> mDataTransferDialog;
    std::unique_ptr<FirmwareDialog> mFirmwareDialog;
    std::unique_ptr<PreferenceDialog> mPreferenceDialog;
    std::unique_ptr<HandSettingsDialog> mHandSettingsDialog;
    std::unique_ptr<ControllerSettingsDialog> mControllerSettingsDialog;
    std::unique_ptr<SteamVRTrackingDialog> mSteamVRTrackingDialog;

};


#endif //UDCAPCOMMUNITYDRIVERUI_CONSUMERUI_H

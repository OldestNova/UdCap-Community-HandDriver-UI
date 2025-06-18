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
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include "../components/HandThreeppContext.h"
#include "CalibrationUI.h"

#include <UdCapProbe.h>
#include <UdCapV1Core.h>

#include "../components/OSCSender.h"
#include "../components/VMCSender.h"
#include "dialogs/DataTransferDialog.h"
#include "components/QTSender.h"

class ConsumerUI: public Gtk::ApplicationWindow, public threepp::PeripheralsEventSource {
public:
    ConsumerUI(std::shared_ptr<Gtk::Application> app);
    ~ConsumerUI() override;
    threepp::WindowSize size() const override;

private:
    void initConnectReceiver();
    void allReady();
    std::mutex uiMutex;
    Gtk::Box mMainBox;
    Gtk::Paned mVPaned;
    Gtk::Box mTopBox;
    Gtk::Box mFirstRow;
    Gtk::Box mSecondRow;
    Gtk::Box mThirdRow;
    Gtk::Box mLeftBox;
    Gtk::Box mRightBox;
    Gtk::GLArea mGLArea;
    Gtk::Label mStatus;
    Gtk::Label mDescription;
    Gtk::Button mCalibrate;
    Gtk::MenuButton mSettings;
    std::shared_ptr<Gio::Menu> mSettingsMenu;
    Gtk::Image mLeftHand;
    Gtk::Image mRightHand;
    Gtk::Image mVR;
    Gtk::Image mOSC;
    Gtk::Image mVMC;
    Gtk::Image mBroadcast;
    std::unique_ptr<HandThreeppContext> mThreeppContext;
    sigc::connection mTimeoutRenderer;
    std::thread probeThread;
    std::shared_ptr<UdCapV1Core> mHandCoreLeft;
    std::shared_ptr<UdCapV1Core> mHandCoreRight;
    std::shared_ptr<PortAccessor> leftHandSerial;
    std::shared_ptr<PortAccessor> rightHandSerial;
    UdState mLeftState = UD_INIT_STATE_INIT;
    UdState mRightState = UD_INIT_STATE_INIT;
    bool leftReady = false;
    bool rightReady = false;
    std::unique_ptr<OSCSender> mOSCSender;
    std::unique_ptr<VMCSender> mVMCSender;
    std::unique_ptr<QTSender> mUdCapQTSender;
    void buildMenu();

    std::thread eventThread;
    std::condition_variable eventCV;
    std::queue<std::function<void()>> eventQueue;
    std::mutex eventMutex;
    std::mutex eventWaitMutex;
    bool eventRunning = true;
    void runOnUIThread(std::function<void()>);
    void setupVMCSender(bool enable, std::string host, uint16_t port);
    void setupOSCSender(bool enable, std::string host, uint16_t port);
    void setupUdcapQTSender(bool enable, std::string host, uint16_t port);
    void setupVRSender(bool enable);
protected:
    bool on_gl_render(const Glib::RefPtr<Gdk::GLContext>& context);
    void on_gl_realize();
    void on_gl_unrealize();
    void on_gl_resize(int x, int y);
    void on_calibrate_button_clicked();
    std::unique_ptr<CalibrationUI> mCalibrationUI;
    std::unique_ptr<DataTransferDialog> mDataTransferDialog;
};


#endif //UDCAPCOMMUNITYDRIVERUI_CONSUMERUI_H

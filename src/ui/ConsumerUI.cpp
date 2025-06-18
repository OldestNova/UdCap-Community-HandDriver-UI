//
// Created by max_3 on 2025/6/6.
//

#include <iostream>
#include "ConsumerUI.h"
#include "UsbEnumerate.h"

Glib::RefPtr<Gdk::Pixbuf> create_placeholder_image() {
    auto pb = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, 48, 48);
    pb->fill(0xAAAAAAFF); // 灰色占位
    return pb;
}
Glib::RefPtr<Gdk::Pixbuf> create_placeholder_green_image() {
    auto pb = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, 48, 48);
    pb->fill(0x00FF00FF); // 绿色占位
    return pb;
}
Glib::RefPtr<Gdk::Pixbuf> create_placeholder_blue_image() {
    auto pb = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, 48, 48);
    pb->fill(0x0000FFFF); // 绿色占位
    return pb;
}
ConsumerUI::ConsumerUI(std::shared_ptr<Gtk::Application> app):
    mMainBox(Gtk::Orientation::VERTICAL),
    mStatus(_("Not Connect"), Gtk::Align::START),
    mDescription(_("Please connect your UdCap gloves."), Gtk::Align::START),
    mCalibrate(_("Calibrate")),
    mSettings(),
    mSettingsMenu(),
    mVPaned(),
    mTopBox(Gtk::Orientation::VERTICAL, 5),
    mFirstRow(Gtk::Orientation::HORIZONTAL, 10),
    mSecondRow(Gtk::Orientation::HORIZONTAL, 10),
    mThirdRow(Gtk::Orientation::HORIZONTAL, 10),
    mGLArea(),
    mLeftBox(Gtk::Orientation::HORIZONTAL, 5),
    mRightBox(Gtk::Orientation::HORIZONTAL, 5),
    mLeftHand(),
    mRightHand(),
    mVR(),
    mOSC(),
    mVMC(),
    mBroadcast() {
    set_application(app);
    set_title(_("UdCap Community Driver - Consumer Edition"));
    set_default_size(600, 550);
    set_resizable(false);

    mMainBox.set_margin(10);
    set_child(mMainBox);
    mMainBox.append(mVPaned);
    // 配置垂直分割
    mVPaned.set_orientation(Gtk::Orientation::VERTICAL);
    mVPaned.set_start_child(mTopBox);
    mVPaned.set_end_child(mGLArea);
    mVPaned.set_resize_start_child(false);
    mVPaned.set_shrink_start_child(false);

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
    mSecondRow.append(mCalibrate);
    mTopBox.append(mSecondRow);

    auto separator = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
    mTopBox.append(*separator);

    mThirdRow.set_spacing(30);
    mLeftBox.set_spacing(10);
    mRightBox.set_spacing(10);

    {
        mLeftHand.set(create_placeholder_image());
        mLeftBox.append(mLeftHand);
    }
    {
        mRightHand.set(create_placeholder_image());
        mLeftBox.append(mRightHand);
    }

    {
        mVR.set(create_placeholder_image());
        mRightBox.append(mVR);
    }
    {
        mOSC.set(create_placeholder_image());
        mRightBox.append(mOSC);
    }
    {
        mVMC.set(create_placeholder_image());
        mRightBox.append(mVMC);
    }
    {
        mBroadcast.set(create_placeholder_image());
        mRightBox.append(mBroadcast);
    }
    mLeftBox.set_valign(Gtk::Align::START);
    mRightBox.set_valign(Gtk::Align::END);
    auto spacer = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    spacer->set_hexpand(true);
    mThirdRow.append(mLeftBox);
    mThirdRow.append(*spacer);
    mThirdRow.append(mRightBox);
    mTopBox.append(mThirdRow);

    mGLArea.set_vexpand(true);
    mGLArea.set_hexpand(true);
    mGLArea.set_auto_render(false);

    mGLArea.signal_realize().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_realize));
    mGLArea.signal_unrealize().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_unrealize), false);
    mGLArea.signal_render().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_render), false);
    // Resize
    mGLArea.signal_resize().connect(sigc::mem_fun(*this, &ConsumerUI::on_gl_resize));

    probeThread = std::thread([this]() {
        UsbEnumerate usbEnum;
        while (true) {
             std::map<std::string, std::shared_ptr<PortAccessor>> receiver;
             usbEnum.refresh(UsbEnumerateRefreshType::USB_ENUMERATE_REFRESH_SERIAL);
             std::vector<SerialDevice> devices = usbEnum.findPorts([](const SerialDevice &device) {
                 return device.vid == 0x1A86 && (device.pid == 0x7523 || device.pid == 0x0001);
             });
             for (const auto &device: devices) {
                 std::shared_ptr<PortAccessor> portAccessor = std::make_shared<PortAccessor>(device);
                 UdCapProbe prober(portAccessor);
                 switch (prober.probe()) {
                     case UDCAP_PROBE_HAND_V1:
                     {
                         receiver[prober.getUDCapSerial()] = portAccessor;
                         std::cout << "Found UdCap Hand V1 device: " << prober.getUDCapSerial() << std::endl;
                     }
                     default:
                     {

                     }
                 }
             }
             if (!receiver.empty()) {
                 for (const auto& recv: receiver) {
                     if (recv.first.ends_with('L') && leftHandSerial == nullptr) {
                         leftHandSerial = recv.second;
                         std::cout << "Found Left Hand Serial: " << recv.first << std::endl;
                         mLeftHand.set(create_placeholder_green_image());
                     } else if (recv.first.ends_with('R') && rightHandSerial == nullptr) {
                         rightHandSerial = recv.second;
                         std::cout << "Found Right Hand Serial: " << recv.first << std::endl;
                         mRightHand.set(create_placeholder_green_image());
                     }
                     if (leftHandSerial && rightHandSerial) {
                         std::lock_guard lk(uiMutex);
                         mHandCoreLeft = std::make_shared<UdCapV1Core>(leftHandSerial);
                         mHandCoreRight = std::make_shared<UdCapV1Core>(rightHandSerial);
                         mStatus.set_markup(_("<span font='18' weight='bold'>Found</span>"));
                         mDescription.set_text(_("Waiting for UdCap Gloves..."));
                         mCalibrate.set_sensitive(false);
                         mHandCoreLeft->listen([&](std::shared_ptr<UdCapV1MCUPacket> data) {
                             if (data->commandType == CMD_LINK_STATE) {
                                 mLeftState = data->udState;
                                switch (data->udState) {
                                    case UD_INIT_STATE_LINKED:
                                    {
                                        mLeftHand.set(create_placeholder_blue_image());
                                        break;
                                    }
                                    default:
                                    {
                                        break;
                                    }
                                }
                                initConnectReceiver();
                             } else if (data->commandType == CMD_READY) {
                                 leftReady = data->isReady;
                                 allReady();
                             }
                         });
                         mHandCoreRight->listen([&](std::shared_ptr<UdCapV1MCUPacket> data) {
                             if (data->commandType == CMD_LINK_STATE) {
                                 mRightState = data->udState;
                                 switch (data->udState) {
                                     case UD_INIT_STATE_LINKED:
                                     {
                                         mRightHand.set(create_placeholder_blue_image());
                                         break;
                                     }
                                     default:
                                     {
                                         break;
                                     }
                                 }
                                 initConnectReceiver();
                             } else if (data->commandType == CMD_READY) {
                                 rightReady = data->isReady;
                                 allReady();
                             }
                         });
                         break;
                     }
                 }
             }
             std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    probeThread.detach();

    eventThread = std::thread([this](){
        while (eventRunning) {
            if (eventQueue.empty()) {
                std::unique_lock lk(eventWaitMutex);
                eventCV.wait_for(lk, std::chrono::milliseconds(100));
                continue;
            }
            std::lock_guard lk(eventMutex);
            if (eventQueue.empty()) {
                continue;
            }
            std::function<void()> event = std::move(eventQueue.front());
            eventQueue.pop();
            try {
                std::lock_guard lku(uiMutex);
                event();
            } catch (const std::exception &e) {
                std::cerr << "Error in event: " << e.what() << std::endl;
            }
        }
    });
}

ConsumerUI::~ConsumerUI() {
    eventRunning = false;
    if (eventThread.joinable()) {
        eventThread.join();
    }
}

void ConsumerUI::runOnUIThread(std::function<void()> callEvent) {
    std::lock_guard lk(eventMutex);
    eventQueue.push(callEvent);
    eventCV.notify_all();
}

void ConsumerUI::allReady() {
    std::lock_guard lk(uiMutex);
    if (leftReady && rightReady) {
        runOnUIThread([this]() {
            mStatus.set_markup(_("<span font='18' weight='bold'>Ready</span>"));
            mDescription.set_text(_("UdCap running normally."));
        });
        // TODO ReadConfig
        setupVMCSender(true, "127.0.0.1", 39540);
//        setupOSCSender();
//        setupUdcapQTSender();
//        setupVRSender();
    }
}

void ConsumerUI::buildMenu() {
    mSettingsMenu = Gio::Menu::create();
    mSettingsMenu->append(_("Controller"), "win.settings.controller");
    mSettingsMenu->append(_("VR Settings"), "win.settings.vr");
    mSettingsMenu->append(_("Hand Settings"), "win.settings.hands");
    mSettingsMenu->append(_("Data Transfer"), "win.settings.data_transfer");
    mSettingsMenu->append(_("Preference"), "app.settings.preference");
    auto actionDataTransfer = Gio::SimpleAction::create("settings.data_transfer");
    actionDataTransfer->signal_activate().connect([this](const Glib::VariantBase&) {
        mDataTransferDialog = std::make_unique<DataTransferDialog>([this](bool enable, std::string host, uint16_t port) {
            // TODO Save
            setupVMCSender(enable, host, port);
        },[this](bool enable, std::string host, uint16_t port) {
            // TODO Save
            setupOSCSender(enable, host, port);
        },[this](bool enable, std::string host, uint16_t port) {
            // TODO Save
            setupUdcapQTSender(enable, host, port);
        },[this](bool enable) {
            // TODO
        });
        mDataTransferDialog->set_transient_for(*this);
        mDataTransferDialog->show();
    });
    add_action(actionDataTransfer);
}

void ConsumerUI::setupVMCSender(bool enable, std::string host, uint16_t port) {
    runOnUIThread([this, enable, host, port](){
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mVMC.set(create_placeholder_green_image());
                return;
            }
            if (mVMCSender) {
                mVMCSender.reset(nullptr);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            mVMCSender = std::make_unique<VMCSender>(host, port);
            mVMCSender->add(mHandCoreLeft);
            mVMCSender->add(mHandCoreRight);
            mVMC.set(create_placeholder_blue_image());
        } else {
            if (mVMCSender) {
                mVMCSender.reset(nullptr);
                mVMC.set(create_placeholder_image());
            }
        }
    });
}

void ConsumerUI::setupOSCSender(bool enable, std::string host, uint16_t port) {
    runOnUIThread([this, enable, host, port](){
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mOSC.set(create_placeholder_green_image());
                return;
            }
            if (mOSCSender) {
                mOSCSender.reset(nullptr);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            mOSCSender = std::make_unique<OSCSender>(host, port);
            mOSCSender->add(mHandCoreLeft);
            mOSCSender->add(mHandCoreRight);
            mOSC.set(create_placeholder_blue_image());
        } else {
            if (mOSCSender) {
                mOSCSender.reset(nullptr);
                mOSC.set(create_placeholder_image());
            }
        }
    });
}

void ConsumerUI::setupUdcapQTSender(bool enable, std::string host, uint16_t port) {
    runOnUIThread([this, enable, host, port](){
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mBroadcast.set(create_placeholder_green_image());
                return;
            }
            if (mUdCapQTSender) {
                mUdCapQTSender.reset(nullptr);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            mUdCapQTSender = std::make_unique<QTSender>(host, port);
            mUdCapQTSender->add(mHandCoreLeft);
            mUdCapQTSender->add(mHandCoreRight);
            mBroadcast.set(create_placeholder_blue_image());
        } else {
            if (mOSCSender) {
                mUdCapQTSender.reset(nullptr);
                mBroadcast.set(create_placeholder_image());
            }
        }
    });
}

void ConsumerUI::setupVRSender(bool enable) {

}

void ConsumerUI::on_calibrate_button_clicked() {
    std::vector<std::shared_ptr<UdCapV1Core>> cores;
    if (mHandCoreLeft) {
        cores.push_back(mHandCoreLeft);
    }
    if (mHandCoreRight) {
        cores.push_back(mHandCoreRight);
    }
    mCalibrationUI = std::make_unique<CalibrationUI>(cores);
    mCalibrationUI->set_transient_for(*this);
    mCalibrationUI->show();
}

void ConsumerUI::initConnectReceiver() {
    std::lock_guard lk(uiMutex);
    if (mLeftState == UD_INIT_STATE_LINKED && mRightState == UD_INIT_STATE_LINKED) {
        mStatus.set_markup(_("<span font='18' weight='bold'>Wait for Calibration</span>"));
        mDescription.set_text(_("UdCap Gloves are waiting for calibration."));
        mCalibrate.set_sensitive(true);
    } else {
        mStatus.set_markup(_("<span font='18' weight='bold'>Waiting</span>"));
        mDescription.set_text(_("Waiting for the other glove..."));
        mCalibrate.set_sensitive(false);
    }
}

bool ConsumerUI::on_gl_render(const Glib::RefPtr<Gdk::GLContext>& context) {
    context->make_current();
    mThreeppContext->loop();
    return true;
}

void ConsumerUI::on_gl_realize() {
    mGLArea.make_current();
    mThreeppContext = std::make_unique<HandThreeppContext>(*this);
    mTimeoutRenderer = Glib::signal_timeout().connect([this]() {
        mGLArea.queue_render();
        return true;
    }, 16);
}

void ConsumerUI::on_gl_unrealize() {
    mGLArea.make_current();
    mTimeoutRenderer.disconnect();
    mThreeppContext.reset(nullptr);
}

void ConsumerUI::on_gl_resize(int x, int y) {
    mThreeppContext->onWindowResize(threepp::WindowSize{x, y});
}

threepp::WindowSize ConsumerUI::size() const {
    auto size = mGLArea.get_size_request();
    return {size.get_width() / 4, size.get_width()};
}
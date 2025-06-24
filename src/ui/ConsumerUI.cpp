//
// Created by max_3 on 2025/6/6.
//

#include <iostream>
#include <regex>
#include "ConsumerUI.h"
#include "../components/UserConfig.h"
#include "UsbEnumerate.h"
#include "../components/Placeholder.h"

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
        std::vector<std::string> connected;
        while (true) {
             std::map<std::string, std::shared_ptr<PortAccessor>> receiver;
             usbEnum.refresh(UsbEnumerateRefreshType::USB_ENUMERATE_REFRESH_SERIAL);
             std::vector<SerialDevice> devices = usbEnum.findPorts([](const SerialDevice &device) {
                 return device.vid == 0x1A86 && (device.pid == 0x7523 || device.pid == 0x0001);
             });
             for (const auto &device: devices) {
                 if (std::find(connected.begin(), connected.end(), device.portName) != connected.end()) {
                     continue; // Already connected
                 }
                 std::cout << "Probing: " << device.portName << std::endl;
                 std::shared_ptr<PortAccessor> portAccessor = std::make_shared<PortAccessor>(device);
                 UdCapProbe prober(portAccessor);
                 switch (prober.probe()) {
                     case UDCAP_PROBE_HAND_V1:
                     {
                         receiver[prober.getUDCapSerial()] = portAccessor;
                         connected.push_back(device.portName);
                         std::cout << "Found UdCap Hand V1 device: " << prober.getUDCapSerial() << std::endl;
                         break;
                     }
                     default:
                     {

                     }
                 }
             }
             if (!receiver.empty()) {
                 bool needStopProbe = false;
                 for (const auto& recv: receiver) {
                     if (recv.first.ends_with('L') && leftHandSerial == nullptr) {
                         leftHandSerial = recv.second;
                         std::cout << "Found Left Hand Serial: " << recv.first << std::endl;
                         runOnUIThread([this](){
                             mLeftHand.set(create_placeholder_green_image());
                             return false;
                         });
                     } else if (recv.first.ends_with('R') && rightHandSerial == nullptr) {
                         rightHandSerial = recv.second;
                         std::cout << "Found Right Hand Serial: " << recv.first << std::endl;
                         runOnUIThread([this](){
                             mRightHand.set(create_placeholder_green_image());
                             return false;
                         });
                     }
                     if (leftHandSerial && rightHandSerial) {
                         mHandCoreLeft = std::make_shared<UdCapV1Core>(leftHandSerial);
                         mHandCoreRight = std::make_shared<UdCapV1Core>(rightHandSerial);
                         runOnUIThread([this](){
                             mStatus.set_markup(_("<span font='18' weight='bold'>Found</span>"));
                             mDescription.set_text(_("Waiting for UdCap Gloves..."));
                             mCalibrate.set_sensitive(false);
                             return false;
                         });
                         mHandCoreLeft->listen([&](std::shared_ptr<UdCapV1MCUPacket> data) {
                             if (data->commandType == CMD_LINK_STATE) {
                                 mLeftState = data->udState;
                                switch (data->udState) {
                                    case UD_INIT_STATE_LINKED:
                                    {
                                        runOnUIThread([this](){
                                            mLeftHand.set(create_placeholder_blue_image());
                                            return false;
                                        });
                                        mHandCoreLeft->tryRestoreHandCalibration();
                                        break;
                                    }
                                    case UD_INIT_STATE_NOT_CONNECT:
                                    {
                                        runOnUIThread([this]() {
                                            mLeftHand.set(create_placeholder_green_image());
                                            return false;
                                        });
                                        break;
                                    }
                                    case UD_INIT_STATE_CONNECTED:
                                    {
                                        runOnUIThread([this]() {
                                            mLeftHand.set(create_placeholder_green_image());
                                            return false;
                                        });
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
                                         runOnUIThread([this](){
                                             mRightHand.set(create_placeholder_blue_image());
                                             return false;
                                         });
                                         mHandCoreRight->tryRestoreHandCalibration();
                                         break;
                                     }
                                     case UD_INIT_STATE_NOT_CONNECT:
                                     {
                                         runOnUIThread([this]() {
                                             mRightHand.set(create_placeholder_green_image());
                                             return false;
                                         });
                                         break;
                                     }
                                     case UD_INIT_STATE_CONNECTED:
                                     {
                                         runOnUIThread([this]() {
                                             mRightHand.set(create_placeholder_green_image());
                                             return false;
                                         });
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
                         needStopProbe = true;
                         break;
                     }
                 }
                 if (needStopProbe) {
                     break;
                 }
             }
             std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    probeThread.detach();

    setupVMCSender();
    setupOSCSender();
    setupUdcapQTSender();
    setupVRSender();
    OSCServer::getInstance().setCallback([this](std::string address, std::vector<std::any> args){
        if (address == "/udcap/device/calibrate") {
            std::vector<std::shared_ptr<UdCapV1Core>> caliCores;
            std::vector<std::string> caliCoresAdded;
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
                std::vector<std::string> serials = any_cast<std::vector<std::string>>(args[2]);
                for (std::string serial: serials) {
                    if (isRegex) {
                        std::regex pattern(serial);
                        if (mHandCoreLeft && std::regex_match(mHandCoreLeft->getUDCapSerial(), pattern) && mLeftState == UD_INIT_STATE_LINKED) {
                            if (std::find(caliCoresAdded.begin(), caliCoresAdded.end(), mHandCoreLeft->getUDCapSerial()) != caliCoresAdded.end()) {
                                continue; // Already added
                            }
                            caliCoresAdded.push_back(mHandCoreLeft->getUDCapSerial());
                            caliCores.push_back(mHandCoreLeft);
                        }
                        if (mHandCoreRight && std::regex_match(mHandCoreRight->getUDCapSerial(), pattern) && mRightState == UD_INIT_STATE_LINKED) {
                            if (std::find(caliCoresAdded.begin(), caliCoresAdded.end(), mHandCoreRight->getUDCapSerial()) != caliCoresAdded.end()) {
                                continue; // Already added
                            }
                            caliCoresAdded.push_back(mHandCoreRight->getUDCapSerial());
                            caliCores.push_back(mHandCoreRight);
                        }
                    } else {
                        if (mHandCoreLeft && mHandCoreLeft->getUDCapSerial() == serial && mLeftState == UD_INIT_STATE_LINKED) {
                            if (std::find(caliCoresAdded.begin(), caliCoresAdded.end(), mHandCoreLeft->getUDCapSerial()) != caliCoresAdded.end()) {
                                continue; // Already added
                            }
                            caliCoresAdded.push_back(mHandCoreLeft->getUDCapSerial());
                            caliCores.push_back(mHandCoreLeft);
                        }
                        if (mHandCoreRight && mHandCoreRight->getUDCapSerial() == serial && mRightState == UD_INIT_STATE_LINKED) {
                            if (std::find(caliCoresAdded.begin(), caliCoresAdded.end(), mHandCoreRight->getUDCapSerial()) != caliCoresAdded.end()) {
                                continue; // Already added
                            }
                            caliCoresAdded.push_back(mHandCoreRight->getUDCapSerial());
                            caliCores.push_back(mHandCoreRight);
                        }
                    }
                }
            }

            if (!caliCores.empty()) {
                runOnUIThread([this, caliCores, autoStart](){
                    try {
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
    });
    OSCServer::getInstance().restart();
}

ConsumerUI::~ConsumerUI() {
    OSCServer::getInstance().setCallback(nullptr);
}

void ConsumerUI::runOnUIThread(std::function<bool(void)> callEvent) {
    Glib::MainContext::get_default()->invoke(callEvent);
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
                mLeftHand.set(create_placeholder_green_image());
            }
            if (!rightReady) {
                mRightHand.set(create_placeholder_green_image());
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
    mSettingsMenu->append(_("VR Settings"), "win.settings.vr");
    mSettingsMenu->append(_("Hand Settings"), "win.settings.hands");
    mSettingsMenu->append(_("Data Transfer"), "win.settings.data_transfer");
    mSettingsMenu->append(_("Firmware"), "win.settings.firmware");
    mSettingsMenu->append(_("Preference"), "app.settings.preference");

    auto actionFirmware = Gio::SimpleAction::create("settings.firmware");
    actionFirmware->signal_activate().connect([this](const Glib::VariantBase&) {
        std::vector<std::shared_ptr<UdCapV1Core>> cores;
        cores.push_back(mHandCoreLeft);
        cores.push_back(mHandCoreRight);
        mFirmwareDialog = std::make_unique<FirmwareDialog>(cores);
        mFirmwareDialog->set_transient_for(*this);
        mFirmwareDialog->show();
    });
    add_action(actionFirmware);
    auto actionDataTransfer = Gio::SimpleAction::create("settings.data_transfer");
    actionDataTransfer->signal_activate().connect([this](const Glib::VariantBase&) {
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
        });
        mDataTransferDialog->set_transient_for(*this);
        mDataTransferDialog->show();
    });
    add_action(actionDataTransfer);
    auto actionPreference = Gio::SimpleAction::create("settings.preference");
    actionPreference->signal_activate().connect([this](const Glib::VariantBase&) {
        mPreferenceDialog = std::make_unique<PreferenceDialog>();
        mPreferenceDialog->set_transient_for(*this);
        mPreferenceDialog->set_modal(true);
        mPreferenceDialog->show();
    });
    get_application()->add_action(actionPreference);
}

void ConsumerUI::setupVMCSender() {
    runOnUIThread([this]() {
        bool enable = UserConfig::getInstance().get<bool>("/consumer/vmc/enabled", false);
        std::string host = UserConfig::getInstance().get<std::string>("/consumer/vmc/host", "127.0.0.1");
        int port = UserConfig::getInstance().get<int>("/consumer/vmc/port", 39540);
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mVMC.set(create_placeholder_green_image());
                return false;
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
        return false;
    });
}

void ConsumerUI::setupOSCSender() {
    runOnUIThread([this](){
        bool enable = UserConfig::getInstance().get<bool>("/consumer/osc/enabled", false);
        std::string host = UserConfig::getInstance().get<std::string>("/consumer/osc/host", "127.0.0.1");
        int port = UserConfig::getInstance().get<int>("/consumer/osc/port", 9000);
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mOSC.set(create_placeholder_green_image());
                return false;
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
        return false;
    });
}

void ConsumerUI::setupUdcapQTSender() {
    runOnUIThread([this](){
        bool enable = UserConfig::getInstance().get<bool>("/consumer/udCapQingTong/enabled", false);
        std::string host = UserConfig::getInstance().get<std::string>("/consumer/udCapQingTong/host", "127.0.0.1");
        int port = UserConfig::getInstance().get<int>("/consumer/udCapQingTong/port", 6666);
        if (enable) {
            if (mLeftState != UD_INIT_STATE_LINKED || mRightState != UD_INIT_STATE_LINKED) {
                mBroadcast.set(create_placeholder_green_image());
                return false;
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
        return false;
    });
}

void ConsumerUI::setupVRSender() {

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
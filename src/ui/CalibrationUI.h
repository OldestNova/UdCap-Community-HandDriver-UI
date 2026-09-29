//
// Created by max_3 on 2025/6/10.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_CALIBRATIONUI_H
#define UDCAPCOMMUNITYDRIVERUI_CALIBRATIONUI_H

#include <cstdint>
#include <memory>
#include <vector>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <UdCapV1Core.h>

enum CalibrationStep {
    CALIBRATION_STEP_FIST = 0,
    CALIBRATION_STEP_ADDUCTION = 1,
    CALIBRATION_STEP_PROTRACT = 2
};

class CalibrationUI: public Gtk::Window {
public:
    explicit CalibrationUI(std::vector<std::shared_ptr<UdCapV1Core>> _core);
    ~CalibrationUI() override;
    void startProcess();
    void stopProcess();
private:
    std::vector<std::shared_ptr<UdCapV1Core>> core;
    Gtk::Box mVbox;
    Gtk::Label mTitleLabel;
    Gtk::Label mInfoLabel;
    Gtk::Image mImage;
    Gtk::Button mCalibrateButton;

    Gtk::Box mCVbox;
    Gtk::Label mCTitleLabel;
    Gtk::Label mCInfoLabel;
    Gtk::Image mCImage;
    Gtk::ProgressBar mCProgressBar;
    sigc::connection mCProcessBarTimeout;
    Gtk::Label mCInstructionLabel;

    Gtk::Box mEVbox;
    Gtk::Label mETitleLabel;
    Gtk::Label mEInfoLabel;
    Gtk::Button mEReturnButton;
    static constexpr int calibrationWaitTicks = 30; // 100 ms timer, 3 seconds per pose.
    int remainingTicks = calibrationWaitTicks;
    CalibrationStep currentCalibrationStep = CALIBRATION_STEP_FIST;
    bool on_calibration_progressbar_timeout();
    void showCurrentStep();
};


#endif //UDCAPCOMMUNITYDRIVERUI_CALIBRATIONUI_H

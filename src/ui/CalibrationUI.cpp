//
// Created by max_3 on 2025/6/10.
//

#include "CalibrationUI.h"

CalibrationUI::CalibrationUI(std::vector<std::shared_ptr<UdCapV1Core>> _core):
    core(_core), mVbox(), mTitleLabel(), mInfoLabel(), mImage(), mCalibrateButton(),
    mCVbox(), mCTitleLabel(), mCInfoLabel(), mCImage(), mCProgressBar(),mCInstructionLabel(),
    mEVbox(), mETitleLabel(), mEInfoLabel(), mEReturnButton()
{
    set_destroy_with_parent(true);
    set_title(_("Calibration"));
    set_default_size(800, 600);
    set_resizable(false);
    set_modal(true);

    mVbox.set_orientation(Gtk::Orientation::VERTICAL);
    mVbox.set_valign(Gtk::Align::CENTER);
    mVbox.set_halign(Gtk::Align::CENTER);
    mVbox.set_spacing(10);

    mTitleLabel.set_markup(_("<span font='18' weight='bold'>Calibration</span>"));
    mTitleLabel.set_justify(Gtk::Justification::CENTER);
    mTitleLabel.set_halign(Gtk::Align::CENTER);

    mInfoLabel.set_label(_("Please follow the visual and audio instructions to calibrate your UdCap gloves."));
    mInfoLabel.set_justify(Gtk::Justification::CENTER);
    mInfoLabel.set_halign(Gtk::Align::CENTER);

    mCalibrateButton.set_label(_("Start Calibration"));
    mCalibrateButton.set_halign(Gtk::Align::CENTER);
    mCalibrateButton.signal_clicked().connect(sigc::mem_fun(*this, &CalibrationUI::startProcess));

    mVbox.append(mTitleLabel);
    mVbox.append(mInfoLabel);
    mVbox.append(mImage);
    mVbox.append(mCalibrateButton);

    set_child(mVbox);


    mCVbox.set_orientation(Gtk::Orientation::VERTICAL);
    mCVbox.set_valign(Gtk::Align::CENTER);
    mCVbox.set_halign(Gtk::Align::CENTER);
    mCVbox.set_spacing(10);
    mCTitleLabel.set_markup(_("<span font='18' weight='bold'>Calibration Process</span>"));
    mCTitleLabel.set_justify(Gtk::Justification::CENTER);
    mCTitleLabel.set_halign(Gtk::Align::CENTER);

    mCInfoLabel.set_label(_("Fist"));
    mCInfoLabel.set_justify(Gtk::Justification::CENTER);
    mCInfoLabel.set_halign(Gtk::Align::CENTER);

    mCInstructionLabel.set_label(_("Wait for 5 seconds"));
    mCInstructionLabel.set_justify(Gtk::Justification::CENTER);
    mCInstructionLabel.set_halign(Gtk::Align::CENTER);

    mCProgressBar.set_fraction(1.0);
    calibrationStep = 5;

    mCVbox.append(mCTitleLabel);
    mCVbox.append(mCInfoLabel);
    mCVbox.append(mCImage);
    mCVbox.append(mCProgressBar);
    mCVbox.append(mCInstructionLabel);

    mEVbox.set_orientation(Gtk::Orientation::VERTICAL);
    mEVbox.set_valign(Gtk::Align::CENTER);
    mEVbox.set_halign(Gtk::Align::CENTER);
    mEVbox.set_spacing(10);
    mETitleLabel.set_markup(_("<span font='18' weight='bold'>Calibration Failed</span>"));
    mETitleLabel.set_justify(Gtk::Justification::CENTER);
    mETitleLabel.set_halign(Gtk::Align::CENTER);
    mEInfoLabel.set_label(_("Calibration failed, please try again."));
    mEInfoLabel.set_justify(Gtk::Justification::CENTER);
    mEInfoLabel.set_halign(Gtk::Align::CENTER);
    mEReturnButton.set_label(_("Close"));
    mEVbox.append(mETitleLabel);
    mEVbox.append(mEInfoLabel);
    mEVbox.append(mEReturnButton);
}

void CalibrationUI::startProcess() {
    if (!mCProcessBarTimeout.connected()) {
        currentCalibrationStep = CALIBRATION_STEP_FIST;
        calibrationStep = 5;
        calibrationSubStep = 10;
        set_child(mCVbox);
        for (const auto& c: core) {
            c->clearCalibrationData(UDCAP_V1_HAND_CALI_TYPE_ALL);
            c->runCalibration(UDCAP_V1_DEVICE_CALI_TYPE_HAND);
        }
        mCProcessBarTimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &CalibrationUI::on_calibration_progressbar_timeout), 100);
    }
}

void CalibrationUI::stopProcess() {
    if (mCProcessBarTimeout.connected()) {
        mCProcessBarTimeout.disconnect();
    }
}

bool CalibrationUI::on_calibration_progressbar_timeout() {
    calibrationSubStep--;
    if (calibrationStep >= 0) {
        switch (currentCalibrationStep) {
            case CALIBRATION_STEP_FIST: {
                mCInfoLabel.set_label(_("Fist"));
                break;
            }
            case CALIBRATION_STEP_ADDUCTION: {
                mCInfoLabel.set_label(_("Adduction"));
                break;
            }
            case CALIBRATION_STEP_PROTRACT: {
                mCInfoLabel.set_label(_("Protract"));
                break;
            }
        }
        if (calibrationSubStep < 0) {
            mCProgressBar.set_fraction(calibrationStep / 5.0);
            if (calibrationStep == 0) {
                mCInstructionLabel.set_label(_("Wait for 1 seconds"));
            } else {
                std::stringstream waitTextStream;
                waitTextStream << _("Wait for ") << calibrationStep << _(" seconds");
                mCInstructionLabel.set_label(waitTextStream.str());
            }
            calibrationStep--;
            calibrationSubStep = 10;
        }
    } else {
        if (calibrationSubStep < 0) {
            calibrationStep--;
            calibrationSubStep = 10;
        }
        if (calibrationSubStep == 5) {
            for (const auto& c: core) {
                if (currentCalibrationStep == CALIBRATION_STEP_FIST) {
                    c->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_FIST);
                } else if (currentCalibrationStep == CALIBRATION_STEP_ADDUCTION) {
                    c->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_ADDUCTION);
                } else if (currentCalibrationStep == CALIBRATION_STEP_PROTRACT) {
                    c->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_PROTRACT);
                }
            }
        }
        mCInstructionLabel.set_label(_("Don't move, capturing data..."));
        mCProgressBar.pulse();
        if (calibrationStep < -4) {
            calibrationStep = 5;
            if (currentCalibrationStep == CALIBRATION_STEP_FIST) {
                currentCalibrationStep = CALIBRATION_STEP_ADDUCTION;
            } else if (currentCalibrationStep == CALIBRATION_STEP_ADDUCTION) {
                currentCalibrationStep = CALIBRATION_STEP_PROTRACT;
            } else if (currentCalibrationStep == CALIBRATION_STEP_PROTRACT) {
                bool isSuccess = true;
                stopProcess();
                for (const auto& c: core) {
                    try {
                        c->completeCalibration(UDCAP_V1_DEVICE_CALI_TYPE_HAND);
                    } catch (std::runtime_error &e) {
                        isSuccess = false;
                    }
                }
                if (isSuccess) {
                    destroy();
                } else {
                    set_child(mEVbox);
                }
            }
        }
    }
    return true;
}
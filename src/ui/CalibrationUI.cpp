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

    mCInfoLabel.set_label(_("Make a fist"));
    mCInfoLabel.set_justify(Gtk::Justification::CENTER);
    mCInfoLabel.set_halign(Gtk::Align::CENTER);

    mCInstructionLabel.set_label(_("Wait for 3 seconds"));
    mCInstructionLabel.set_justify(Gtk::Justification::CENTER);
    mCInstructionLabel.set_halign(Gtk::Align::CENTER);

    mCProgressBar.set_fraction(1.0);

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
    mEInfoLabel.set_wrap(true);
    mEReturnButton.set_label(_("Close"));
    mEReturnButton.signal_clicked().connect([this]() {
        close();
    });
    signal_close_request().connect([this]() {
        stopProcess();
        return false;
    }, false);
    mEVbox.append(mETitleLabel);
    mEVbox.append(mEInfoLabel);
    mEVbox.append(mEReturnButton);
}

CalibrationUI::~CalibrationUI() {
    stopProcess();
}

void CalibrationUI::startProcess() {
    if (!mCProcessBarTimeout.connected()) {
        if (core.empty()) {
            mEInfoLabel.set_text(_("No connected gloves are available for calibration."));
            set_child(mEVbox);
            return;
        }
        currentCalibrationStep = CALIBRATION_STEP_FIST;
        remainingTicks = calibrationWaitTicks;
        showCurrentStep();
        set_child(mCVbox);
        try {
            for (const auto& c: core) {
                c->clearCalibrationData(UDCAP_V1_HAND_CALI_TYPE_ALL);
                c->runCalibration(UDCAP_V1_DEVICE_CALI_TYPE_HAND);
            }
        } catch (const std::exception &e) {
            mEInfoLabel.set_text(e.what());
            set_child(mEVbox);
            return;
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
    --remainingTicks;
    mCProgressBar.set_fraction(static_cast<double>(remainingTicks) / calibrationWaitTicks);
    if (remainingTicks > 0) {
        if (remainingTicks % 10 == 0) {
            const int seconds = remainingTicks / 10;
            mCInstructionLabel.set_label(seconds == 1
                ? _("Wait for 1 second")
                : Glib::ustring::compose(_("Wait for %1 seconds"), seconds));
        }
        return true;
    }

    try {
        for (const auto& c: core) {
            if (currentCalibrationStep == CALIBRATION_STEP_FIST) {
                c->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_FIST);
            } else if (currentCalibrationStep == CALIBRATION_STEP_ADDUCTION) {
                c->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_ADDUCTION);
            } else {
                c->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_PROTRACT);
            }
        }
    } catch (const std::exception &e) {
        stopProcess();
        mEInfoLabel.set_text(e.what());
        set_child(mEVbox);
        return false;
    }

    if (currentCalibrationStep == CALIBRATION_STEP_FIST) {
        currentCalibrationStep = CALIBRATION_STEP_ADDUCTION;
    } else if (currentCalibrationStep == CALIBRATION_STEP_ADDUCTION) {
        currentCalibrationStep = CALIBRATION_STEP_PROTRACT;
    } else {
        stopProcess();
        std::string failureDetails;
        for (const auto& c: core) {
            try {
                c->completeCalibration(UDCAP_V1_DEVICE_CALI_TYPE_HAND);
            } catch (const std::exception &e) {
                if (!failureDetails.empty()) failureDetails += "\n";
                std::string glove = c->getUDCapSerial();
                if (glove.empty()) {
                    glove = c->getTarget() == UD_TARGET_RIGHT_HAND
                        ? _("Right glove") : _("Left glove");
                }
                const std::string reason = std::string(e.what()) == "Calibration failed"
                    ? _("Finger movement was too small between the fist and open hand captures.")
                    : e.what();
                failureDetails += glove + ": " + reason;
            }
        }
        if (failureDetails.empty()) close();
        else {
            mEInfoLabel.set_text(failureDetails);
            set_child(mEVbox);
        }
        return false;
    }

    remainingTicks = calibrationWaitTicks;
    showCurrentStep();
    return true;
}

void CalibrationUI::showCurrentStep() {
    switch (currentCalibrationStep) {
        case CALIBRATION_STEP_FIST:
            mCInfoLabel.set_label(_("Make a fist"));
            break;
        case CALIBRATION_STEP_ADDUCTION:
            mCInfoLabel.set_label(_("Straighten and bring your fingers together"));
            break;
        case CALIBRATION_STEP_PROTRACT:
            mCInfoLabel.set_label(_("Spread your fingers"));
            break;
    }
    mCInstructionLabel.set_label(_("Wait for 3 seconds"));
    mCProgressBar.set_fraction(1.0);
}

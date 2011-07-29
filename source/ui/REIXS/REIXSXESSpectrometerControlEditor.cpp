#include "REIXSXESSpectrometerControlEditor.h"
#include "ui_REIXSXESSpectrometerControlEditor.h"

#include "util/AMErrorMonitor.h"
#include <QStringBuilder>

REIXSXESSpectrometerControlEditor::REIXSXESSpectrometerControlEditor(REIXSSpectrometer* spectrometer, QWidget *parent) :
	QFrame(parent),
	ui_(new Ui::REIXSXESSpectrometerControlEditor)
{
	ui_->setupUi(this);

	spectrometer_ = spectrometer;

	populateGratingComboBox();

	connect(spectrometer_, SIGNAL(valueChanged(double)), this, SLOT(updateCurrentEnergyStatus(double)));
	connect(spectrometer_, SIGNAL(calibrationChanged()), this, SLOT(populateGratingComboBox()));
	connect(spectrometer_, SIGNAL(gratingChanged(int)), this, SLOT(updateCurrentGratingStatus()));

	connect(ui_->energyBox, SIGNAL(valueChanged(double)), this, SLOT(updateCurrentEnergyStatus()));
	connect(ui_->gratingSelectorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onGratingComboBoxActivated(int)));


	connect(ui_->moveNowButton, SIGNAL(clicked()), this, SLOT(onMoveButtonClicked()));

	connect(spectrometer_, SIGNAL(moveSucceeded()), this, SLOT(onSpectrometerMoveSucceeded()));
	connect(spectrometer_, SIGNAL(moveFailed(int)), this, SLOT(onSpectrometerMoveFailed(int)));

	connect(ui_->stopButton, SIGNAL(clicked()), this, SLOT(onStopButtonClicked()));

}

REIXSXESSpectrometerControlEditor::~REIXSXESSpectrometerControlEditor()
{
	delete ui_;
}


void REIXSXESSpectrometerControlEditor::onMoveButtonClicked()
{
	spectrometer_->specifyGrating(ui_->gratingSelectorBox->currentIndex());
	spectrometer_->specifyDetectorTiltOffset(ui_->tiltOffsetBox->value());
	spectrometer_->specifyFocusOffset(ui_->defocusOffsetBox->value());

	spectrometer_->move(ui_->energyBox->value());
}

void REIXSXESSpectrometerControlEditor::onGratingComboBoxActivated(int grating)
{
	Q_UNUSED(grating)
	updateCurrentGratingStatus();
}

void REIXSXESSpectrometerControlEditor::populateGratingComboBox()
{
	ui_->gratingSelectorBox->blockSignals(true);

	ui_->gratingSelectorBox->clear();

	QStringList gratingNames = spectrometer_->spectrometerCalibration()->gratingNames();

	ui_->gratingSelectorBox->addItems(gratingNames);

	ui_->gratingSelectorBox->blockSignals(false);
	updateCurrentGratingStatus();
}

void REIXSXESSpectrometerControlEditor::updateCurrentEnergyStatus(double eV)
{
	if(eV < 0) {
		ui_->energyFeedbackLabel->setText("Currently: not ready");
	}

	else if(fabs(eV - ui_->energyBox->value()) < 0.001) {
		ui_->energyFeedbackLabel->setText(QString());	// we're exactly what the user asked for; all is well, don't need to show anything.
	}
	else {
		ui_->energyFeedbackLabel->setText(QString("Currently: %1").arg(eV));
	}
}

void REIXSXESSpectrometerControlEditor::updateCurrentEnergyStatus() {
	updateCurrentEnergyStatus(spectrometer_->value());
}

void REIXSXESSpectrometerControlEditor::updateCurrentGratingStatus()
{
	if(spectrometer_->grating() == -1) {
		ui_->gratingFeedbackLabel->setText("Currently: no grating ready");
	}
	else if(spectrometer_->gratingInPosition() == false) {
		ui_->gratingFeedbackLabel->setText(
					QString("Currently: %1 (out of position)")
					.arg(spectrometer_->spectrometerCalibration()->gratingName(spectrometer_->grating())));
	}
	else if(spectrometer_->grating() != ui_->gratingSelectorBox->currentIndex()) {
		ui_->gratingFeedbackLabel->setText(("Currently: " % spectrometer_->spectrometerCalibration()->gratingName(spectrometer_->grating())));
	}
	else {
		// our drop-down selection matches the current grating, and it's in position. All is well -- don't need to tell the user anything.
		ui_->gratingFeedbackLabel->setText(QString());
	}
}

void REIXSXESSpectrometerControlEditor::onSpectrometerMoveSucceeded()
{
	AMErrorMon::report(AMErrorReport(this, AMErrorReport::Information, 0, QString("Spectrometer move to %1 finished.").arg(spectrometer_->value())));
}

void REIXSXESSpectrometerControlEditor::onSpectrometerMoveFailed(int reason)
{
	AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, reason, "Spectrometer Move Failed"));
}

void REIXSXESSpectrometerControlEditor::onStopButtonClicked()
{
	spectrometer_->stop();
}


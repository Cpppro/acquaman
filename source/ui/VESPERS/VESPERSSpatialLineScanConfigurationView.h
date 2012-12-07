/*
Copyright 2010-2012 Mark Boots, David Chevrier, and Darren Hunter.

This file is part of the Acquaman Data Acquisition and Management framework ("Acquaman").
Acquaman is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Acquaman is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Acquaman.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef VESPERSSPATIALLINESCANCONFIGURATIONVIEW_H
#define VESPERSSPATIALLINESCANCONFIGURATIONVIEW_H

#include "ui/acquaman/AMScanConfigurationView.h"
#include "ui/VESPERS/VESPERSScanConfigurationView.h"
#include "acquaman/VESPERS/VESPERSSpatialLineScanConfiguration.h"
#include "acquaman/VESPERS/VESPERSSpatialLineDacqScanController.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>

/// This class builds the view for configuring a spatial line scan for the VESPERS beamline.
class VESPERSSpatialLineScanConfigurationView : public VESPERSScanConfigurationView
{
	Q_OBJECT
public:
	/// Constructor.
	explicit VESPERSSpatialLineScanConfigurationView(VESPERSSpatialLineScanConfiguration *config, QWidget *parent = 0);

	/// Getter for the configuration.
	const AMScanConfiguration* configuration() const { return config_; }

	/// Method that updates the map info label based on the current values of the start, end, and step size.
	void updateMapInfo();

protected slots:
	/// Handles setting the start position when the "Use Current" button is pushed.
	void onSetStartPosition();
	/// Handles setting the end position when the "Use Current" button is pushed.
	void onSetEndPosition();
	/// Helper slot that manages setting the x axis start position.
	void onStartChanged();
	/// Helper slot that manages setting the x axis end position.
	void onEndChanged();
	/// Helper slot that manages setting the x axis step size.
	void onStepChanged();
	/// Helper slot that manages setting the time per point.
	void onDwellTimeChanged();

	/// Helper slot that sets the CCD detector setting in the configuration.
	void onCCDButtonClicked(bool useCCD) { config_->setCCDDetector(useCCD ? 1 : 0); }
	/// Handles passing the name of the CCD file name to the scan configuration when the CCD check box is enabled.
	void onCCDDetectorChanged(int useCCD);
	/// Handles changes in the name of the CCD file name and sets the label that corresponds to it.
	void onCCDFileNameChanged(QString name) { currentCCDFileName_->setText(QString("Current CCD file name:\t%1").arg(name)); }
	/// Handles setting the name of the configuration from the line edit.
	void onScanNameEdited() { config_->setName(scanName_->text()); config_->setUserScanName(scanName_->text());}
	/// Passes on the selection for I0 to the configuration.
	void onI0Clicked(int id) { config_->setIncomingChoice(id); }
	/// Handles changes to the fluorescence detector choice.
	void onFluorescenceDetectorChanged(int id);
	/// Handles changes in the motor selection choice.
	void onMotorChanged(int id);
	/// Helper slot that sets the time offset for the scan.
	void setTimeOffset(double time) { config_->setTimeOffset(time); }
	/// Helper slot that handles the setting the estimated time label.
	void onEstimatedTimeChanged();

	/// Emits the configureDetector signal based on the current fluorescence detector choice.
	void onConfigureXRFDetectorClicked() { emit configureDetector(fluorescenceDetectorIdToString(int(config_->fluorescenceDetector()))); }
	/// Emits the configureDetector signal based with 'Roper CCD'.
	void onConfigureRoperDetectorClicked();
	/// Updates roiText_ based on the current state of the ROI list.
	void updateRoiText();

	/// Slot that updates the horizontal step size spin box.
	void updateStep(double val) { step_->setValue(val*1000); }

protected:
	/// Reimplements the show event to update the Regions of Interest text.
	virtual void showEvent(QShowEvent *e) { updateRoiText(); AMScanConfigurationView::showEvent(e); }
	/// Helper method that updates the x and y step spin boxes if the map is not possible to change.
	void axesAcceptable();

	/// Pointer to the specific scan config the view is modifying.
	VESPERSSpatialLineScanConfiguration *config_;

	/// Pointer to the start point spin box.
	QDoubleSpinBox *start_;
	/// Pointer to the end point spin box.
	QDoubleSpinBox *end_;
	/// Pointer to the step size spin box.
	QDoubleSpinBox *step_;

	/// Pointer to the label that holds the current map settings.
	QLabel *mapInfo_;
	/// Pointer to the check box for doing XRD maps as well.
	QCheckBox *ccdCheckBox_;
	/// Pointer to the label holding the current file name.
	QLabel *currentCCDFileName_;
	/// Label holding the current estimated time for the scan to complete.  Takes into account extra time per point based on experience on the beamline.
	QLabel *estimatedTime_;
};

#endif // VESPERSSPATIALLINESCANCONFIGURATIONVIEW_H

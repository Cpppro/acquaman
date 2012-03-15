#ifndef VESPERS2DSCANCONFIGURATIONVIEW_H
#define VESPERS2DSCANCONFIGURATIONVIEW_H

#include "ui/acquaman/AMScanConfigurationView.h"
#include "acquaman/VESPERS/VESPERS2DDacqScanController.h"
#include "acquaman/VESPERS/VESPERS2DScanConfiguration.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QComboBox>
#include <QLabel>

/// This class builds the view for configuring a 2D map scan for the VESPERS beamline.
class VESPERS2DScanConfigurationView : public AMScanConfigurationView
{
	Q_OBJECT

public:
	/// Constructor.  \param config is the 2D configuration that the view will modify.
	VESPERS2DScanConfigurationView(VESPERS2DScanConfiguration *config, QWidget *parent = 0);

	/// Getter for the configuration.
	const AMScanConfiguration *configuration() const { return config_; }

signals:
	/// Sends out a request that the current detector (based on FluorescenceDetectorChoice) to be configured.  Asks the app controller to change to the detector view.  String will be either "Single Element" or "Four Element".
	void configureDetector(const QString &);

protected slots:
	/// Handles setting the start position when the "Use Current" button is pushed.
	void onSetStartPosition();
	/// Handles setting the end position when the "Use Current" button is pushed.
	void onSetEndPosition();
	/// Helper slot that manages setting the x axis start position.
	void onXStartChanged();
	/// Helper slot that manages setting the x axis end position.
	void onXEndChanged();
	/// Helper slot that manages setting the x axis step size.
	void onXStepChanged();
	/// Helper slot that manages setting the y axis start position.
	void onYStartChanged();
	/// Helper slot that manages setting the y axis end position.
	void onYEndChanged();
	/// Helper slot that manages setting the y axis step size.
	void onYStepChanged();
	/// Helper slot that manages setting the time per point.
	void onDwellTimeChanged();

	/// Handles setting the name of the configuration from the line edit.
	void onScanNameEdited() { config_->setName(scanName_->text()); }
	/// Passes on the selection for I0 to the configuration.
	void onI0Clicked(int id) { config_->setIncomingChoice(id); }
	/// Handles changes to the fluorescence detector choice.
	void onFluorescenceChoiceChanged(int id);
	/// Helper slot that handles the setting the estimated time label.
	void onEstimatedTimeChanged();

	/// Emits the configureDetector signal based on the current fluorescence detector choice.
	void onConfigureDetectorClicked();
	/// Updates roiText_ based on the current state of the ROI list.
	void updateRoiText();
	/// Handles the context menu.
	void onCustomContextMenuRequested(QPoint pos);

protected:
	/// Reimplements the show event to update the Regions of Interest text.
	virtual void showEvent(QShowEvent *e) { updateRoiText(); AMScanConfigurationView::showEvent(e); }
	/// Helper method that takes a time in seconds and returns a string of d:h:m:s.
	QString convertTimeToString(double time);
	/// Helper method that updates the map info label based on the current values of the start, end, and step size.
	void updateMapInfo();
	/// Helper method that updates the x and y step spin boxes if the map is not possible to change.
	void axesAcceptable();

	/// Pointer to the specific scan config the view is modifying.
	VESPERS2DScanConfiguration *config_;

	/// Pointer to the horizontal start point.
	QDoubleSpinBox *hStart_;
	/// Pointer to the horizontal end point.
	QDoubleSpinBox *hEnd_;
	/// Pointer to the vertical start point.
	QDoubleSpinBox *vStart_;
	/// Pointer to the vertical end point.
	QDoubleSpinBox *vEnd_;
	/// Pointer to the horizontal step size.
	QDoubleSpinBox *hStep_;
	/// Pointer to the vertical step size.
	QDoubleSpinBox *vStep_;

	/// Pointer to the dwell time per point.
	QDoubleSpinBox *dwellTime_;

	/// Pointer to the label that holds the current map settings.
	QLabel *mapInfo_;

	/// Line edit for changing the name of the scan.
	QLineEdit *scanName_;
	/// Label holding the current estimated time for the scan to complete.  Takes into account extra time per point based on experience on the beamline.
	QLabel *estimatedTime_;
	/// Button group for the I0 ion chamber selection.
	QButtonGroup *I0Group_;
	/// The text edit that holds all the names of the regions of interest.
	QTextEdit *roiText_;

	/// A label holding text for the the time offset spin box.
	QLabel *timeOffsetLabel_;
	/// A spin box holding the time offset.
	QDoubleSpinBox *timeOffset_;
};

#endif // VESPERS2DSCANCONFIGURATIONVIEW_H
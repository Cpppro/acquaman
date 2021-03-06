#ifndef VESPERSSCANCONFIGURATIONVIEW_H
#define VESPERSSCANCONFIGURATIONVIEW_H

#include "ui/acquaman/AMScanConfigurationView.h"
#include "dataman/AMUser.h"
#include "application/VESPERS/VESPERS.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QMenu>

/*! This class is primarily for having helper methods to all of the subclasses for when
	they build up the varios GUI elements.  Because many of them use the exact same code
	they are brought together here.  Many of the methods return a QWidget* but they still
	have their specific pointers to individual buttons, spin boxes, etc..
*/
class VESPERSScanConfigurationView : public AMScanConfigurationView
{
	Q_OBJECT

public:
	/// Constructor.  Passes in the configuraiton that this view will modify.
	VESPERSScanConfigurationView(QWidget *parent = 0);

signals:
	/// Sends out a request that the current detector based on FluorescenceDetector to be configured.  Asks the app controller to change to the detector view.  String will be either "Single Element" or "Four Element".
	void configureDetector(const QString &);

protected slots:
	/// Handles propogating changes in the config to the It buttons.
	void updateItButtons(int It);
	/// Handles propogating changes in the config to the I0 buttons.
	void updateI0Buttons(int I0);
	/// Slot that updates the fluorescence detector buttons.
	void updateFluorescenceDetector(int detector);
	/// Slot that updates the ccd detector buttons.
	void updateCCDDetectorButtons(int detector);
	/// Slot that updates the motor choice buttons.
	void updateMotor(int choice);

	/// Handles the context menu.
	void onCustomContextMenuRequested(QPoint pos);
	/// Emits the configureDetector signal based on the current fluorescence detector choice given by \param id.
	QString fluorescenceDetectorIdToString(int id);
	/// Emits the configureDetector signal based on the current ccd detector choice given by \param id.
	QString ccdDetectorIdToString(int id);

protected:
	/// Figure out the current configuration of the regions of interest and write it out in a readable way.
	void updateAndSetRoiTextBox(int xrfId);

	/// Add the fluorescenceDetector view.  Returns a pointer to the widget.
	QGroupBox *addFluorescenceDetectorSelectionView();
	/// Add the ccdDetector view.  Returns a pointer to the widget.
	QGroupBox *addCCDDetectorSelectionView();
	/// Add the I0 selection view.  Returns a pointer to the widget.
	QGroupBox *addI0SelectionView();
	/// Add the It selection view.  Returns a pointer to the widget.
	QGroupBox *addItSelectionView();
	/// Add the motor group view.  Returns a pointer to the widget.  \note Both lists must be the same size!
	QGroupBox *addMotorSelectionView(QStringList labels, QList<int> ids);
	/// Add the scan name view.  Returns the widget.
	QLineEdit *addScanNameView(const QString &name);
	/// Add the goToPosition group box.  Returns a pointer to the widget.
	QGroupBox *addGoToPositionView(bool goToPosition, double x, double y);
	/// Add the export path label.  Returns a pointer to the widget.
	QLabel *addExportPathLabel();
	/// Add the time offset label.
	QGroupBox *addTimeOffsetLabel(double time);
	/// Add the export options view.  Returns a pointer to the widget.
	QGroupBox *addExporterOptionsView(QStringList list, bool exportSpectra);
	/// Add the dwell time box.  Returns a pointer to the widget.
	QDoubleSpinBox *addDwellTimeWidget(double time);
	/// Build a position QDoubleSpinBox based on the prefix, suffix and value.  They have the same format, this should cut down on duplicate code.
	QDoubleSpinBox *buildPositionDoubleSpinBox(const QString &prefix, const QString &suffix, double value, int decimals);

	/// Button group for the fluorescence detector selection.
	QButtonGroup *fluorescenceButtonGroup_;
	/// Button group for the ccd detector selection.
	QButtonGroup *ccdButtonGroup_;
	/// Button group for the It ion chamber selection.
	QButtonGroup *ItGroup_;
	/// Button group for the I0 ion chamber selection.
	QButtonGroup *I0Group_;
	/// Button group for the exporter options.
	QButtonGroup *autoExportButtonGroup_;
	/// Button group for the motor choice selection.
	QButtonGroup *motorButtonGroup_;

	/// Label holding what the currently saved x position is in the scan configuration.
	QLabel *savedXPosition_;
	/// Label holding what the currently saved y position is in the scan configuration.
	QLabel *savedYPosition_;
	/// Label holding whether or not the x and y positions have been saved yet.
	QLabel *positionsSaved_;
	/// Check box for enabling the go to position check box.
	QCheckBox *goToPositionCheckBox_;
	/// Button for saving the current coordinates of the motor.
	QPushButton *setCurrentPositionButton_;
	/// Check box for enabling/disabling whether or not spectra should be exported or not.
	QCheckBox *autoExportSpectra_;
	/// A label holding text for the the time offset spin box.
	QLabel *timeOffsetLabel_;
	/// A spin box holding the time offset.
	QDoubleSpinBox *timeOffset_;
	/// The text edit that holds all the names of the regions of interest.
	QTextEdit *roiText_;
	/// The label that holds useful information about the CCD - such as path and name.
	QLabel *ccdText_;
	/// The label that holds the help message when CCD file names conflict.
	QLabel *ccdHelpText_;
	/// Pointer to the dwell time per point.
	QDoubleSpinBox *dwellTime_;
	/// Line edit for changing the name of the scan.
	QLineEdit *scanName_;
};

#endif // VESPERSSCANCONFIGURATIONVIEW_H

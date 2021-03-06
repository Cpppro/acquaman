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


#ifndef VESPERSSAMPLESTAGEVIEW_H
#define VESPERSSAMPLESTAGEVIEW_H

#include "beamline/VESPERS/VESPERSBeamline.h"
#include "beamline/VESPERS/VESPERSSampleStageControl.h"

#include <QWidget>
#include <QDoubleSpinBox>
#include <QButtonGroup>
#include <QLabel>
#include <QMessageBox>
#include <QToolButton>

/*! This class builds a view on the sample stage control that does relative movement for the sample stage.  It does not change the focus distance,
  only the horizontal and vertical motors.
  */
class VESPERSSampleStageView : public QWidget
{
	Q_OBJECT
public:
	/// Default constructor.
	explicit VESPERSSampleStageView(VESPERSSampleStageControl *sampleStage, QWidget *parent = 0);

	/// Returns the title of the sample stage view.
	QString title() const { return title_; }
	/// Returns the horizontal title of the sample stage view.
	QString horizontalTitle() const { return horizontalTitle_; }
	/// Returns the vertical title of the sample stage view.
	QString verticalTitle() const { return verticalTitle_; }
	/// Returns whether the horizontal sample stage direction is being inverted or not.
	bool invertHorizontalDirection() const { return invertHorizontal_; }
	/// Returns whether the vertical sample stage direction is being inverted or not.
	bool invertVerticalDirection() const { return invertVertical_; }

signals:

public slots:
	/// Sets the title of the sample stage view.
	void setTitle(const QString &title);
	/// Sets the horizontal title of the sample stage view.
	void setHorizontalTitle(const QString &title);
	/// Sets the vertical title of the sample stage view.
	void setVerticalTitle(const QString &title);
	/// Sets whether the horizontal jog direction should be inverted or not.
	void setInvertHorizontalDirection(bool invert) { invertHorizontal_ = invert; }
	/// Sets whether the vertical jog direction should be inverted or not.
	void setInvertVerticalDirection(bool invert) { invertVertical_ = invert; }

protected slots:
	/// Slot that handles going up.
	void onUpClicked();
	/// Slot that handles going down.
	void onDownClicked();
	/// Slot that handles going left.
	void onLeftClicked();
	/// Slot that handles going right.
	void onRightClicked();
	/// Slot that handles if the horizontal setpoint line edit is triggered.
	void onHorizontalSetpoint();
	/// Slot that handles if the vertical setpoint line edit is triggered.
	void onVerticalSetpoint();

	/// Handles changes in the moving flags of the sample stages.
	void onMovingChanged(bool isMoving);
	/// Handles if the motors time out.
	void onTimedOut() { setEnabled(false); }
	/// Handles the error message from a horizontal move error from the sample stage.
	void onHorizontalMoveError(bool direction);
	/// Handles the error message from a vertical move error from the sample stage.
	void onVerticalMoveError(bool direction);
	/// Handles the error message from a normal move error from the sample stage.
	void onNormalMoveError(bool direction);

protected:
	/// Holds the title of this sample stage.
	QString title_;
	/// Holds the horizontal title.
	QString horizontalTitle_;
	/// Holds the vertical title.
	QString verticalTitle_;
	/// Holds the title label.
	QLabel *titleLabel_;
	/// Holds the horizontal title label.
	QLabel *horizontalLabel_;
	/// Holds the vertical title label.
	QLabel *verticalLabel_;
	/// Holds the jog value.  All movements are relative to this number.
	QDoubleSpinBox *jog_;
	/// This button group holds the four directional buttons.
	QButtonGroup *buttons_;
	/// The status label that shows whether the sample stage is moving or not.
	QLabel *status_;
	/// Holds the horizontal setpoint.
	QDoubleSpinBox *horizontal_;
	/// Holds the vertical setpoint.
	QDoubleSpinBox *vertical_;

	/// Flag holding whether the horizontal jog directions should be inverted.
	bool invertHorizontal_;
	/// Flag holding whether the vertical jog directions should be inverted.
	bool invertVertical_;

	// Jog buttons.
	/// The jog up button.
	QToolButton *goUp_;
	/// The jog down button.
	QToolButton *goDown_;
	/// The jog left button.
	QToolButton *goLeft_;
	/// The jog right button.
	QToolButton *goRight_;

	/// The control for the sample stage.
	VESPERSSampleStageControl *sampleStage_;
};

#endif // VESPERSSAMPLESTAGEVIEW_H

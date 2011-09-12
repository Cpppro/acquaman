/*
Copyright 2010, 2011 Mark Boots, David Chevrier, and Darren Hunter.

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


#ifndef VESPERSIONCHAMBER_H
#define VESPERSIONCHAMBER_H

#include <QObject>

#include "beamline/AMProcessVariable.h"

/*!
  This class encasulates all the necessary components to calibrate an individual ion chamber.  It can change the sensitivity to bring the counts into the linear range of the detector.
  */
class VESPERSIonChamber : public QObject
{
	Q_OBJECT
public:
	/// Constructor.  Builds a VESPERS specific ion chamber.
	/*!
	  \param name is the name of the ion chamber.
	  \param hvName is the full name of the PV for the high voltage.
	  \param sensitivityBaseName is the base name for the sensitivity value and units PV (ex: AMP1607-202).
	  \param voltageName is the full PV name for the voltage feedback.
	  \param countName is the full PV name for the counts feedback.
	  */
	explicit VESPERSIonChamber(QString name, QString hvName, QString sensitivityBaseName, QString voltageName, QString countName, QObject *parent = 0);

	/// Returns the name of the ion chamber.
	QString name() const { return name_; }
	/// Returns the high voltage of the ion chamber.
	int highVoltage() const { return hv_->getInt(); }
	/// Returns the value of the sensitivity.
	int sensitivityValue() const { return sensitivityValue_->getInt(); }
	/// Returns the units of the sensitivity.
	QString sensitivityUnits() const { return sensitivityUnits_->getString(); }
	/// Returns the voltage feedback.
	double voltage() const { return voltage_->getDouble(); }
	/// Returns the count feedback.
	double counts() const { return counts_->getDouble(); }

signals:
	/// Notifier that the high voltage has changed.  Passes the new value.
	void highVoltageChanged(int);
	/// Notifier that the sensitivity value has changed.  Passes the new value.
	void sensitivityValueChanged(int);
	/// Notifier that the sensitivity units have changed.  Passes the new value.
	void sensitivityUnitsChanged(QString);
	/// Notifier that the voltage feedback has been updated.  Passes the new value.
	void voltageChanged(double);
	/// Notifier that the counts feedback has been updated.  Passes the new value.
	void countsChanged(double);

public slots:
	/// Sets the high voltage.
	void setHighVoltage(int voltage) { hv_->setValue(voltage); }
	/// Sets the sensitivity value.  Must be 1, 2, 5, 10, 20, 50, 100, 200, or 500.  Does nothing otherwise.
	void setSensitivityValue(int value) { if (sensitivityValueOkay(value)) sensitivityValue_->setValue(value); }
	/// Sets the sensitivity units.  Must be pA/V, nA/V, uA/V, or mA/V.  Does nothing otherwise.
	void setSensitivityUnits(QString units) { if (sensitivityUnitsOkay(units)) sensitivityUnits_->setValue(units); }

protected slots:
	/// Turns the sensitivity value from an index to the value.
	void onSensitivityValueChanged(int index);

protected:
	/// Determines if the new sensitivity value is acceptable.
	bool sensitivityValueOkay(int value)
	{
		if (value >= 0 && value <= 8)
			return true;

		return false;
	}

	/// Determines if the new sensitivity units are acceptable.
	bool sensitivityUnitsOkay(QString units)
	{
		if (units == "pA/V" || units == "nA/V" || units == "uA/V" || units == "mA/V")
			return true;

		return false;
	}

	/// The name of the ion chamber.
	QString name_;

	/// The process variable that holds the high voltage of the ion chamber.
	AMProcessVariable *hv_;
	/// The process variable that holds the value of the sensitivity of the ion chamber.
	AMProcessVariable *sensitivityValue_;
	/// The process variable that holds the units of the sensitivity of the ion chamber.
	AMProcessVariable *sensitivityUnits_;
	/// The process variable that holds the voltage of the ion chamber.
	AMProcessVariable *voltage_;
	/// The process variable that holds the counts of the ion chamber.
	AMProcessVariable *counts_;
};

#endif // VESPERSIONCHAMBER_H

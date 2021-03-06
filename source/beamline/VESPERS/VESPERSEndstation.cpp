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


#include "VESPERSEndstation.h"
#include "beamline/CLS/CLSMAXvMotor.h"

#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>

VESPERSEndstation::VESPERSEndstation(AMControl *pseudoNormal, AMControl *realNormal, QObject *parent)
	: QObject(parent)
{
	current_ = 0;
	wasConnected_ = false;

	// The controls used for the control window.
	ccdControl_ = new CLSMAXvMotor("CCD motor", "SMTR1607-2-B21-18", "CCD motor", true, 1.0, 2.0, this);
	microscopeControl_ = new CLSMAXvMotor("Microscope motor", "SMTR1607-2-B21-17", "Microscope motor", false, 1.0, 2.0, this);
	fourElControl_ = new CLSMAXvMotor("4-Element Vortex motor", "SMTR1607-2-B21-27", "4-Element Vortex motor", true, 1.0, 2.0, this);
	singleElControl_ = new CLSMAXvMotor("1-Element Vortex motor", "SMTR1607-2-B21-15", "1-Element Vortex motor", true, 1.0, 2.0, this);

	focusNormalControl_ = pseudoNormal;
	focusYControl_ = realNormal;
	laserPositionControl_ = new AMReadOnlyPVControl("Laser Position", "PSD1607-2-B20-01:OUT1:fbk", this);

	// Microscope light PV.
	micLightPV_ = new AMProcessVariable("07B2_PLC_Mic_Light_Inten", true, this);

	// Laser on/off control.
	laserPower_ = new AMPVControl("Laser Power Control", "07B2_PLC_LaserDistON", "07B2_PLC_LaserDistON_Tog", QString(), this);

	// The beam attenuation filters.
	filter250umA_ = new AMPVControl("Filter 250um A", "07B2_PLC_PFIL_01_F1_Ctrl", "07B2_PLC_PFIL_01_F1_Toggle", QString(), this);
	filter250umB_ = new AMPVControl("Filter 250um B", "07B2_PLC_PFIL_01_F2_Ctrl", "07B2_PLC_PFIL_01_F2_Toggle", QString(), this);
	filter100umA_ = new AMPVControl("Filter 100um A", "07B2_PLC_PFIL_02_F3_Ctrl", "07B2_PLC_PFIL_02_F3_Toggle", QString(), this);
	filter100umB_ = new AMPVControl("Filter 100um B", "07B2_PLC_PFIL_02_F4_Ctrl", "07B2_PLC_PFIL_02_F4_Toggle", QString(), this);
	filter50umA_ = new AMPVControl("Filter 50um A", "07B2_PLC_PFIL_02_F1_Ctrl", "07B2_PLC_PFIL_02_F1_Toggle", QString(), this);
	filter50umB_ = new AMPVControl("Filter 50um B", "07B2_PLC_PFIL_02_F2_Ctrl", "07B2_PLC_PFIL_02_F2_Toggle", QString(), this);
	filterShutterUpper_ = new AMPVControl("Filter Shutter Upper", "07B2_PLC_PFIL_01_F3_Ctrl", "07B2_PLC_PFIL_01_F3_Toggle", QString(), this);
	filterShutterLower_ = new AMPVControl("Filter Shutter Lower", "07B2_PLC_PFIL_01_F4_Ctrl", "07B2_PLC_PFIL_01_F4_Toggle", QString(), this);
	filterThickness_ = -1;

	// Setup filters.
	filterMap_.insert("50A", filter50umA_);
	filterMap_.insert("50B", filter50umB_);
	filterMap_.insert("100A", filter100umA_);
	filterMap_.insert("100B", filter100umB_);
	filterMap_.insert("250A", filter250umA_);
	filterMap_.insert("250B", filter250umB_);

	// Get the current soft limits.
	loadConfiguration();

	// Connections.
	connect(filterShutterLower_, SIGNAL(valueChanged(double)), this, SLOT(onShutterChanged(double)));
	connect(laserPower_, SIGNAL(valueChanged(double)), this, SIGNAL(laserPoweredChanged()));
	connect(micLightPV_, SIGNAL(valueChanged(int)), this, SIGNAL(lightIntensityChanged(int)));
	connect(ccdControl_, SIGNAL(valueChanged(double)), this, SIGNAL(ccdFbkChanged(double)));
	connect(microscopeControl_, SIGNAL(valueChanged(double)), this, SIGNAL(microscopeFbkChanged(double)));
	connect(focusNormalControl_, SIGNAL(valueChanged(double)), this, SIGNAL(focusNormalFbkChanged(double)));
	connect(focusYControl_, SIGNAL(valueChanged(double)), this, SIGNAL(focusYFbkChanged(double)));
	connect(singleElControl_, SIGNAL(valueChanged(double)), this, SIGNAL(singleElFbkChanged(double)));
	connect(fourElControl_, SIGNAL(valueChanged(double)), this, SIGNAL(fourElFbkChanged(double)));
	connect(laserPositionControl_, SIGNAL(valueChanged(double)), this, SIGNAL(laserPositionChanged(double)));

	QList<AMControl *> list(filterMap_.values());
	for (int i = 0; i < list.size(); i++)
		connect(list.at(i), SIGNAL(connected(bool)), this, SLOT(onFiltersConnected()));
}

bool VESPERSEndstation::loadConfiguration()
{
	QFile file(QDir::currentPath() + "/endstation.config");

	if (!file.open(QFile::ReadOnly | QFile::Text))
		return false;

	QTextStream in(&file);
	QStringList contents;

	while(!in.atEnd())
		contents << in.readLine();

	file.close();

	softLimits_.clear();

	softLimits_.insert(ccdControl_, qMakePair(((QString)contents.at(2)).toDouble(), ((QString)contents.at(3)).toDouble()));
	softLimits_.insert(singleElControl_, qMakePair(((QString)contents.at(6)).toDouble(), ((QString)contents.at(7)).toDouble()));
	softLimits_.insert(fourElControl_, qMakePair(((QString)contents.at(10)).toDouble(), ((QString)contents.at(11)).toDouble()));
	softLimits_.insert(microscopeControl_, qMakePair(((QString)contents.at(14)).toDouble(), ((QString)contents.at(15)).toDouble()));

	microscopeNames_ = qMakePair((QString)contents.at(16), (QString)contents.at(17));

	updateControl(current_);

	return true;
}

AMControl *VESPERSEndstation::control(QString name) const
{
	if (name.compare("CCD motor") == 0)
		return ccdControl_;
	else if (name.compare("1-Element Vortex motor") == 0)
		return singleElControl_;
	else if (name.compare("4-Element Vortex motor") == 0)
		return fourElControl_;
	else if (name.compare("Microscope motor") == 0)
		return microscopeControl_;
	else if (name.compare("Normal Sample Stage") == 0)
		return focusNormalControl_;
	else if (name.compare("Y (normal) motor") == 0)
		return focusYControl_;

	return 0;
}
#include <QDebug>
void VESPERSEndstation::setCurrent(QString name)
{
	if (name.compare("CCD motor") == 0)
		updateControl(ccdControl_);
	else if (name.compare("1-Element Vortex motor") == 0)
		updateControl(singleElControl_);
	else if (name.compare("4-Element Vortex motor") == 0)
		updateControl(fourElControl_);
	else if (name.compare("Microscope motor") == 0)
		updateControl(microscopeControl_);
	else if (name.compare("Normal Sample Stage") == 0)
		updateControl(focusNormalControl_);
	else if (name.compare("Y (normal) motor") == 0)
		updateControl(focusYControl_);
	else
		current_ = 0;
}

void VESPERSEndstation::updateControl(AMControl *control)
{
	if (control == 0)
		return;

	current_ = control;
	emit currentControlChanged(current_);
}

void VESPERSEndstation::onFiltersConnected()
{
	bool connected = true;
	QList<AMControl *> filters(filterMap_.values());

	for (int i = 0; i < filters.size(); i++)
		connected = connected && filters.at(i)->isConnected();

	if (!wasConnected_ && connected){

		for (int i = 0; i < filters.size(); i++)
			connect(filters.at(i), SIGNAL(valueChanged(double)), this, SLOT(onFiltersChanged()));

		onFiltersChanged();
	}
	else if (wasConnected_ && !connected)
		for (int i = 0; i < filters.size(); i++)
			disconnect(filters.at(i), SIGNAL(valueChanged(double)), this, SLOT(onFiltersChanged()));
}

void VESPERSEndstation::onFiltersChanged()
{
	int sum = 0;

	if ((int)filterMap_.value("50A")->value() == 1)
		sum += 1;
	if ((int)filterMap_.value("50B")->value() == 1)
		sum += 1;
	if ((int)filterMap_.value("100A")->value() == 1)
		sum += 2;
	if ((int)filterMap_.value("100B")->value() == 1)
		sum += 2;
	if ((int)filterMap_.value("250A")->value() == 1)
		sum += 5;
	if ((int)filterMap_.value("250B")->value() == 1)
		sum += 5;

	filterThickness_ = sum*50;
	emit filterThicknessChanged(sum);
}

void VESPERSEndstation::setFilterThickness(int index)
{
	QList<AMControl *> filters(filterMap_.values());

	// Put all the filters back to an original state.  The -2 is to exclude the upper and lower shutters.
	for (int i = 0; i < filters.size(); i++)
		if ((int)filters.at(i)->value() == 1)
			toggleControl(filters.at(i));

	switch(index){
	case 0: // Filters are already taken out with previous loop.
		break;
	case 1: // 50 um
		toggleControl(filterMap_.value("50A"));
		break;
	case 2: // 100 um
		toggleControl(filterMap_.value("100A"));
		break;
	case 3: // 150 um
		toggleControl(filterMap_.value("50A"));
		toggleControl(filterMap_.value("100A"));
		break;
	case 4: // 200 um
		toggleControl(filterMap_.value("100A"));
		toggleControl(filterMap_.value("100B"));
		break;
	case 5: // 250 um
		toggleControl(filterMap_.value("250A"));
		break;
	case 6: // 300 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("50A"));
		break;
	case 7: // 350 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("100A"));
		break;
	case 8: // 400 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("100A"));
		toggleControl(filterMap_.value("50A"));
		break;
	case 9: // 450 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("100A"));
		toggleControl(filterMap_.value("100B"));
		break;
	case 10: // 500 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("250B"));
		break;
	case 11: // 550 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("250B"));
		toggleControl(filterMap_.value("50A"));
		break;
	case 12: // 600 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("250B"));
		toggleControl(filterMap_.value("100A"));
		break;
	case 13: // 650 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("250B"));
		toggleControl(filterMap_.value("100A"));
		toggleControl(filterMap_.value("50A"));
		break;
	case 14: // 700 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("250B"));
		toggleControl(filterMap_.value("100A"));
		toggleControl(filterMap_.value("100B"));
		break;
	case 15: // 750 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("250B"));
		toggleControl(filterMap_.value("100A"));
		toggleControl(filterMap_.value("100B"));
		toggleControl(filterMap_.value("50A"));
		break;
	case 16: // 800 um
		toggleControl(filterMap_.value("250A"));
		toggleControl(filterMap_.value("250B"));
		toggleControl(filterMap_.value("100A"));
		toggleControl(filterMap_.value("100B"));
		toggleControl(filterMap_.value("50A"));
		toggleControl(filterMap_.value("50B"));
		break;
	}
}

void VESPERSEndstation::setShutterState(bool state)
{
	// Don't do anything if the shutter is already in the right state.
	if (shutterState() == state)
		return;

	// 0 = OUT.  For this to work properly, the upper shutter is to remain fixed at the out position and the lower shutter changes.  Therefore if upper is IN, put it out.
	if ((int)filterShutterUpper_->value() == 1)
		toggleControl(filterShutterUpper_);

	toggleControl(filterShutterLower_);
}

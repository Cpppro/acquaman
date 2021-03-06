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


#include "CLSSynchronizedDwellTime.h"
#include "beamline/AMDetectorTriggerSource.h"
#include "actions3/actions/AMControlMoveAction3.h"

CLSSynchronizedDwellTime::CLSSynchronizedDwellTime(QString baseName, QObject *parent)
	: AMSynchronizedDwellTime(parent)
{
	baseName_ = baseName;

	triggerSource_ = new AMDetectorTriggerSource(QString("%1TriggerSource").arg(baseName), this);
	connect(triggerSource_, SIGNAL(triggered(AMDetectorDefinitions::ReadMode)), this, SLOT(onTriggerSourceTriggered(AMDetectorDefinitions::ReadMode)));

	dwellTimeSource_ = new AMDetectorDwellTimeSource(QString("%1DwellTimeSource").arg(baseName), this);
	connect(dwellTimeSource_, SIGNAL(setDwellTime(double)), this, SLOT(onDwellTimeSourceSetDwellTime(double)));

	dwellTime_ = new AMSinglePVControl("Dwell Time", baseName+":setTime", this, 0.1);
	startScan_ = new AMSinglePVControl("Start Scan", baseName+":startScan", this, 0.1);
	mode_ = new AMSinglePVControl("Dwell Mode", baseName+":setMode", this, 0.1);

	connect(dwellTime_, SIGNAL(connected(bool)), this, SLOT(onConnectedChanged()));
	connect(startScan_, SIGNAL(connected(bool)), this, SLOT(onConnectedChanged()));
	connect(mode_, SIGNAL(connected(bool)), this, SLOT(onConnectedChanged()));

	//connect(dwellTime_, SIGNAL(valueChanged(double)), this, SIGNAL(timeChanged(double)));
	connect(dwellTime_, SIGNAL(valueChanged(double)), this, SLOT(onDwellTimeChanged(double)));
	connect(startScan_, SIGNAL(valueChanged(double)), this, SLOT(onScanningChanged(double)));
	connect(mode_, SIGNAL(valueChanged(double)), this, SLOT(onModeChanged(double)));
}

QString CLSSynchronizedDwellTime::keyAt(int index) const{
	return elementAt(index)->key();
}

QStringList CLSSynchronizedDwellTime::keys() const{
	QStringList retVal;
	for(int x = 0; x < elementCount(); x++)
		retVal.append(elementAt(x)->key());

	return retVal;
}

CLSSynchronizedDwellTimeElement *CLSSynchronizedDwellTime::elementByName(const QString &name) const
{
	for (int i = 0, size = elements_.size(); i < size; i++)
		if (elements_.at(i)->name() == name)
			return elements_.at(i);

	return 0;
}

AMDetectorTriggerSource* CLSSynchronizedDwellTime::triggerSource(){
	return triggerSource_;
}

AMDetectorDwellTimeSource* CLSSynchronizedDwellTime::dwellTimeSource(){
	return dwellTimeSource_;
}

void CLSSynchronizedDwellTime::addElement(int index)
{
	elements_ << new CLSSynchronizedDwellTimeElement(baseName_, index, this);
	connect(elements_.last(), SIGNAL(statusChanged(bool)), this, SLOT(onStatusChanged()));
	connect(elements_.last(), SIGNAL(connected(bool)), this, SLOT(onConnectedChanged()));
}

AMBeamlineActionItem *CLSSynchronizedDwellTime::createMasterTimeAction(double time)
{
	if (!dwellTime_->isConnected())
		return 0;

	AMBeamlineControlMoveAction *action = new AMBeamlineControlMoveAction(dwellTime_);
	action->setSetpoint(time);

	return action;
}

AMAction3 *CLSSynchronizedDwellTime::createMasterTimeAction3(double time){
	if (!dwellTime_->isConnected())
		return 0;

	AMControlInfo setpoint = dwellTime_->toInfo();
	setpoint.setValue(time);
	AMControlMoveActionInfo3 *actionInfo = new AMControlMoveActionInfo3(setpoint);
	AMAction3 *action = new AMControlMoveAction3(actionInfo, dwellTime_);

	return action;
}

AMBeamlineActionItem *CLSSynchronizedDwellTime::createScanningAction(bool scan)
{
	if (!startScan_->isConnected())
		return 0;

	AMBeamlineControlMoveAction *action = new AMBeamlineControlMoveAction(startScan_);
	action->setSetpoint(scan == true ? 1.0 : 0.0);

	return action;
}

AMAction3 *CLSSynchronizedDwellTime::createScanningAction3(bool scan){
	if (!startScan_->isConnected())
		return 0;

	AMControlInfo setpoint = startScan_->toInfo();
	setpoint.setValue(scan == true ? 1.0 : 0.0);
	AMControlMoveActionInfo3 *actionInfo = new AMControlMoveActionInfo3(setpoint);
	AMAction3 *action = new AMControlMoveAction3(actionInfo, startScan_);

	return action;
}

AMBeamlineActionItem *CLSSynchronizedDwellTime::createModeAction(CLSSynchronizedDwellTime::Mode mode)
{
	if (!mode_->isConnected())
		return 0;

	AMBeamlineControlMoveAction *action = new AMBeamlineControlMoveAction(mode_);
	action->setSetpoint(mode == Continuous ? 0.0 : 1.0);

	return action;
}

AMAction3 *CLSSynchronizedDwellTime::createModeAction3(CLSSynchronizedDwellTime::Mode mode){
	if (!mode_->isConnected())
		return 0;

	AMControlInfo setpoint = mode_->toInfo();
	setpoint.setValue(mode == Continuous ? 0.0 : 1.0);
	AMControlMoveActionInfo3 *actionInfo = new AMControlMoveActionInfo3(setpoint);
	AMAction3 *action = new AMControlMoveAction3(actionInfo, mode_);

	return action;
}

void CLSSynchronizedDwellTime::onScanningChanged(double status){
	emit scanningChanged((int)status == 1 ? true : false);
	if((int)status == 0)
		triggerSource_->setSucceeded();
}

void CLSSynchronizedDwellTime::onDwellTimeChanged(double dwellTime){
	emit timeChanged(dwellTime);
	dwellTimeSource_->setSucceeded();
}

void CLSSynchronizedDwellTime::onTriggerSourceTriggered(AMDetectorDefinitions::ReadMode readMode){
	if(!isConnected() || isScanning() || readMode != AMDetectorDefinitions::SingleRead)
		return;

	//start();
	if(mode() == CLSSynchronizedDwellTime::Continuous){
		connect(this, SIGNAL(modeChanged(CLSSynchronizedDwellTime::Mode)), this, SLOT(triggerSynchronizedDwellTimeAcquisition(CLSSynchronizedDwellTime::Mode)));
		setMode(CLSSynchronizedDwellTime::SingleShot);
	}
	else
		triggerSynchronizedDwellTimeAcquisition(mode());
}

void CLSSynchronizedDwellTime::triggerSynchronizedDwellTimeAcquisition(CLSSynchronizedDwellTime::Mode newMode){
	disconnect(this, SIGNAL(modeChanged(CLSSynchronizedDwellTime::Mode)), this, SLOT(triggerSynchronizedDwellTimeAcquisition(CLSSynchronizedDwellTime::Mode)));
	if(newMode == CLSSynchronizedDwellTime::Continuous)
		return;

	start();
}

void CLSSynchronizedDwellTime::onDwellTimeSourceSetDwellTime(double dwellSeconds){
	if(!isConnected() || isScanning())
		return;

	if(dwellSeconds != time())
		setTime(dwellSeconds);
	else
		dwellTimeSource_->setSucceeded();
}

CLSSynchronizedDwellTimeElement::CLSSynchronizedDwellTimeElement(QString baseName, int index, QObject *parent)
	: QObject(parent)
{
	// 65 is 'A' in ascii.  Therefore the index offset will give the appropriate letter for the PV name since they are named 'A', 'B', 'C', etc.
	name_ = new AMProcessVariable(baseName+":device"+QChar(65+index), true, this);
	enable_ = new AMSinglePVControl(QString("Dwell Element Enable %1").arg(index), baseName+":enable"+QChar(65+index), this, 0.1);
	time_ = new AMSinglePVControl(QString("Dwell Element Time %1").arg(index), baseName+":set"+QChar(65+index), this, 0.1);
	status_ = new AMProcessVariable(baseName+":status"+QChar(65+index), true, this);
	trigger_ = new AMProcessVariable(baseName+":start"+QChar(65+index)+".LNK2", true, this);
	configuration_ = new CLSSynchronizedDwellTimeConfiguration(baseName, index, this);

	connect(name_, SIGNAL(connected()), this, SLOT(onConnectedChanged()));
	connect(time_, SIGNAL(connected(bool)), this, SLOT(onConnectedChanged()));
	connect(enable_, SIGNAL(connected(bool)), this, SLOT(onConnectedChanged()));
	connect(status_, SIGNAL(connected()), this, SLOT(onConnectedChanged()));
	connect(trigger_, SIGNAL(connected()), this, SLOT(onConnectedChanged()));
	connect(configuration_, SIGNAL(connected(bool)), this, SLOT(onConnectedChanged()));

	connect(name_, SIGNAL(valueChanged(QString)), this, SIGNAL(nameChanged(QString)));
	connect(time_, SIGNAL(valueChanged(double)), this, SIGNAL(timeChanged(double)));
	connect(enable_, SIGNAL(valueChanged(double)), this, SLOT(onEnabledChanged(double)));
	connect(status_, SIGNAL(valueChanged(int)), this, SLOT(onStatusChanged(int)));
	connect(trigger_, SIGNAL(valueChanged(QString)), this, SIGNAL(triggerChanged(QString)));
}

AMBeamlineActionItem *CLSSynchronizedDwellTimeElement::createTimeAction(double time)
{
	if (!time_->isConnected())
		return 0;

	AMBeamlineControlMoveAction *action = new AMBeamlineControlMoveAction(time_);
	action->setSetpoint(time);

	return action;
}

AMAction3 *CLSSynchronizedDwellTimeElement::createTimeAction3(double time)
{
	if (!time_->isConnected())
		return 0;

	AMControlInfo setpoint = time_->toInfo();
	setpoint.setValue(time);
	AMControlMoveActionInfo3 *actionInfo = new AMControlMoveActionInfo3(setpoint);
	AMAction3 *action = new AMControlMoveAction3(actionInfo, time_);

	return action;
}

AMBeamlineActionItem *CLSSynchronizedDwellTimeElement::createEnableAction(bool enable)
{
	if (!enable_->isConnected())
		return 0;

	AMBeamlineControlMoveAction *action = new AMBeamlineControlMoveAction(enable_);
	action->setSetpoint(enable == true ? 1.0 : 0.0);

	return action;
}

AMAction3 *CLSSynchronizedDwellTimeElement::createEnableAction3(bool enable)
{
	if (!enable_->isConnected())
		return 0;

	AMControlInfo setpoint = enable_->toInfo();
	setpoint.setValue(enable == true ? 1.0 : 0.0);
	AMControlMoveActionInfo3 *actionInfo = new AMControlMoveActionInfo3(setpoint);
	AMAction3 *action = new AMControlMoveAction3(actionInfo, enable_);

	return action;
}

void CLSSynchronizedDwellTimeElement::configure(const CLSSynchronizedDwellTimeConfigurationInfo &info)
{
	configuration_->configure(info);
}

QString CLSSynchronizedDwellTimeElement::key() const{
	if (!trigger_->isConnected())
		return "";

	return trigger_->getString();
}

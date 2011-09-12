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


#include "PGTDetector.h"

PGTDetector::PGTDetector(const QString &name, AMControlSet *readingsControls, AMControlSet *settingsControls, AMBeamlineActionItem *toggleOnAction, AMBeamlineActionItem *toggleOffAction, AMDetector::ReadMethod readMethod, QObject *parent) :
		PGTDetectorInfo(name, name, parent), AMDetector(name, readMethod)
{
	ownsControlSets_ = false;
	toggleOnAction_ = toggleOnAction;
	toggleOffAction_ = toggleOffAction;
	initializeFromControlSet(readingsControls, settingsControls);
}

PGTDetector::PGTDetector(const QString &name, AMControl *dataWaveform, AMControl *hv, AMControl *integrationTime, AMControl *integrationMode, AMBeamlineActionItem *toggleOnAction, AMBeamlineActionItem *toggleOffAction, AMDetector::ReadMethod readMethod, QObject *parent) :
		PGTDetectorInfo(name, name, parent), AMDetector(name, readMethod)
{
	ownsControlSets_ = true;
	AMControlSet *readingsControls = new AMControlSet(this);
	AMControlSet *settingsControls = new AMControlSet(this);
	readingsControls->addControl(dataWaveform);
	settingsControls->addControl(hv);
	settingsControls->addControl(integrationTime);
	settingsControls->addControl(integrationMode);
	toggleOnAction_ = toggleOnAction;
	toggleOffAction_ = toggleOffAction;
	initializeFromControlSet(readingsControls, settingsControls);
}

PGTDetector::~PGTDetector()
{
	/* NTBA March 14, 2011 David Chevrier
	   Need to take care of ownsControlSet_
	   */
}

const QMetaObject* PGTDetector::getMetaObject() {
	return metaObject();
}

double PGTDetector::reading() const{
	if(isConnected())
		return dataWaveformCtrl()->value();
	else
		return -1;
}

AMDetectorInfo* PGTDetector::toInfo() const{
	return new PGTDetectorInfo(*this);
}

PGTDetectorInfo PGTDetector::toPGTInfo() const{
	return PGTDetectorInfo(*this);
}

bool PGTDetector::setFromInfo(const AMDetectorInfo *info){
	const PGTDetectorInfo *di = qobject_cast<const PGTDetectorInfo*>(info);
	if(!di)
		return false;
	//integrationTimeCtrl()->move(di->integrationTime());
	//integrationModeCtrl()->move(integrationModeCtrl()->enumNames().indexOf(di->integrationMode()));
	//hvCtrl()->move(di->hvSetpoint());
	return true;
}

bool PGTDetector::setFromInfo(const PGTDetectorInfo& info){
	//integrationTimeCtrl()->move(info.integrationTime());
	//integrationModeCtrl()->move(integrationModeCtrl()->enumNames().indexOf(info.integrationMode()));
	//hvCtrl()->move(info.hvSetpoint());
	return true;
}

bool PGTDetector::isPoweredOn(){
	return poweredOn_;
}

bool PGTDetector::activate(){
	hvCtrl()->move(180);
	return true;
}

AMBeamlineActionItem* PGTDetector::turnOnAction(){
	return toggleOnAction_->createCopy();
}

AMControl* PGTDetector::dataWaveformCtrl() const {
	if(isConnected())
		return readingsControls_->at(0);
	else
		return 0;
}

AMControl* PGTDetector::hvCtrl() const {
	if(isConnected())
		return settingsControls_->at(0);
	else
		return 0;
}

AMControl* PGTDetector::integrationTimeCtrl() const {
	if(isConnected())
		return settingsControls_->at(1);
	else
		return 0;
}

AMControl* PGTDetector::integrationModeCtrl() const {
	if(isConnected())
		return settingsControls_->at(2);
	else
		return 0;
}



bool PGTDetector::settingsMatchFbk(PGTDetectorInfo *settings){
	bool rVal = false;
	if( fabs(settings->integrationTime() - integrationTimeCtrl()->value()) > integrationTimeCtrl()->tolerance() )
		return rVal;
	else if( fabs( settings->integrationModeList().indexOf(settings->integrationMode()) - integrationModeCtrl()->value()) > integrationModeCtrl()->tolerance() )
		return rVal;
	else if( fabs(settings->hvSetpoint() - hvCtrl()->value()) > hvCtrl()->tolerance())
		return rVal;
	else{
		rVal = true;
		return rVal;
	}
}

QString PGTDetector::description() const{
	return AMDetectorInfo::description();
}

void PGTDetector::setDescription(const QString &description){
	AMDetectorInfo::setDescription(description);
}

bool PGTDetector::setControls(PGTDetectorInfo *pgtSettings){
//	hvCtrl()->move( pgtSettings->hvSetpoint() );
//	integrationTimeCtrl()->move( pgtSettings->integrationTime() );
//	integrationModeCtrl()->move( integrationModeCtrl()->enumNames().indexOf(pgtSettings->integrationMode()) );
	return true;
}

void PGTDetector::onControlsConnected(bool connected){
	Q_UNUSED(connected)
	bool allConnected = readingsControls_->isConnected() && settingsControls_->isConnected();
	if(allConnected != isConnected())
		setConnected(allConnected);
}

void PGTDetector::onSettingsControlValuesChanged(){
	if(isConnected()){
		setHVSetpoint(hvCtrl()->value());
		setIntegrationTime(integrationTimeCtrl()->value());
		setIntegrationMode(integrationModeCtrl()->enumNameAt(integrationModeCtrl()->value()));
		bool lastPoweredOn = poweredOn_;
		poweredOn_ = (hvCtrl()->value() >= 170);
		if(lastPoweredOn != poweredOn_)
			emit poweredOnChanged(poweredOn_);
		emitSettingsChanged();
	}
}

void PGTDetector::onReadingsControlValuesChanged(){
	if(isConnected())
		emitReadingsChanged();
}

bool PGTDetector::initializeFromControlSet(AMControlSet *readingsControls, AMControlSet *settingsControls){
	readingsControls_ = 0; //NULL
	settingsControls_ = 0; //NULL
	poweredOn_ = false;

	if(readingsControls->count() == 1 && settingsControls->count() == 3){
		readingsControls_ = readingsControls;
		settingsControls_ = settingsControls;
		connect(signalSource(), SIGNAL(connected(bool)), this, SLOT(onSettingsControlValuesChanged()));
		connect(readingsControls_, SIGNAL(connected(bool)), this, SLOT(onControlsConnected(bool)));
		connect(settingsControls_, SIGNAL(connected(bool)), this, SLOT(onControlsConnected(bool)));
		connect(readingsControls_, SIGNAL(controlSetValuesChanged(AMControlInfoList)), this, SLOT(onReadingsControlValuesChanged()));
		connect(settingsControls_, SIGNAL(controlSetValuesChanged(AMControlInfoList)), this, SLOT(onSettingsControlValuesChanged()));
		onControlsConnected(readingsControls_->isConnected() && settingsControls_->isConnected());
		if(isConnected()){
			onReadingsControlValuesChanged();
			onSettingsControlValuesChanged();
		}

		return true;
	}

	return false;
}

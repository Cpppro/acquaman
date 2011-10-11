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


#include "AMDacqScanController.h"
#include <QDebug>
#include <QTime>
#include <QStringList>
#include <QDir>


AMDacqScanController::AMDacqScanController(AMScanConfiguration *cfg, QObject *parent) : AMScanController(cfg, parent)
{
	_pCfg_ = &generalCfg_;
	_pScan_ = &generalScan_;

	useDwellTimes_ = false;
	dwellTimeTrigger_ = 0; //NULL
	dwellTimeConfirmed_ = 0; //NULL

	dacqCancelled_ = false;
	QEpicsAcqLocal *lAcq = new QEpicsAcqLocal((QWidget*)parent);
	advAcq_ = new QEpicsAdvAcq(lAcq);
	connect(advAcq_, SIGNAL(onStart()), this, SLOT(onDacqStart()));
	connect(advAcq_, SIGNAL(onStop()), this, SLOT(onDacqStop()));
	connect(advAcq_, SIGNAL(onPause(int)), this, SLOT(onDacqPause(int)));
	connect(advAcq_, SIGNAL(sendCompletion(int)), this, SLOT(onDacqSendCompletion(int)));
	connect(advAcq_, SIGNAL(onState(QString)), this, SLOT(onDacqState(QString)));
	usingSpectraDotDatFile_ = false;
}

// Pass a pair of controls to be used as the dwell time trigger and dwell time confirmation.
// Expectation is that the dacq file will include lines like:
//  # Action StartPass SetPV "<Dwell Time Confirmed PV>" "0"
//  # Action StartPass SetPV "<Dwell Time Trigger PV>" "1"
//  # Action StartPass Delay 0.5
//  # Action StartPass WaitPV "<Dwell Time Confirmed PV>" "1"
// Basically, reset the confirmation, then trigger a request for change, then wait someone to confirm that the change has been made
// On this side, we will connect to the trigger, look to see it has changed to "1", do what changes we need to, and then reset the trigger and set the confirm to "1"
// Check SGMXASDacqScanController for more information, tested with that model.
void AMDacqScanController::useDwellTimes(AMControl *dwellTimeTrigger, AMControl *dwellTimeConfirmed){
	if(dwellTimeTrigger && dwellTimeConfirmed && dwellTimeTrigger->isConnected() && dwellTimeConfirmed->isConnected()){
		useDwellTimes_ = true;
		dwellTimeTrigger_ = dwellTimeTrigger;
		dwellTimeConfirmed_ = dwellTimeConfirmed;
	}
	else
		useDwellTimes_ = false;
}

bool AMDacqScanController::startImplementation(){
		acqBaseOutput *abop = acqOutputHandlerFactory::new_acqOutput("AMScanSpectrum", "File");
		if( abop)
		{
			if(useDwellTimes_)
				connect(dwellTimeTrigger_, SIGNAL(valueChanged(double)), this, SLOT(onDwellTimeTriggerChanged(double)));

			acqRegisterOutputHandler( advAcq_->getMaster(), (acqKey_t) abop, &abop->handler);                // register the handler with the acquisition

			QFileInfo fullPath(AMUserSettings::defaultRelativePathForScan(QDateTime::currentDateTime()));	// ex: 2010/09/Mon_03_12_24_48_0000   (Relative, and with no extension)

			QString path = fullPath.path();// just the path, not the file name. Still relative.
			QString file = fullPath.fileName() + ".dat"; // just the file name, now with an extension

			abop->setProperty( "File Template", file.toStdString());
			abop->setProperty( "File Path", (AMUserSettings::userDataFolder + "/" + path).toStdString());	// given an absolute path here

			pScan_()->setFilePath(fullPath.filePath()+".dat");	// relative path and extension (is what the database wants)
			if(usingSpectraDotDatFile_){
				pScan_()->setAdditionalFilePaths( QStringList() << fullPath.filePath()+"_spectra.dat" );
			}

			((AMAcqScanSpectrumOutput*)abop)->setScan(pScan_());
			((AMAcqScanSpectrumOutput*)abop)->setScanController(this);
			advAcq_->Start();
			return true;
		}
		else{
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1, "AMDacqScanController: could not create output handler."));
			return false;
		}
}

bool AMDacqScanController::canPause(){
	return true;
}

void AMDacqScanController::pauseImplementation(){
	advAcq_->Pause(1);
}

void AMDacqScanController::resumeImplementation(){
	advAcq_->Pause(0);
}

/// Cancel scan if currently running or paused
void AMDacqScanController::cancelImplementation()
{
	dacqCancelled_ = true;
	advAcq_->Stop();
}

int AMDacqScanController::detectorReadMethodToDacqReadMethod(AMDetector::ReadMethod readMethod){
	switch(readMethod){
	case AMDetector::ImmediateRead :
		return 0;
	case AMDetector::RequestRead :
		return 1;
	case AMDetector::WaitRead :
		return 2;
	default:
		return 0;
	}
}

bool AMDacqScanController::event(QEvent *e){
	if(e->type() == (QEvent::Type)AM::AcqEvent){
		QMap<int, double> aeData = ((AMAcqEvent*)e)->dataPackage_;
		QMap<int, QList<double> > aeSpectra = ((AMAcqEvent*)e)->spectraPackage_;
		QMap<int, double>::const_iterator i = aeData.constBegin();
		QMap<int, QList<double> >::const_iterator j = aeSpectra.constBegin();
		if(i.key() == 0 && aeData.count() > 1){
			AMnDIndex insertIndex = toScanIndex(aeData);
			pScan_()->rawData()->beginInsertRowsAsNecessaryForScanPoint(insertIndex);
			/// \bug CRITICAL: This is ASSUMING ONE AXIS, need to fix that somewhere
			pScan_()->rawData()->setAxisValue(0, insertIndex.row(), i.value());
			++i;
			while(i != aeData.constEnd()){
				pScan_()->rawData()->setValue(insertIndex, i.key()-1, AMnDIndex(), i.value());
				++i;
			}
			while(j != aeSpectra.constEnd()){
				for(int x = 0; x < j.value().count(); x++)
					pScan_()->rawData()->setValue(insertIndex, j.key()-1, AMnDIndex(x), j.value().at(x));
				++j;
			}
			pScan_()->rawData()->endInsertRows();
		}
		e->accept();
		return true;
	}
	else
		return AMScanController::event(e);
}

AMnDIndex AMDacqScanController::toScanIndex(QMap<int, double> aeData){
	//Simple indexer, assumes there is ONLY ONE scan dimension and appends to the end
	Q_UNUSED(aeData);
	return AMnDIndex(pScan_()->rawData()->scanSize(0));
}

void AMDacqScanController::onDacqStart()
{
	qDebug() << "Heard that the dacq started";
	startTime_.start();
	setStarted();
}

void AMDacqScanController::onDacqStop()
{
	if(dacqCancelled_)
		setCancelled();
	else
		setFinished();
}

void AMDacqScanController::onDacqPause(int mode)
{
	if(mode == 1)
		setPaused();
	else if(mode == 0)
		setResumed();
}

void AMDacqScanController::onDacqSendCompletion(int completion){
	double tc = ((double)startTime_.elapsed())/1000;
	double remaining = (completion != 0) ? (100*tc)/((double)completion) - tc : tc*100000;
	emit timeRemaining(remaining);
	emit progress(tc, tc+remaining);
}

void AMDacqScanController::onDacqState(const QString& state){
	Q_UNUSED(state)//qDebug() << "State of dacq is " << state;
}

void AMDacqScanController::onDwellTimeTriggerChanged(double newValue){
	qDebug() << "Looks like dwell time trigger has changed to " << newValue;
	if( fabs(newValue - 1.0) < 0.1 ){
		qDebug() << "Confirm dwell time has been set and reset trigger";
		dwellTimeTrigger_->move(0);
		dwellTimeConfirmed_->move(1);
	}
}

/*
Copyright 2010, 2011 Mark Boots, David Chevrier.

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


#include "SGMXASDacqScanController.h"

SGMXASDacqScanController::SGMXASDacqScanController(SGMXASScanConfiguration *cfg, QObject *parent) :
		AMDacqScanController(cfg, parent) , SGMXASScanController(cfg)
{
	_pCfg_ = &specificCfg_;
	_pScan_ = &specificScan_;
}

void SGMXASDacqScanController::initializeImplementation(){
	if(SGMXASScanController::beamlineInitialize() && initializationActions_){
		#warning "Do we need to also clear any raw data sources here, or just the raw data itself?"
		pScan_()->clearRawDataPoints();
		connect(initializationActions_, SIGNAL(listSucceeded()), this, SLOT(onInitializationActionsSucceeded()));
		initializationActions_->start();
	}
}

void SGMXASDacqScanController::startImplementation(){
	if(SGMBeamline::sgm()->isBeamlineScanning()){
		qDebug() << "Beamline already scanning";
		return;
	}
	bool loadSuccess;
	QString homeDir = QDir::homePath();
	if( QDir(homeDir+"/dev").exists())
		homeDir.append("/dev");
	else if( QDir(homeDir+"/beamline/programming").exists())
		homeDir.append("/beamline/programming");

	if(pCfg_()->allDetectorConfigurations().isActiveNamed(SGMBeamline::sgm()->pgtDetector()->detectorName()))
		loadSuccess = advAcq_->setConfigFile(homeDir.append("/acquaman/devConfigurationFiles/pgt.cfg"));
	else
		loadSuccess = advAcq_->setConfigFile(homeDir.append("/acquaman/devConfigurationFiles/defaultEnergy.cfg"));
	if(!loadSuccess){
		qDebug() << "LIBRARY FAILED TO LOAD CONFIG FILE";
		return;
	}

	for(int i = 0; i < pCfg_()->allDetectors()->count(); i++){
		AMDetector *dtctr = pCfg_()->allDetectors()->detectorAt(i);

		if(dtctr->detectorName() == SGMBeamline::sgm()->pgtDetector()->detectorName()){
			advAcq_->appendRecord(SGMBeamline::sgm()->pvName(dtctr->detectorName()), true, true, 0);
		}
		else{
			advAcq_->appendRecord(SGMBeamline::sgm()->pvName(dtctr->detectorName()), true, false, 0);
		}
		for(int x = 0; x < pCfg_()->regionCount(); x++){
			if(advAcq_->getNumRegions() == x)
				advAcq_->addRegion(x, pCfg_()->regionStart(x), pCfg_()->regionDelta(x), pCfg_()->regionEnd(x), 1);
			else{
				advAcq_->setStart(x, pCfg_()->regionStart(x));
				advAcq_->setDelta(x, pCfg_()->regionDelta(x));
				advAcq_->setEnd(x, pCfg_()->regionEnd(x));
			}
		}
	}
	advAcq_->saveConfigFile("/Users/fawkes/dev/acquaman/devConfigurationFiles/davidTest.cfg");
	generalScan_ = specificScan_;

	AMDacqScanController::startImplementation();
}

AMnDIndex SGMXASDacqScanController::toScanIndex(QMap<int, double> aeData){
	// SGM XAS Scan has only one dimension (energy), simply append to the end of this
	return AMnDIndex(pScan_()->rawData()->scanSize(0));
}

void SGMXASDacqScanController::onInitializationActionsSucceeded(){
	setInitialized();
}

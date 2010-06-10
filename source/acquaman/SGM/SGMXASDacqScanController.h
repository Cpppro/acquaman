#ifndef ACQMAN_SGMXASDACQSCANCONTROLLER_H
#define ACQMAN_SGMXASDACQSCANCONTROLLER_H

#include "acquaman/AMDacqScanController.h"
//#include "AMXASScanConfiguration.h"
#include "SGMXASScanController.h"
//#include "beamline/SGMBeamline.h"

class SGMXASDacqScanController : public AMDacqScanController, public SGMXASScanController
{
Q_OBJECT
public:
	explicit SGMXASDacqScanController(SGMXASScanConfiguration *cfg, QObject *parent = 0);

	bool isInitialized() { return (initialized_ && beamlineInitialized_);}

signals:
	void initialized();

public slots:
	void initialize();
	void start(){
		bool loadSuccess;
//		qDebug() << "Start of control Start()";
		if(pScan_()->detectorNames().contains("pgt"))
			loadSuccess = advAcq_->setConfigFile("/home/reixs/beamline/programming/acquaman/devConfigurationFiles/pgt.cfg");
		else
			loadSuccess = advAcq_->setConfigFile("/home/reixs/beamline/programming/acquaman/devConfigurationFiles/defaultEnergy.cfg");
		if(!loadSuccess){
			qDebug() << "LIBRARY FAILED TO LOAD CONFIG FILE";
			return;
		}

//		foreach(QString str, pScan_()->detectors()){
		foreach(const AMAbstractDetector *dtctr, pScan_()->detectors()){
			if(dtctr->name() == SGMBeamline::sgm()->pgtDetector()->name())
			{advAcq_->appendRecord(SGMBeamline::sgm()->pvName(dtctr->name()), true, true, 0);qDebug() << "Adding " << dtctr->name() << " as spectrum";}
			else
			{advAcq_->appendRecord(SGMBeamline::sgm()->pvName(dtctr->name()), true, false, 0);qDebug() << "Adding " << dtctr->name() << " as normal";}
/*
			if(str == SGMBeamline::sgm()->pgtDetector()->name())
			{advAcq_->appendRecord(SGMBeamline::sgm()->pvName(str), true, true, 0);qDebug() << "Adding " << str << " as spectrum";}
			else
			{advAcq_->appendRecord(SGMBeamline::sgm()->pvName(str), true, false, 0);qDebug() << "Adding " << str << " as normal";}
*/
			advAcq_->saveConfigFile("/home/reixs/acquamanData/test.cfg");
		}
		qDebug() << "Using config file: " << advAcq_->getConfigFile();
		//advAcq_->clearRegions();
		for(int x = 0; x < pCfg_()->count(); x++){
			if(advAcq_->getNumRegions() == x)
				advAcq_->addRegion(x, pCfg_()->start(x), pCfg_()->delta(x), pCfg_()->end(x), 1);
			else{
				advAcq_->setStart(x, pCfg_()->start(x));
				advAcq_->setDelta(x, pCfg_()->delta(x));
				advAcq_->setEnd(x, pCfg_()->end(x));
			}
		}
		generalScan_ = specificScan_;
//		qDebug() << "Just before calling AMDacqScanController::start() from SGMXASDacqScanController::start()";
		AMDacqScanController::start();
	}

	void spit(){
		QStringList row;
		QString value;

		for(int x = 0; x < pScan_()->count(); x++){
			for(int y = 0; y < pScan_()->numChannels(); y++){
				if(!pScan_()->channel(y)->name().contains("PGT_COUNTS")){
					value.setNum(pScan_()->channel(y)->value(x));
					row << value;
				}
			}
			qDebug() << row;
			row.clear();
		}

	}

private:
	SGMXASScanConfiguration **_pCfg_;
	AMXASScan **_pScan_;

	SGMXASScanConfiguration *pCfg_() { return *_pCfg_;}
	AMXASScan* pScan_() { return *_pScan_;}
};

#endif // ACQMAN_SGMXASDACQSCANCONTROLLER_H

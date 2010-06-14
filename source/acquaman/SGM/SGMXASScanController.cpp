#include "SGMXASScanController.h"

typedef QPair<QString, QString> chPair;

SGMXASScanController::SGMXASScanController(SGMXASScanConfiguration *cfg){
	specificCfg_ = cfg;
	_pCfg_ = & specificCfg_;
	beamlineInitialized_ = false;

	QList<AMAbstractDetector*> scanDetectors;
	scanDetectors = pCfg_()->usingDetectors();
	scanDetectors.prepend(SGMBeamline::sgm()->i0Detector());
	scanDetectors.prepend(SGMBeamline::sgm()->eVFbkDetector());

	QList<QPair<QString, QString> > scanChannels;
	scanChannels = pCfg_()->defaultChannels();

	/*
	  BIG NOTE TO DAVE:
	  YOU NEW'D THE SCAN ... SOMEONE ELSE HAS TO TAKE OWNERSHIP OF IT FOR DELETION
	  opts: function call, new in scan viewer ...
	  */
	specificScan_ = new AMXASScan(scanDetectors);
	_pScan_ = &specificScan_;
	pScan_()->setName("SGM XAS Scan");

	foreach(chPair tmpCh, scanChannels){
		pScan_()->addChannel(tmpCh.first, tmpCh.second);
	}

	/*
	pScan_()->addChannel("eV", "eV");
	foreach(AMAbstractDetector *dtctr, scanDetectors){
		if(!dtctr->isSpectralOutput())
			pScan_()->addChannel(dtctr->name().toUpper(), dtctr->name());
		// What to do if it is spectral?
	}
	*/
}

bool SGMXASScanController::beamlineInitialize(){
	SGMBeamline::sgm()->exitSlitGap()->move( pCfg_()->exitSlitGap() );
	SGMBeamline::sgm()->grating()->move( pCfg_()->grating() );
	SGMBeamline::sgm()->harmonic()->move( pCfg_()->harmonic());
	SGMBeamline::sgm()->undulatorTracking()->move( pCfg_()->undulatorTracking() );
	SGMBeamline::sgm()->monoTracking()->move( pCfg_()->monoTracking() );
	SGMBeamline::sgm()->exitSlitTracking()->move( pCfg_()->exitSlitTracking() );
	beamlineInitialized_ = true;
	return beamlineInitialized_;
}

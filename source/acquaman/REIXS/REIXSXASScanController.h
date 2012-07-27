#ifndef REIXSXASSCANCONTROLLER_H
#define REIXSXASSCANCONTROLLER_H

#include "acquaman/AMSA1DScanController.h"
#include "acquaman/REIXS/REIXSXASScanConfiguration.h"

class AMListAction;

/// Re-implements AMSA1DScanController to add analysis blocks for normalized TEY and TFY spectra, and disable/re-enable the settling time and tolerance for REIXSBrokenMonoControl.
class REIXSXASScanController : public AMSA1DScanController
{
    Q_OBJECT
public:
	explicit REIXSXASScanController(REIXSXASScanConfiguration* configuration, QObject *parent = 0);
	virtual ~REIXSXASScanController();

signals:

public slots:

protected:
	/// Re-implemented from AMSA1DScanController to add analysis blocks for normalization.
	virtual void createAnalysisBlocks();

	/// Re-implemented to use the three-step move control for the first move, and then the direct energy PV after that.
	virtual AMControl* control();

	/// Re-implemented to ...
	virtual bool customInitializeImplementation();
//	/// Re-implemented to restore the settling time and re-set the tolerance for REIXSBrokenMonoControl after the scan is over or cancelled.
//	virtual void customCleanupImplementation();
//	/// Used to restore the existing values
//	double repeatMoveSettlingTime_, singleMoveSettlingTime_, tolerance_;


	/// Fills the scan meta-data (scan name, number, sampleId) either automatically, or from the pre-set configuration values.
	void initializeScanMetaData();

protected slots:
	/// Called when the initialization move (mono, slit, polarization) succeeds.
	void onInitializationMoveSucceeded();
	/// Called when the initialization move (mono, slit, polarization) fails.
	void onInitializationMoveFailed();

protected:
	REIXSXASScanConfiguration* config_;

	// used to run the custom initialization moves (moving mono grating, mirror, slit width, polarization).
	AMListAction* initialMoveAction_;

};

#endif // REIXSXASSCANCONTROLLER_H

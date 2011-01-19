#ifndef AMFASTSCAN_H
#define AMFASTSCAN_H

#include "dataman/AMScan.h"
#include "beamline/AMDetector.h"

class AMFastScan : public AMScan
{
	Q_OBJECT
	Q_CLASSINFO("AMDbObject_Attributes", "shareTableWithClass=AMScan;description=XRay Fast Scan")
public:
	/// create a new Fast scan with the following named \c detectors. Each "detector" is a source of a datapoint, that will be stored/logged, available as a column of raw data, and accessible through channel(s).
	Q_INVOKABLE explicit AMFastScan(QObject *parent = 0);

	/// Re-implemented from AMScan. Currently only the SGM2004 and ALS Bl8.0.1 file formats are supported.
	virtual bool loadDataImplementation();

signals:
	// inherits dataChanged(AMScan*)

public slots:

protected slots:

protected:

	/*
	QString legacyGrating_;
	QString legacyIntegrationTime_;

	friend class SGM2004FileLoader;
	friend class ALSBL8XASFileLoader;
	*/
};

#endif // AMFASTSCAN_H
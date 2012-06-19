#ifndef SGMOPTIMIZATIONSUPPORT_H
#define SGMOPTIMIZATIONSUPPORT_H

#include "beamline/AMControlOptimization.h"

class SGMFluxOptimization : public AMControlOptimization
{
	Q_OBJECT
public:
	SGMFluxOptimization(QObject *parent=0);

	virtual QMap<double, double> curve(QList<QVariant> stateParameters, AMRegionsList* contextParameters);
	QMap< QString, QMap<double, double> > collapse(AMRegionsList *contextParameters);

protected:
	double collapser(QMap<double, double> optCurve);
	double maximumEnergy(AMRegionsList* regions);
	double minimumEnergy(AMRegionsList* regions);
};

class SGMResolutionOptimization : public AMControlOptimization
{
	Q_OBJECT
public:
	SGMResolutionOptimization(QObject *parent=0);

	QMap<double, double> curve(QList<QVariant> stateParameters, AMRegionsList* contextParameters);
	QMap< QString, QMap<double, double> > collapse(AMRegionsList *contextParameters);

protected:
	double collapser(QMap<double, double> optCurve);
	double maximumEnergy(AMRegionsList* regions);
	double minimumEnergy(AMRegionsList* regions);
};

#endif // SGMOPTIMIZATIONSUPPORT_H

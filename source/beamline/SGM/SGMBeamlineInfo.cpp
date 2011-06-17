#include "SGMBeamlineInfo.h"

SGMBeamlineInfo::SGMBeamlineInfo(QObject *parent) :
	QObject(parent)
{
	standardEnergyParameters_.append( new SGMEnergyParameters(1.68923e-06, 2.45477e-05, -1.59392, 509.468, 3.05575, this), "lowGrating" );
	standardEnergyParameters_.append( new SGMEnergyParameters(9.16358e-07, 2.46204e-05, -1.59047, 511.292, 3.05478, this), "mediumGrating" );
	standardEnergyParameters_.append( new SGMEnergyParameters(5.9087e-07,  2.45691e-05, -1.59401, 510.465, 3.05458, this), "highGrating" );
}

SGMEnergyParameters* SGMBeamlineInfo::standardEnergyParametersByName(QString gratingName){
	if(standardEnergyParameters_.contains(gratingName))
		return standardEnergyParameters_.at(standardEnergyParameters_.indexOfKey(gratingName));
	else
		return 0; //NULL
}

SGMEnergyParameters::SGMEnergyParameters(QObject *parent) : QObject(parent)
{
}

SGMEnergyParameters::SGMEnergyParameters(double spacingParameter, double c1Parameter, double c2Parameter, double sParameter, double thetaParameter, QObject *parent) :
		QObject(parent)
{
	setSpacingParameter(spacingParameter);
	setC1Parameter(c1Parameter);
	setC2Parameter(c2Parameter);
	setSParameter(sParameter);
	setThetaParameter(thetaParameter);
}

bool SGMEnergyParameters::operator ==(const SGMEnergyParameters &other){
	if( (fabs(spacingParameter() - other.spacingParameter() ) <= 0.001e-07) &&
	    (fabs(c1Parameter() - other.c1Parameter()) <= 0.0001e-05) &&
	    (fabs(c2Parameter() - other.c2Parameter()) <= 0.0001) &&
	    (fabs(sParameter() - other.sParameter()) <= 0.01) &&
	    (fabs(thetaParameter() - other.thetaParameter()) <= 0.00001) ){
		return true;
	}
	return false;
}

bool SGMEnergyParameters::operator !=(const SGMEnergyParameters &other){
	return !(this->operator ==(other));
}

#include "VESPERSSplitIonChamber.h"

VESPERSSplitIonChamber::VESPERSSplitIonChamber(QString name, QString hvName, QString sensitivityBaseNameA, QString sensitivityBaseNameB, QString voltageNameA, QString voltageNameB, QString countNameA, QString countNameB, QObject *parent)
	: QObject(parent)
{
	name_ = name;

	hv_ = new AMProcessVariable(hvName, true, this);
	sensitivityValueA_ = new AMProcessVariable(sensitivityBaseNameA+":sens_num.VAL", true, this);
	sensitivityValueB_ = new AMProcessVariable(sensitivityBaseNameB+":sens_num.VAL", true, this);
	sensitivityUnitsA_ = new AMProcessVariable(sensitivityBaseNameA+":sens_unit.VAL", true, this);
	sensitivityUnitsB_ = new AMProcessVariable(sensitivityBaseNameB+":sens_unit.VAL", true, this);
	voltageA_ = new AMProcessVariable(voltageNameA, true, this);
	voltageB_ = new AMProcessVariable(voltageNameB, true, this);
	countsA_ = new AMProcessVariable(countNameA, true, this);
	countsB_ = new AMProcessVariable(countNameB, true, this);

	connect(hv_, SIGNAL(valueChanged(int)), this, SIGNAL(highVoltageChanged(int)));
	connect(sensitivityValueA_, SIGNAL(valueChanged(int)), this, SLOT(onSensitivityValueChanged(int)));
	connect(sensitivityValueB_, SIGNAL(valueChanged(int)), this, SLOT(onSensitivityValueChanged(int)));
	connect(sensitivityUnitsA_, SIGNAL(valueChanged(QString)), this, SIGNAL(sensitivityUnitsChanged(QString)));
	connect(sensitivityUnitsB_, SIGNAL(valueChanged(QString)), this, SIGNAL(sensitivityUnitsChanged(QString)));
	connect(voltageA_, SIGNAL(valueChanged(double)), this, SIGNAL(voltageAChanged(double)));
	connect(voltageB_, SIGNAL(valueChanged(double)), this, SIGNAL(voltageBChanged(double)));
	connect(countsA_, SIGNAL(valueChanged(double)), this, SIGNAL(countsAChanged(double)));
	connect(countsB_, SIGNAL(valueChanged(double)), this, SIGNAL(countsBChanged(double)));
	connect(countsA_, SIGNAL(valueChanged()), this, SLOT(onCountsChanged()));
	connect(countsB_, SIGNAL(valueChanged()), this, SLOT(onCountsChanged()));
}

void VESPERSSplitIonChamber::onSensitivityValueChanged(int index)
{
	switch(index){

	case 0:
		emit sensitivityValueChanged(1);
		break;
	case 1:
		emit sensitivityValueChanged(2);
		break;
	case 2:
		emit sensitivityValueChanged(5);
		break;
	case 3:
		emit sensitivityValueChanged(10);
		break;
	case 4:
		emit sensitivityValueChanged(20);
		break;
	case 5:
		emit sensitivityValueChanged(50);
		break;
	case 6:
		emit sensitivityValueChanged(100);
		break;
	case 7:
		emit sensitivityValueChanged(200);
		break;
	case 8:
		emit sensitivityValueChanged(500);
		break;
	}
}

#include "VESPERSRoperCCDDetector.h"

VESPERSRoperCCDDetector::VESPERSRoperCCDDetector(const QString &name, const QString &description, QObject *parent)
	: VESPERSRoperCCDDetectorInfo(name, description, parent), AMDetector(name)
{
	temperatureControl_ = new AMPVControl("Temperature", "IOC1607-003:det1:Temperature_RBV", "IOC1607-003:det1:Temperature", QString(), this, 1);
	imageModeControl_ = new AMPVControl("Image Mode", "IOC1607-003:det1:ImageMode_RBV", "IOC1607-003:det1:ImageMode", QString(), this, 0.1);
	triggerModeControl_ = new AMPVControl("Trigger Mode", "IOC1607-003:det1:TriggerMode_RBV", "IOC1607-003:det1:TriggerMode", QString(), this, 0.1);
	operationControl_ = new AMSinglePVControl("Operation", "IOC1607-003:det1:Acquire", this, 0.1);
	stateControl_ = new AMReadOnlyPVControl("State", "IOC1607-003:det1:DetectorState_RBV", this);
	acquireTimeControl_ = new AMSinglePVControl("Acquire Time", "IOC1607-003:det1:AcquireTime", this, 0.1);
	autoSaveControl_ = new AMPVControl("AutoSave", "IOC1607-003:det1:AutoSave_RBV", "IOC1607-003:det1:AutoSave", QString(), this, 0.1);
	saveFileControl_ = new AMPVwStatusControl("Save File", "IOC1607-003:det1:WriteFile", "IOC1607-003:det1:WriteFile", "IOC1607-003:det1:WriteFile_RBV", QString(), this, 0.1);

	// Various CCD file path PVs.
	ccdPath_ = new AMProcessVariable("IOC1607-003:det1:FilePath", true, this);
	ccdFile_ = new AMProcessVariable("IOC1607-003:det1:FileName", true, this);
	ccdNumber_ = new AMProcessVariable("IOC1607-003:det1:FileNumber", true, this);

	connect(signalSource(), SIGNAL(connected(bool)), this, SIGNAL(connected(bool)));
	connect(temperatureControl_, SIGNAL(valueChanged(double)), this, SIGNAL(temperatureChanged(double)));
	connect(temperatureControl_, SIGNAL(setpointChanged(double)), this, SIGNAL(temperatureSetpointChanged(double)));
	connect(imageModeControl_, SIGNAL(valueChanged(double)), this, SLOT(onImageModeChanged()));
	connect(triggerModeControl_, SIGNAL(valueChanged(double)), this, SLOT(onTriggerModeChanged()));
	connect(operationControl_, SIGNAL(valueChanged(double)), this, SLOT(onIsAcquiringChanged()));
	connect(stateControl_, SIGNAL(valueChanged(double)), this, SLOT(onStateChanged()));
	connect(acquireTimeControl_, SIGNAL(valueChanged(double)), this, SIGNAL(acquireTimeChanged(double)));
	connect(autoSaveControl_, SIGNAL(valueChanged(double)), this, SLOT(onAutoSaveEnabledChanged()));
	connect(saveFileControl_, SIGNAL(valueChanged(double)), this, SLOT(onSaveFileStateChanged()));

	connect(ccdPath_, SIGNAL(valueChanged()), this, SLOT(onCCDPathChanged()));
	connect(ccdFile_, SIGNAL(valueChanged()), this, SLOT(onCCDNameChanged()));
	connect(ccdNumber_, SIGNAL(valueChanged(int)), this, SIGNAL(ccdNumberChanged(int)));
}

VESPERSRoperCCDDetector::ImageMode VESPERSRoperCCDDetector::imageMode() const
{
	ImageMode mode = Normal;

	switch((int)imageModeControl_->value()){

	case 0:
		mode = Normal;
		break;

	case 1:
		mode = Continuous;
		break;

	case 2:
		mode = Focus;
		break;
	}

	return mode;
}

VESPERSRoperCCDDetector::TriggerMode VESPERSRoperCCDDetector::triggerMode() const
{
	TriggerMode mode = FreeRun;

	switch((int)triggerModeControl_->value()){

	case 0:
		mode = FreeRun;
		break;

	case 1:
		mode = ExtSync;
		break;

	case 2:
		mode = BulbTrigger;
		break;

	case 3:
		mode = SingleTrigger;
		break;
	}

	return mode;
}

VESPERSRoperCCDDetector::State VESPERSRoperCCDDetector::state() const
{
	State detectorState = Idle;

	switch((int)stateControl_->value()){

	case 0:
		detectorState = Idle;
		break;

	case 1:
		detectorState = Acquire;
		break;

	case 2:
		detectorState = Readout;
		break;

	case 3:
		detectorState = Correct;
		break;

	case 4:
		detectorState = Saving;
		break;

	case 5:
		detectorState = Aborting;
		break;

	case 6:
		detectorState = Error;
		break;

	case 7:
		detectorState = Waiting;
		break;
	}

	return detectorState;
}

bool VESPERSRoperCCDDetector::setFromInfo(const AMDetectorInfo *info)
{
	const VESPERSRoperCCDDetectorInfo *detectorInfo = qobject_cast<const VESPERSRoperCCDDetectorInfo *>(info);

	// Check to make sure the detector info was valid.  If it isn't, then don't do anything to the detector.
	if (!detectorInfo)
		return false;

	setTemperature(detectorInfo->temperature());
	setAcquireTime(detectorInfo->acquireTime());

	return true;
}

void VESPERSRoperCCDDetector::setFromRoperInfo(const VESPERSRoperCCDDetectorInfo &info)
{
	setTemperature(info.temperature());
	setAcquireTime(info.acquireTime());
}

QString VESPERSRoperCCDDetector::AMPVtoString(AMProcessVariable *pv)
{
	int current;
	QString name;

	for (unsigned i = 0; i < pv->count(); i++){

		current = pv->getInt(i);
		if (current == 0)
			break;

		name += QString::fromAscii((const char *) &current);
	}

	return name;
}

void VESPERSRoperCCDDetector::StringtoAMPV(AMProcessVariable *pv, QString toConvert)
{
	int converted[256];

	QByteArray toConvertBA = toConvert.toAscii();
	for (int i = 0; i < 256; i++){

		if (i < toConvertBA.size())
			converted[i] = toConvertBA.at(i);
		else
			converted[i] = 0;
	}

	pv->setValues(converted, 256);
}

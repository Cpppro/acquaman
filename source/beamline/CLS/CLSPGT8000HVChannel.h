#ifndef CLSPGT8000HVCHANNEL_H
#define CLSPGT8000HVCHANNEL_H

#include "beamline/AMHighVoltageChannel.h"

class AMControl;
class AMProcessVariable;

class CLSPGT8000HVChannel : public AMHighVoltageChannel
{
Q_OBJECT
public:
	CLSPGT8000HVChannel(const QString &name, const QString &pvBaseName, QObject *parent = 0);

	virtual QString description() const;

	virtual bool isConnected() const;

	virtual double demand() const;
	virtual double voltage() const;

	virtual QString status() const;
	virtual double current() const;

public slots:
	virtual void setDescription(const QString &description);
	virtual void setDemand(double demand);
	virtual void setPowerState(highVoltageChannelPowerState powerState);

protected slots:
	void onChildConnected(bool connect);
	void onDemandChanged(double demand);
	void onVoltageChanged(double voltage);
	void onToggleChanged(double toggle);

protected:
	bool wasConnected_;
	bool poweringDown_;
	QString description_;
	AMControl *demand_;
	AMControl *voltage_;
	AMControl *toggle_;
};

#endif // CLSPGT8000HVCHANNEL_H

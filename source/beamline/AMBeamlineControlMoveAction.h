#ifndef AMBEAMLINECONTROLMOVEACTION_H
#define AMBEAMLINECONTROLMOVEACTION_H

#include <QProgressBar>
#include <QTime>

#include "AMBeamlineActionItem.h"
#include "AMControl.h"

class AMBeamlineControlMoveAction : public AMBeamlineActionItem
{
Q_OBJECT
public:
	explicit AMBeamlineControlMoveAction(AMControl *control, QObject *parent = 0);

	virtual QString type() const;
	virtual AMControl* control();
	virtual double setpoint();

signals:
	void progress(double, double);

public slots:
	virtual void start();
	virtual void cancel();
	virtual void setControl(AMControl *control);
	virtual bool setSetpoint(double setpoint);
	virtual void cleanup(){}

protected slots:
//	virtual void initialize();
	void delayedStart(bool ready);
	virtual void onMovingChanged(bool moving);
	virtual void onConnected(bool connected);
	virtual void checkReady();
	virtual void onStarted();
	virtual void onSucceeded();
	virtual void onFailed(int explanation);
	virtual void onFinished();
	virtual void calculateProgress();

protected:
	AMControl *control_;
	double setpoint_;
	double startPoint_;
	QTimer progressTimer_;

private:
	QString type_;
};

class AMBeamlineControlMoveActionView : public AMBeamlineActionView
{
	Q_OBJECT
public:
	AMBeamlineControlMoveActionView(AMBeamlineControlMoveAction *moveAction, int index = 0, QWidget *parent = 0);

	virtual QString viewType() const;

public slots:
	void setIndex(int index);
	void setAction(AMBeamlineControlMoveAction *moveAction);

signals:
	void actionStarted(AMBeamlineActionItem *action);
	void actionSucceeded(AMBeamlineActionItem *action);
	void actionFailed(AMBeamlineActionItem *action);

protected slots:
	virtual void onInfoChanged();
	virtual void updateProgressBar(double elapsed, double total);
	virtual void onStopCancelButtonClicked();
	virtual void onPlayPauseButtonClicked();

	virtual void onStarted();
	virtual void onSucceeded();
	virtual void onFailed(int explanation);

protected:
	AMBeamlineControlMoveAction *moveAction_;

	QLabel *infoLabel_;
	QProgressBar *progressBar_;
	QLabel *timeRemainingLabel_;
	QPushButton *stopCancelButton_;
	QPushButton *playPauseButton_;
	QHBoxLayout *hl_;

	QIcon closeIcon_, stopIcon_, startIcon_, pauseIcon_;

private:
	QString viewType_;
};

#endif // AMBEAMLINECONTROLMOVEACTION_H

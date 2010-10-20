#include "AMBeamlineControlSetMoveAction.h"

AMBeamlineControlSetMoveAction::AMBeamlineControlSetMoveAction(AMControlSet *controlSet, QObject *parent) :
	AMBeamlineActionItem(parent)
{
	controlSet_ = NULL;
	setpoint_ = NULL;
	type_ = "controlSetMoveAction";
	if(controlSet){
		setControlSet(controlSet);
		setpoint_ = new AMControlSetInfo(controlSet_->info(), this);
	}
	else
		setpoint_ = NULL;
}

QString AMBeamlineControlSetMoveAction::type() const{
	return AMBeamlineActionItem::type()+"."+type_;
}

AMControlSet* AMBeamlineControlSetMoveAction::controlSet(){
	return controlSet_;
}

AMControlSetInfo* AMBeamlineControlSetMoveAction::setpoint(){
	return setpoint_;
}

void AMBeamlineControlSetMoveAction::start(){
	if(isReady()){
		connect(this, SIGNAL(finished()), this, SLOT(onFinished()));
		for(int x = 0; x < controlSet_->count(); x++){
			connect(controlSet_->controlAt(x), SIGNAL(moveSucceeded()), this, SLOT(onSucceeded()));
			connect(controlSet_->controlAt(x), SIGNAL(moveFailed(int)), this, SLOT(onFailed(int)));
			connect(controlSet_->controlAt(x), SIGNAL(moveStarted()), this, SLOT(onStarted()));
		}
		startPoint_ = new AMControlSetInfo(controlSet_->info(), this);
		connect(&progressTimer_, SIGNAL(timeout()), this, SLOT(calculateProgress()) );
		progressTimer_.start(500);
		controlSet_->setFromInfo(setpoint_);
	}
	else
		connect(this, SIGNAL(ready(bool)), this, SLOT(delayedStart(bool)));
}

void AMBeamlineControlSetMoveAction::cancel(){
	if(controlSet_ && isRunning())
		for(int x = 0; x < controlSet_->count(); x++)
			controlSet_->controlAt(x)->stop();
}

void AMBeamlineControlSetMoveAction::setControlSet(AMControlSet *controlSet){
	if(controlSet_){
		for(int x = 0; x < controlSet_->count(); x++)
			disconnect(controlSet_->controlAt(x), 0, this, 0);
	}
	controlSet_ = controlSet;
	if(controlSet_){
		for(int x = 0; x < controlSet_->count(); x++){
			connect(controlSet_->controlAt(x), SIGNAL(movingChanged(bool)), this, SLOT(onMovingChanged(bool)));
			connect(controlSet_->controlAt(x), SIGNAL(connected(bool)), this, SLOT(onConnected(bool)));
		}
	}
	if(!controlSet_){
		if(setpoint_)
			delete setpoint_;
		setpoint_ = NULL;
	}
	else if(controlSet_ && setpoint_){
		for(int x = 0; x < controlSet_->count(); x++){
			if(controlSet_->controlAt(x)->name() != setpoint_->nameAt(x)){
				delete setpoint_;
				setpoint_ = NULL;
				break;
			}
			else if(controlSet_->controlAt(x)->valueOutOfRange(setpoint_->valueAt(x)))
				setpoint_->setValueAt(x, controlSet_->controlAt(x)->value());
		}
	}
	checkReady();
}

bool AMBeamlineControlSetMoveAction::setSetpoint(AMControlSetInfo *setpoint){
	if(!controlSet_)
		return false;
	for(int x = 0; x < controlSet_->count(); x++){
		if(controlSet_->controlAt(x)->name() != setpoint_->nameAt(x))
			return false;
		else if(controlSet_->controlAt(x)->valueOutOfRange(setpoint_->valueAt(x)))
			return false;
	}
	if(setpoint_)
		delete setpoint_;
	setpoint_ = new AMControlSetInfo(setpoint, this);
	return true;
}

void AMBeamlineControlSetMoveAction::delayedStart(bool ready){
	if(ready){
		disconnect(this, SIGNAL(ready(bool)), this, SLOT(delayedStart(bool)));
		start();
	}
}

void AMBeamlineControlSetMoveAction::onMovingChanged(bool moving){
	if(!hasStarted() && moving)
		setReady(false);
	else if(!hasStarted())
		checkReady();
	else{
		/// \todo check for finished or stopped
	}
}

void AMBeamlineControlSetMoveAction::onConnected(bool connected){
	if(!hasStarted() && !connected)
		setReady(false);
	else if(!hasStarted())
		checkReady();
}

void AMBeamlineControlSetMoveAction::checkReady(){
	if(!controlSet_ || !setpoint_)
		setReady(false);
	else{
		for(int x = 0; x < controlSet_->count(); x++)
			if(!controlSet_->controlAt(x)->isConnected() || controlSet_->controlAt(x)->isMoving())
				setReady(false);
	}
	setReady(true);
}

void AMBeamlineControlSetMoveAction::onStarted(){
	if(!hasStarted())
		setStarted(true);
}

void AMBeamlineControlSetMoveAction::onSucceeded(){
	for(int x = 0; x < controlSet_->count(); x++)
		if(controlSet_->controlAt(x)->moveInProgress())
			return;
	for(int x = 0; x < controlSet_->count(); x++)
		disconnect(controlSet_->controlAt(x), 0, this, 0);
	setSucceeded(true);
}

void AMBeamlineControlSetMoveAction::onFailed(int explanation){
	setFailed(true, explanation);
}

void AMBeamlineControlSetMoveAction::onFinished(){
	progressTimer_.stop();
	emit progress(1,1);
}

void AMBeamlineControlSetMoveAction::calculateProgress(){
	if(!controlSet_)
		return;
	double avgPercent, iPercent;
	double csCount = (double)controlSet_->count();
	qDebug() << "Count is " << csCount;
	for(int x = 0; x < controlSet_->count(); x++){
		if( fabs(setpoint_->valueAt(x) - startPoint_->valueAt(x)) < 0.0001 )
			iPercent = 100;
		else
			iPercent = (fabs(controlSet_->controlAt(x)->value()-startPoint_->valueAt(x))/fabs(setpoint_->valueAt(x) - startPoint_->valueAt(x))*100);
		avgPercent += iPercent/csCount;
		qDebug() << "i " << iPercent << " avg " << avgPercent;
//		avgPercent += (fabs(controlSet_->controlAt(x)->value()-startPoint_->valueAt(x))/fabs(setpoint_->valueAt(x) - startPoint_->valueAt(x))*100)/csCount;
	}
	qDebug() << "\n\n";
	emit progress(avgPercent, 100);
}

AMBeamlineControlSetMoveActionView::AMBeamlineControlSetMoveActionView(AMBeamlineControlSetMoveAction *controlSetAction, int index, QWidget *parent) :
		AMBeamlineActionView(controlSetAction, index, parent)
{
	controlSetAction_ = NULL;
	setAction(controlSetAction);
	viewType_ = "controlSetMoveView";

	setMinimumHeight(NATURAL_ACTION_VIEW_HEIGHT);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	infoLabel_ = new QLabel();
	infoLabel_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
	onInfoChanged();

	progressBar_ = new QProgressBar();
	progressBar_->setMinimum(0);
	timeRemainingLabel_ = new QLabel("Move Not Started");
	timeRemainingLabel_->setAlignment(Qt::AlignHCenter);
	QVBoxLayout *progressVL = new QVBoxLayout();
	progressVL->addWidget(progressBar_);
	progressVL->addWidget(timeRemainingLabel_);
	closeIcon_ = QIcon(":/window-close.png");
	stopIcon_ = QIcon(":/media-playback-stop-dark.png");
	startIcon_ = QIcon(":/media-playback-start-dark.png");
	pauseIcon_ = QIcon(":/media-playback-pause-dark.png");
	stopCancelButton_ = new QPushButton(closeIcon_, "");
	stopCancelButton_->setMaximumHeight(progressBar_->size().height());
	stopCancelButton_->setFixedWidth(25);
	stopCancelButton_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	playPauseButton_ = new QPushButton(startIcon_, "");
	playPauseButton_->setMaximumHeight(progressBar_->size().height());
	playPauseButton_->setFixedWidth(25);
	playPauseButton_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	playPauseButton_->setEnabled(false);
	connect(stopCancelButton_, SIGNAL(clicked()), this, SLOT(onStopCancelButtonClicked()));
	connect(playPauseButton_, SIGNAL(clicked()), this, SLOT(onPlayPauseButtonClicked()));
	hl_ = new QHBoxLayout();
	hl_->addWidget(infoLabel_);
	hl_->addLayout(progressVL);
	hl_->addWidget(playPauseButton_, 0, Qt::AlignTop | Qt::AlignRight);
	hl_->addWidget(stopCancelButton_, 0, Qt::AlignTop | Qt::AlignRight);
	setLayout(hl_);
}

QString AMBeamlineControlSetMoveActionView::viewType() const{
	return AMBeamlineActionView::viewType()+"."+viewType_;
}

void AMBeamlineControlSetMoveActionView::setIndex(int index){
	index_ = index;
	onInfoChanged();
}

void AMBeamlineControlSetMoveActionView::setAction(AMBeamlineControlSetMoveAction *controlSetAction){
	if(controlSetAction_){
		disconnect(controlSetAction_, SIGNAL(progress(double,double)), this, SLOT(updateProgressBar(double,double)));
		disconnect(controlSetAction_, SIGNAL(started()), this, SLOT(onStarted()));
		disconnect(controlSetAction_, SIGNAL(succeeded()), this, SLOT(onSucceeded()));
		disconnect(controlSetAction_, SIGNAL(failed(int)), this, SLOT(onFailed(int)));
	}
	controlSetAction_ = controlSetAction;
	if(controlSetAction_){
		connect(controlSetAction_, SIGNAL(progress(double,double)), this, SLOT(updateProgressBar(double,double)));
		connect(controlSetAction_, SIGNAL(started()), this, SLOT(onStarted()));
		connect(controlSetAction_, SIGNAL(succeeded()), this, SLOT(onSucceeded()));
		connect(controlSetAction_, SIGNAL(failed(int)), this, SLOT(onFailed(int)));
	}
}

void AMBeamlineControlSetMoveActionView::onInfoChanged(){
	QString infoText, tmpStr;
	if(index_ != -1){
		infoText.setNum(index_);
		infoText.append(". ");
	}
	infoText += " Moving ";
	QString adjName = controlSetAction_->controlSet()->name();
	adjName.replace(QRegExp("([A-Z])"), " \\1");
	QChar fCap = adjName[0].toUpper();
	adjName.replace(0, 1, fCap);
	infoText += adjName;
//	infoText += " to ";
//	tmpStr.setNum( controlSetAction_->setpoint() );
//	infoText.append(tmpStr);
	infoLabel_->setText(infoText);
}

void AMBeamlineControlSetMoveActionView::updateProgressBar(double elapsed, double total){
	if(elapsed == total){
		progressBar_->setMaximum(100);
		progressBar_->setValue(100);
		timeRemainingLabel_->setText("Move Complete");
		return;
	}
	if(controlSetAction_->hasFailed())
		return;
	progressBar_->setMaximum((int)total);
	progressBar_->setValue((int)elapsed);

	double secondsRemaining = total - elapsed;
	QTime tRemaining = QTime(0,0,0,0).addMSecs((int)1000*secondsRemaining);
	QString rStr = (tRemaining.hour() > 0) ? "h:mm:ss" : "m:ss" ;
	timeRemainingLabel_->setText(tRemaining.toString(rStr)+" Remaining");
}

void AMBeamlineControlSetMoveActionView::onStopCancelButtonClicked(){
	emit removeRequested(controlSetAction_);
}

void AMBeamlineControlSetMoveActionView::onPlayPauseButtonClicked(){

}

void AMBeamlineControlSetMoveActionView::onStarted(){
	stopCancelButton_->setIcon(stopIcon_);
	playPauseButton_->setIcon(pauseIcon_);
	playPauseButton_->setEnabled(true);
//	updateLook();
	emit actionStarted(controlSetAction_);
}

void AMBeamlineControlSetMoveActionView::onSucceeded(){
	qDebug() << "In control set move action (view) succeeded";
	progressBar_->setValue(progressBar_->maximum());

	progressBar_->setMaximum(100);
	progressBar_->setValue(100);
	timeRemainingLabel_->setText("Move Complete");
	disconnect(stopCancelButton_, SIGNAL(clicked()), this, SLOT(onStopCancelButtonClicked()));
	disconnect(playPauseButton_, SIGNAL(clicked()), this, SLOT(onPlayPauseButtonClicked()));
	hl_->removeWidget(stopCancelButton_);
	stopCancelButton_->hide();
	//delete hl_->takeAt(hl_->indexOf(stopCancelButton_));
	hl_->removeWidget(playPauseButton_);
	playPauseButton_->hide();
	//delete hl_->takeAt(hl_->indexOf(playPauseButton_));
	emit actionSucceeded(controlSetAction_);
}

void AMBeamlineControlSetMoveActionView::onFailed(int explanation){
	stopCancelButton_->setIcon(closeIcon_);
	playPauseButton_->setIcon(startIcon_);
	playPauseButton_->setEnabled(false);
	timeRemainingLabel_->setText("Move Cancelled");
}
/*
Copyright 2010, 2011 Mark Boots, David Chevrier.

This file is part of the Acquaman Data Acquisition and Management framework ("Acquaman").

Acquaman is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Acquaman is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Acquaman.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDebug>

#include "AMScanConfigurationViewHolder.h"
#include "acquaman/AMScanConfiguration.h"
#include "ui/AMScanConfigurationView.h"
#include "ui/AMWorkflowManagerView.h"

AMScanConfigurationHolder::AMScanConfigurationHolder(AMWorkflowManagerView* workflow, AMScanConfigurationView* view, QWidget *parent) :
		QWidget(parent)
{
	view_ = view;
	workflow_ = workflow;

	startScanButton_ = new QPushButton("Start Scan");
	addToQueueButton_ = new QPushButton("Add to Workflow");

	goToWorkflowOption_ = new QRadioButton("Show me the workflow");
	goToWorkflowOption_->setChecked(true);
	setupAnotherScanOption_ = new QRadioButton("Setup another scan");


	//	statusLabel_ = new QLabel("Ready To Start Scan");

	layout_ = new QVBoxLayout();
	if(view_)
		layout_->addWidget(view_);

	QHBoxLayout* hl = new QHBoxLayout();

	//	hl->addWidget(statusLabel_);
	//	hl->addStretch();
	hl->addWidget(new QLabel("When I'm done here:"));
	hl->addWidget(goToWorkflowOption_);
	hl->addWidget(setupAnotherScanOption_);
	hl->addStretch();
	hl->addWidget(addToQueueButton_);
	hl->addWidget(startScanButton_);

	layout_->addLayout(hl);

	setLayout(layout_);

	connect(startScanButton_, SIGNAL(clicked()), this, SLOT(onStartScanRequested()));
	connect(addToQueueButton_, SIGNAL(clicked()), this, SLOT(onAddToQueueRequested()));

	connect(workflow_, SIGNAL(workflowStatusChanged(bool,bool,bool)), this, SLOT(reviewStartScanButtonState()));

	reviewStartScanButtonState();
}



AMScanConfigurationHolder::~AMScanConfigurationHolder()
{
}

void AMScanConfigurationHolder::setView(AMScanConfigurationView *view) {
	// delete old view, if it exists
	if(view_)
		delete view_;

	view_ = view;
	if(view_) {
		layout_->insertWidget(0, view_);
	}

	reviewStartScanButtonState();
}



void AMScanConfigurationHolder::reviewStartScanButtonState() {

	// if the scan configuration view, or its actual configuration, is not valid...
	if(!view_ || !view_->configuration()) {
		startScanButton_->setEnabled(false);
		addToQueueButton_->setEnabled(false);
		startScanButton_->setText("No valid Scan Configuration");
	}

	// if we can't start the workflow because it's already running (ie: our scans / actions)
	else if(workflow_->isRunning()) {
		startScanButton_->setEnabled(false);
		addToQueueButton_->setEnabled(false);
		startScanButton_->setText("Scans in progress");
	}
	// if we can't start the workflow because the beamline is busy/locked out (ie: another program is using it)
	else if(workflow_->beamlineBusy()) {
		startScanButton_->setEnabled(false);
		addToQueueButton_->setEnabled(false);
		startScanButton_->setText("Beamline Busy");
	}
	// Good to go. The workflow isn't running, and the beamline is not busy. Watch out: there may or may not still be scan actions already in the queue.
	else {
		startScanButton_->setEnabled(true);
		addToQueueButton_->setEnabled(true);
		startScanButton_->setText("Start Scan");
	}
}


#include <QMessageBox>
void AMScanConfigurationHolder::onStartScanRequested(){

	if(!view_ || !view_->configuration())
		return;

	if(workflow_->isRunning() || workflow_->beamlineBusy())
		return;

	bool startNow = true;
	int position = 0;

	// check first: if there's already items in the workflow, we need to find out if they want to add this action to the end of the queue, add this action to the beginning of the queue,
	if(workflow_->actionItemCount()) {

		qDebug() << workflow_->actionItemCount();

		QMessageBox questionBox;
		questionBox.setText("There are already scans waiting in the workflow queue.");
		questionBox.setInformativeText("What do you want to do with this scan?");
		questionBox.setIcon(QMessageBox::Question);
		QPushButton* cancel = questionBox.addButton("Cancel", QMessageBox::RejectRole);
		QPushButton* addToEnd = questionBox.addButton("Add to end", QMessageBox::YesRole);
		QPushButton* addToEndAndStart = questionBox.addButton("Add to end, and start workflow", QMessageBox::YesRole);
		QPushButton* addToBeginningAndStart = questionBox.addButton("Add to beginning and start", QMessageBox::YesRole);
		questionBox.setDefaultButton(addToEndAndStart);

		questionBox.exec();

		QAbstractButton* result = questionBox.clickedButton();
		if(result == cancel) {
			return;
		}
		else if(result == addToEnd) {
			position = -1;
			startNow = false;
		}
		else if(result == addToEndAndStart) {
			position = -1;
			startNow = false;
		}
		else if(result == addToBeginningAndStart) {
			position = 0;
			startNow = true;
		}
	}

	AMBeamlineScanAction* action = new AMBeamlineScanAction(view_->configuration()->createCopy());
	workflow_->insertBeamlineAction(position, action, startNow);
}

void AMScanConfigurationHolder::onAddToQueueRequested() {

	if(!view_ || !view_->configuration())
		return;

	AMBeamlineScanAction* action = new AMBeamlineScanAction(view_->configuration()->createCopy());
	workflow_->insertBeamlineAction(-1, action);

	if(goToWorkflowOption_->isChecked())
		emit showWorkflowRequested();
}




// From old version: AMFastScanConfigurationHolder:

//AMFastScanConfigurationHolder::AMFastScanConfigurationHolder(QWidget *parent) :
//		AMScanConfigurationHolder(parent)
//{
//	vl_ = NULL;
//	cfg_ = NULL;
//	sfscViewer_ = NULL;

//	autoSavePath_ = "";
//	lastSettings_ = 0; //NULL

//	requestedStart_ = false;
//	canStartImmediately_ = false;
//	director = new AMScanConfigurationQueueDirector();
//	director->setWindowModality(Qt::ApplicationModal);
//	sDirector = new AMScanConfigurationScanDirector();
//	sDirector->setWindowModality(Qt::ApplicationModal);

//	connect(director, SIGNAL(goToQueue()), this, SLOT(goToQueue()));
//	connect(director, SIGNAL(goToNewScan()), this, SLOT(goToNewScan()));
//}

//AMFastScanConfigurationHolder::~AMFastScanConfigurationHolder()
//{
//}

//void AMFastScanConfigurationHolder::onBecameCurrentWidget()
//{
//	if(!sfscViewer_ && isVisible() && SGMBeamline::sgm()->isConnected()){
//		createScanConfiguration();
//		sfscViewer_ = new SGMFastScanConfigurationViewer(cfg());
//		connect(sfscViewer_, SIGNAL(startScanRequested()), this, SLOT(onStartScanRequested()));
//		connect(sfscViewer_, SIGNAL(addToQueueRequested()), this, SLOT(onAddToQueueRequested()));
//		connect(sfscViewer_, SIGNAL(queueDirectorRequested()), director, SLOT(show()));
//		connect(sfscViewer_, SIGNAL(lastSettings(SGMFastScanParameters*)), this, SLOT(setLastSettings(SGMFastScanParameters*)));
//		connect(this, SIGNAL(lockdownScanning(bool,QString)), sfscViewer_, SLOT(onLockdowScanning(bool,QString)));

//		if(!vl_)
//			vl_ = new QVBoxLayout();
//		vl_->addWidget(sfscViewer_);
//		if(layout() != vl_){
//			delete layout();
//			this->setLayout(vl_);
//		}
//		emit newScanConfigurationView();
//	}
//}

//void AMFastScanConfigurationHolder::createScanConfiguration(){
//	cfg_ = new SGMFastScanConfiguration(this);
//	cfg_->setFileName("daveData.%03d.dat");
//	cfg_->setFilePath(AMUserSettings::userDataFolder);
//	if(!autoSavePath_.isEmpty())
//		cfg()->setSensibleFileSavePath(autoSavePath_);
//	connect(cfg(), SIGNAL(onSensibleFileSavePathChanged(QString)), this, SLOT(setAutoSavePath(QString)));
//	if(lastSettings_)
//		cfg()->setParameters(lastSettings_);


//}

//void AMFastScanConfigurationHolder::destroyScanConfigurationViewer(){
//	qDebug() << "Trying to destroy fast scan viewer";
//	if(sfscViewer_){
//		disconnect(sfscViewer_, SIGNAL(startScanRequested()), this, SLOT(onStartScanRequested()));
//		disconnect(sfscViewer_, SIGNAL(addToQueueRequested()), this, SLOT(onAddToQueueRequested()));
//		disconnect(sfscViewer_, SIGNAL(queueDirectorRequested()), director, SLOT(show()));
//		disconnect(sfscViewer_, SIGNAL(lastSettings(SGMFastScanParameters*)), this, SLOT(setLastSettings(SGMFastScanParameters*)));
//		disconnect(this, SIGNAL(lockdownScanning(bool,QString)), sfscViewer_, SLOT(onLockdowScanning(bool,QString)));
//		vl_->removeWidget(sfscViewer_);
//		delete sfscViewer_;
//		sfscViewer_ = NULL;
//	}
//}

//void AMFastScanConfigurationHolder::setAutoSavePath(const QString &autoSavePath){
//	autoSavePath_ = autoSavePath;
//}

//void AMFastScanConfigurationHolder::setLastSettings(SGMFastScanParameters *lastSettings){
//	lastSettings_ = lastSettings;
//}


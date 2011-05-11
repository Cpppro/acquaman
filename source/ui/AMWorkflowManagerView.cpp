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


#include "AMWorkflowManagerView.h"
#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMenu>

#include "dataman/AMSamplePlate.h"
#include "ui/AMSamplePlateView.h"

#include "acquaman/AMScanConfiguration.h"
#include "beamline/AMBeamlineScanAction.h"
//#include "beamline/AMBeamlineControlSetMoveAction.h"
#include "beamline/AMBeamlineSamplePlateMoveAction.h"
#include "beamline/AMBeamlineFiducializationMoveAction.h"

#include "beamline/AMBeamline.h"
#include "ui/AMVerticalStackWidget.h"
#include "ui/AMTopFrame.h"


AMWorkflowManagerView::AMWorkflowManagerView(QWidget *parent) :
	QWidget(parent)
{

	currentSamplePlate_ = 0;//NULL
	samplePlateModel_ = 0;//NULL
	addActionMenu_ = 0;//NULL
	samplePlateAddActionMenu_ = 0;//NULL
	fiducializationMarkAddActionMenu_ = 0;//NULL
	samplePlateHoverIndex_ = 0;
	fiducializationMarkHoverIndex_ = 0;

	topFrame_ = new AMTopFrame("Workflow");
	topFrame_->setIcon(QIcon(":/user-away.png"));

	startWorkflowButton_ = new QPushButton("Start This Workflow\nReady");
	startWorkflowButton_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	addActionButton_ = new QPushButton("Add an Action");
	addActionButton_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	QHBoxLayout *hl = new QHBoxLayout();
	hl->addItem(new QSpacerItem(10, 10, QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
	hl->addWidget(addActionButton_, 0, Qt::AlignRight);
	hl->addWidget(startWorkflowButton_, 0, Qt::AlignRight);
	connect(startWorkflowButton_, SIGNAL(clicked()), this, SLOT(startQueue()));
	connect(addActionButton_, SIGNAL(clicked()), this, SLOT(onAddActionButtonClicked()));
	workflowActions_ = new AMBeamlineActionsList(this);
	workflowQueue_ = new AMBeamlineActionsQueue(workflowActions_, this);
	workflowView_ = new AMBeamlineActionsListView(workflowActions_, workflowQueue_, this);
	connect(workflowView_, SIGNAL(copyRequested(AMBeamlineActionItem*)), this, SLOT(onCopyActionRequested(AMBeamlineActionItem*)));
	connect(workflowView_, SIGNAL(moveUpRequested(AMBeamlineActionItem*)), this, SLOT(onMoveUpActionRequested(AMBeamlineActionItem*)));
	connect(workflowView_, SIGNAL(moveDownRequested(AMBeamlineActionItem*)), this, SLOT(onMoveDownActionRequested(AMBeamlineActionItem*)));

	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setWidget(workflowView_);
	scrollArea->setWidgetResizable(true);

	connect(AMBeamline::bl(), SIGNAL(beamlineScanningChanged(bool)), this, SLOT(reviewWorkflowStatus()));
	connect(workflowQueue_, SIGNAL(isRunningChanged(bool)), this, SLOT(reviewWorkflowStatus()));
	connect(workflowQueue_, SIGNAL(isEmptyChanged(bool)), this, SLOT(reviewWorkflowStatus()));

	//	removed with adder: connect(workflowView_, SIGNAL(queueUpdated(int)), adder_, SLOT(onQueueUpdated(int)));

	vl_ = new QVBoxLayout();
	vl_->addWidget(scrollArea);

	topFrame_->frameLayout()->addLayout(hl);

	QVBoxLayout *vl2 = new QVBoxLayout();
	vl2->addWidget(topFrame_);
	vl2->addLayout(vl_);
	vl2->setContentsMargins(0,0,0,0);
	vl2->setSpacing(1);

	vl_->setContentsMargins(10, 5, 10, 10);
	vl_->setSpacing(5);
	hl->setContentsMargins(0, 2, 10, 2);
	hl->setSpacing(5);

	setLayout(vl2);
}

void AMWorkflowManagerView::startQueue(){
	if(!workflowQueue_->isEmpty()){
//		qDebug() << "Trying to start queue";
		if(!workflowQueue_->head()->hasFinished() ){
//			qDebug() << "Initialized, so just start, no reset";
			workflowQueue_->startQueue();
		}
		else{
			qDebug() << "Not initialized anymore, so need to reset";
			workflowQueue_->head()->reset();
			workflowQueue_->startQueue();
		}
	}
}

#include <QMessageBox>
void AMWorkflowManagerView::onAddActionRequested(){
	// disabled for now...
//	adder_->move(addActionButton_->pos());
//	adder_->show();

	QMessageBox::information(this, "Sorry, This Action Not Available Yet", "Sorry, we haven't implemented this yet.\n\nWe're working on it...", QMessageBox::Ok);
}

void AMWorkflowManagerView::onCopyActionRequested(AMBeamlineActionItem *action){
	qDebug() << "Copy requested!";
	insertBeamlineAction(-1, action);
}

void AMWorkflowManagerView::onMoveUpActionRequested(AMBeamlineActionItem *action){
	qDebug() << "Move up requested by " << workflowActions_->indexOf(action) << " so swap on " << workflowActions_->indexOf(action)-1;
	int indexOfAction = workflowActions_->indexOf(action);
	workflowActions_->swapActions(indexOfAction-1);
	workflowView_->swap(indexOfAction-1);
}

void AMWorkflowManagerView::onMoveDownActionRequested(AMBeamlineActionItem *action){
	qDebug() << "Move down requested by " << workflowActions_->indexOf(action) << " so swap on " << workflowActions_->indexOf(action);
	int indexOfAction = workflowActions_->indexOf(action);
	workflowActions_->swapActions(indexOfAction);
	workflowView_->swap(indexOfAction);
}

void AMWorkflowManagerView::setCurrentSamplePlate(AMSamplePlate *newSamplePlate){
	if(!currentSamplePlate_){
		qDebug() << "From NULL plate to real plate";
		currentSamplePlate_ = newSamplePlate;
		samplePlateModel_ = new AMSamplePlateItemModel(currentSamplePlate_, this);
	}
	if(!newSamplePlate){
		qDebug() << "Set as NULL plate, delete model";
		currentSamplePlate_ = 0;
		delete samplePlateModel_;
	}
	if(currentSamplePlate_ != newSamplePlate){
		currentSamplePlate_ = newSamplePlate;
		qDebug() << "Workflow heard it has a new sample plate";
	}
}


void AMWorkflowManagerView::insertBeamlineAction(int index, AMBeamlineActionItem *action, bool startNow) {
	if(index < 0 || index > workflowQueue_->count()) {
		index = workflowQueue_->count();
	}

	if(workflowQueue_->isEmpty())
		workflowActions_->appendAction(action);
	else
		workflowActions_->addAction(workflowQueue_->indexOfHead()+index, action);

	if(startNow)
		startQueue();

}


void AMWorkflowManagerView::reviewWorkflowStatus(){
	bool qEmpty = workflowQueue_->isEmpty();
	bool qRunning = workflowQueue_->isRunning();
	bool blScanning = AMBeamline::bl()->isBeamlineScanning();

	/// \todo Move this code out of the UI and into the workflow proper.
	if(qEmpty || qRunning || blScanning)
		startWorkflowButton_->setEnabled(false);
	else{
		startWorkflowButton_->setEnabled(true);
		startWorkflowButton_->setText("Start This Workflow\n-- Ready --");
	}

	if(blScanning && !qRunning){
			startWorkflowButton_->setText("Start This Workflow\n-- Beamline Busy --");
	}
	else if(qEmpty){
		startWorkflowButton_->setText("Start This Workflow\n-- No Items --");
	}
	else if(qRunning){
		startWorkflowButton_->setText("Start This Workflow\n-- Already Running --");
	}
	/////////////////////////

	emit workflowStatusChanged(blScanning, qEmpty, qRunning);
	/// \todo Emit the following only when they change:
	emit actionItemCountChanged(workflowQueue_->count());
	emit runningChanged(qRunning);
	// emit beamlineBusyChanged(blScanning);
}

void AMWorkflowManagerView::onAddActionButtonClicked(){
	if(addActionMenu_)
		delete addActionMenu_;
	addActionMenu_ = new QMenu(this);
	samplePlateAddActionMenu_ = new QMenu();
	samplePlateAddActionMenu_->setTitle("Sample Plate");
	QAction *tmpAction;
	if(samplePlateModel_ && samplePlateModel_->rowCount(QModelIndex()) == 0)
		samplePlateAddActionMenu_->addAction("<No Samples On Plate>");
	else if(samplePlateModel_){
		for(int x = 0; x < samplePlateModel_->rowCount(QModelIndex()); x++){
			tmpAction = samplePlateAddActionMenu_->addAction(samplePlateModel_->data(samplePlateModel_->index(x), Qt::EditRole).toString());
			tmpAction->setData(x);
			connect(tmpAction, SIGNAL(hovered()), this, SLOT(setSampleHoverIndex()));
			connect(tmpAction, SIGNAL(triggered()), this, SLOT(onSamplePlateAddActionClicked()));
		}
	}
	else
		samplePlateAddActionMenu_->addAction("<No Sample Plate Selected>");
	addActionMenu_->addMenu(samplePlateAddActionMenu_);

	QList<AMControlInfoList> fiducializations = AMBeamline::bl()->currentFiducializations();
	if(fiducializations.count() > 0){
		fiducializationMarkAddActionMenu_ = new QMenu();
		fiducializationMarkAddActionMenu_->setTitle("Fiducialization Marks");
		for(int x = 0; x < fiducializations.count(); x++){
			tmpAction = fiducializationMarkAddActionMenu_->addAction(QString("Location %1").arg(x+1));
			tmpAction->setData(x);
			connect(tmpAction, SIGNAL(hovered()), this, SLOT(setFiducializationHoverIndex()));
			connect(tmpAction, SIGNAL(triggered()), this, SLOT(onFiducializationMarkAddActionClicked()));
		}
		addActionMenu_->addMenu(fiducializationMarkAddActionMenu_);
	}
	addActionMenu_->popup(QCursor::pos());
	addActionMenu_->show();
}

void AMWorkflowManagerView::setSampleHoverIndex(){
	samplePlateHoverIndex_ = samplePlateAddActionMenu_->activeAction()->data().toInt();
}

void AMWorkflowManagerView::setFiducializationHoverIndex(){
	fiducializationMarkHoverIndex_ = fiducializationMarkAddActionMenu_->activeAction()->data().toInt();
}

void AMWorkflowManagerView::onSamplePlateAddActionClicked(){
	AMBeamlineSamplePlateMoveAction *sampleMoveAction = new AMBeamlineSamplePlateMoveAction(currentSamplePlate_->at(samplePlateHoverIndex_).sampleId(), samplePlateModel_);
	sampleMoveAction->setSetpoint(currentSamplePlate_->at(samplePlateHoverIndex_).position());
	insertBeamlineAction(-1, sampleMoveAction);
}

void AMWorkflowManagerView::onFiducializationMarkAddActionClicked(){ qDebug() << "Fiducialization triggered is " << fiducializationMarkHoverIndex_;
	AMBeamlineFiducializationMoveAction *fiducializationMoveAction = new AMBeamlineFiducializationMoveAction(fiducializationMarkHoverIndex_);
	insertBeamlineAction(-1, fiducializationMoveAction);
}

bool AMWorkflowManagerView::beamlineBusy() const {
	return AMBeamline::bl()->isBeamlineScanning();
}

AMBeamlineActionsListView::AMBeamlineActionsListView(AMBeamlineActionsList *actionsList, AMBeamlineActionsQueue *actionsQueue, QWidget *parent) :
		QWidget(parent)
{
	actionsList_ = actionsList;
	actionsQueue_ = actionsQueue;
	focusAction_ = -1;
	needsNewGroup_ = true;

	actionsViewList_ = new AMVerticalStackWidget(this, true);
	actionsViewList_->setGroupings(groupings_);
	QVBoxLayout *vl = new QVBoxLayout();
	vl->addWidget(actionsViewList_);
	setLayout(vl);

	connect(actionsList_, SIGNAL(actionChanged(int)), this, SLOT(onActionChanged(int)));
	connect(actionsList_, SIGNAL(actionAdded(int)), this, SLOT(onActionAdded(int)));
	connect(actionsList_, SIGNAL(actionRemoved(int)), this, SLOT(onActionRemoved(int)));
	connect(actionsQueue_, SIGNAL(headChanged()), this, SLOT(reindexViews()));
	connect(actionsQueue_, SIGNAL(isRunningChanged(bool)), this, SLOT(onRunningChanged()));
	connect(actionsList_, SIGNAL(actionStarted(int)), this, SLOT(onActionStarted()));
	connect(actionsList_, SIGNAL(actionSucceeded(int)), this, SLOT(onActionSucceeded()));
	connect(actionsList_, SIGNAL(actionFailed(int,int)), this, SLOT(onActionFailed()));
}

bool AMBeamlineActionsListView::swap(int indexOfFirst){
	bool retVal = actionsViewList_->swapItem(indexOfFirst);
	if(retVal)
		reindexViews();
	return retVal;
}

#include "beamline/AMBeamlineControlMoveAction.h"
#include "beamline/AMBeamlineControlSetMoveAction.h"

/// \bug What happens if the action at \c index has changed type? This assumes the subclass of view is correct for the subclass of actionItem
/*NTBA April 21, 2011 David Chevrier
	If the action type changed, then the view type needs to change too
 */
void AMBeamlineActionsListView::onActionChanged(int index){
	AMBeamlineActionItem *tmpItem = actionsList_->action(index);
	((AMBeamlineActionItemView*)actionsViewList_->widget(index))->setAction(tmpItem);
}

#include "ui/AMDateTimeUtils.h"
void AMBeamlineActionsListView::onActionAdded(int index){
	AMBeamlineActionItem *tmpItem = actionsList_->action(index);
	AMBeamlineActionItemView *tmpView = tmpItem->createView(index);
	if(!tmpView)
		return;
	actionsViewList_->insertItem(index, tmpItem->description(), tmpView, true);
	connect(tmpView, SIGNAL(removeRequested(AMBeamlineActionItem*)), this, SLOT(onActionRemoveRequested(AMBeamlineActionItem*)));
	connect(tmpView, SIGNAL(copyRequested(AMBeamlineActionItem*)), this, SIGNAL(copyRequested(AMBeamlineActionItem*)));
	connect(tmpView, SIGNAL(moveUpRequested(AMBeamlineActionItem*)), this, SIGNAL(moveUpRequested(AMBeamlineActionItem*)));
	connect(tmpView, SIGNAL(moveDownRequested(AMBeamlineActionItem*)), this, SIGNAL(moveDownRequested(AMBeamlineActionItem*)));
	if(!needsNewGroup_){
		qDebug() << "Add to existing group";
		//groupings_.last().first++;
		groupings_.last().setActionCount(groupings_.last().actionCount()+1);
		actionsViewList_->setGroupings(groupings_);
	}
	else{
		needsNewGroup_ = false;
		qDebug() << "Create a new group";
		//QPair<int, QString> newGroup = QPair<int, QString>(1, QString("Created %1").arg(AMDateTimeUtils::prettyDateTime(QDateTime::currentDateTime())) );
		AMRunGroup newGroup = AMRunGroup(1, "Created");
		groupings_.append(newGroup);
		actionsViewList_->setGroupings(groupings_);
	}

	reindexViews();
	emit queueUpdated(actionsQueue_->count());
}

void AMBeamlineActionsListView::onActionRemoved(int index){
	/*
	if(groupings_.last().first > 1)
		groupings_.last().first--;
	else
		groupings_.removeLast();
	*/
	if(groupings_.last().actionCount() > 1)
		groupings_.last().setActionCount(groupings_.last().actionCount()-1);
	else
		groupings_.removeLast();
	actionsViewList_->setGroupings(groupings_);
	actionsViewList_->deleteItem(index);
	actionsViewList_->forceGroupingsCheck();

	reindexViews();
	emit queueUpdated(actionsQueue_->count());
}

void AMBeamlineActionsListView::onActionRemoveRequested(AMBeamlineActionItem *item){
	int index = actionsList_->indexOf(item);
	actionsList_->deleteAction(index);
}

void AMBeamlineActionsListView::onRunningChanged(){
	if(actionsQueue_->isRunning()){
		//groupings_.insert(groupings_.count()-1, QPair<int, QString>(0, "Running"));
		groupings_.insert(groupings_.count()-1, AMRunGroup(0, "Running"));
		actionsViewList_->startRunning();
	}
	qDebug() << "---- Heard that running state changed to " << actionsQueue_->isRunning() << " ----";
}

void AMBeamlineActionsListView::onActionStarted(){
	/*
	groupings_[groupings_.count()-2].first++;
	groupings_.last().first--;
	*/
	groupings_[groupings_.count()-2].setActionCount(groupings_.at(groupings_.count()-2).actionCount()+1);
	groupings_.last().setActionCount(groupings_.last().actionCount()-1);
	actionsViewList_->setGroupings(groupings_);
	if(actionsQueue_->peekIsEmpty()){
		qDebug() << "Found queue is empty";
		actionsViewList_->endRunning();
		needsNewGroup_ = true;
		groupings_.removeLast();
		actionsViewList_->setGroupings(groupings_);
		actionsViewList_->forceGroupingsCheck();
	}
	actionsViewList_->forceGroupingsCheck();
	qDebug() << "---- Heared action STARTED ----";
}

void AMBeamlineActionsListView::onActionSucceeded(){
	qDebug() << "---- Heard action SUCCEEDED ----";
	if(actionsQueue_->isEmpty()){
		//groupings_.last().second = QString("Completed %1").arg(AMDateTimeUtils::prettyDateTime(QDateTime::currentDateTime()));
		groupings_.last().setDisplayText("Completed");
		groupings_.last().setEventTime(QDateTime::currentDateTime());
		actionsViewList_->setGroupings(groupings_);
		actionsViewList_->forceGroupingsCheck();
	}
}

void AMBeamlineActionsListView::onActionFailed(){
	qDebug() << "---- Heard action FAILED";
}

void AMBeamlineActionsListView::reindexViews(){
	if(actionsQueue_->indexOfHead() != -1){
		QString lastSampleDescription = AMBeamline::bl()->currentSampleDescription();
		for(int x = 0; x < actionsQueue_->indexOfHead(); x++)
			if(actionsViewList_->widget(x) && ((AMBeamlineActionItemView*)(actionsViewList_->widget(x)))->index() != -1 )
				((AMBeamlineActionItemView*)(actionsViewList_->widget(x)))->setIndex(-1);
		for(int x = actionsQueue_->indexOfHead(); x < actionsList_->count(); x++){
			if(actionsViewList_->widget(x) && ((AMBeamlineActionItemView*)(actionsViewList_->widget(x)))->index() != x-actionsQueue_->indexOfHead()+1 )
				((AMBeamlineActionItemView*)(actionsViewList_->widget(x)))->setIndex(x-actionsQueue_->indexOfHead()+1);
			AMBeamlineScanAction *scanAction = qobject_cast<AMBeamlineScanAction*>(actionsList_->action(x));
			if(scanAction)
				scanAction->setLastSampleDescription(lastSampleDescription);
			AMBeamlineSamplePlateMoveAction *sampleAction = qobject_cast<AMBeamlineSamplePlateMoveAction*>(actionsList_->action(x));
			if(sampleAction)
				lastSampleDescription = sampleAction->sampleDescription();
			AMBeamlineFiducializationMoveAction *fiducializationAction = qobject_cast<AMBeamlineFiducializationMoveAction*>(actionsList_->action(x));
			if(fiducializationAction)
				lastSampleDescription = fiducializationAction->sampleDescription();
		}
	}
	else
		for(int x = 0; x < actionsList_->count(); x++)
			if(actionsViewList_->widget(x) && ((AMBeamlineActionItemView*)(actionsViewList_->widget(x)))->index() != -1 )
				((AMBeamlineActionItemView*)(actionsViewList_->widget(x)))->setIndex(-1);
}

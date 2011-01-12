#include "AMActionRunner.h"
#include "actions2/AMAction.h"

#include <QStringBuilder>

AMActionRunner* AMActionRunner::instance_ = 0;

// testing only:
#include "actions2/actions/AMWaitAction.h"

AMActionRunner::AMActionRunner(QObject *parent) :
	QObject(parent)
{
	currentAction_ = 0;
	isPaused_ = true;

	addActionToQueue(new AMWaitAction(20));
	addActionToQueue(new AMWaitAction(20));
	addActionToQueue(new AMWaitAction(20));
	addActionToQueue(new AMWaitAction(20));
}

AMActionRunner::~AMActionRunner() {
	while(queuedActionCount() != 0) {
		deleteActionInQueue(queuedActionCount()-1);
	}
}

AMActionRunner * AMActionRunner::s()
{
	if(!instance_)
		instance_ = new AMActionRunner();
	return instance_;
}

void AMActionRunner::releaseActionRunner()
{
	delete instance_;
	instance_ = 0;
}

void AMActionRunner::onCurrentActionStateChanged(int state, int previousState)
{
	Q_UNUSED(previousState)

	if(state == AMAction::Failed) {
		// What should we do?
		int failureResponse = currentAction_->failureResponseInActionRunner();
		if(failureResponse == AMAction::PromptUserResponse)
			failureResponse = internalAskUserWhatToDoAboutFailedAction(currentAction_);

		if(failureResponse == AMAction::AttemptAnotherCopyResponse) {
			// make a fresh copy of this action and put it at the front of the queue to run again.
			insertActionInQueue(currentAction_->createCopy(), 0);
		}
		// other failure response is to MoveOn, which we'll do anyway.
	}

	if(state == AMAction::Cancelled) {
		// Usability guess for now: If the user intervened to cancel, they probably want to make some changes to something. Let's pause the workflow for now to let them do that, rather than automatically go on to the next.
		setQueuePaused(true);
	}

	// for all three final states, this is how we wrap things up for the current action:
	if(state == AMAction::Failed ||
			state == AMAction::Cancelled ||
			state == AMAction::Succeeded) {

		// log it:
		// todozzzz AMActionHistoryLogger::s()->logCompletedAction(currentAction_);

		// move onto the next, if there is one, and disconnect and delete the old one.
		internalDoNextAction();
	}
}

void AMActionRunner::insertActionInQueue(AMAction *action, int index)
{
	if(index < 0 || index > queueModel_.rowCount())
		index = queueModel_.rowCount();

	// emit actionAboutToBeAdded(index);
	queueModel_.insertRow(index, new AMActionModelItem(action));
	// emit actionAdded(index);

	// was this the first action inserted into a running but empty queue? Start it up!
	if(!isPaused_ && !actionRunning())
		internalDoNextAction();
}

bool AMActionRunner::deleteActionInQueue(int index)
{
	if(index<0 || index>=queueModel_.rowCount())
		return false;

	// emit actionAboutToBeRemoved(index);
	AMAction* action = static_cast<AMActionModelItem*>(queueModel_.item(index))->action();
	queueModel_.removeRow(index);
	delete action;
	// emit actionRemoved(index);

	return true;
}

bool AMActionRunner::duplicateActionInQueue(int index)
{
	if(index<0 || index>=queueModel_.rowCount())
		return false;

	insertActionInQueue(static_cast<AMActionModelItem*>(queueModel_.item(index))->action()->createCopy(), index+1);
	return true;
}

bool AMActionRunner::duplicateActionsInQueue(const QList<int> &indexesToCopy)
{
	if(indexesToCopy.isEmpty())
		return false;

	// find max index, and return false if any are out of range.
	int maxIndex = indexesToCopy.at(0);
	if(maxIndex < 0 || maxIndex >= queuedActionCount())
		return false;

	for(int i=1, cc=indexesToCopy.count(); i<cc; i++) {
		int index = indexesToCopy.at(i);
		if(index < 0 || index >= queuedActionCount())
			return false;
		if(index > maxIndex)
			maxIndex = index;
	}

	// note: because we're inserting at higher indexes than any of indexesToCopy, we know that any that we're supposed to copy won't change positions as we start inserting.
	foreach(int index, indexesToCopy) {
		insertActionInQueue(static_cast<AMActionModelItem*>(queueModel_.item(index))->action()->createCopy(),
							++maxIndex);
	}
	return true;
}

bool AMActionRunner::moveActionInQueue(int currentIndex, int finalIndex)
{
	if(currentIndex == finalIndex)
		return false;

	if(currentIndex < 0 || currentIndex >= queuedActionCount())
		return false;

	if(finalIndex <0 || finalIndex >= queuedActionCount())
		finalIndex = queuedActionCount()-1;

	// emit actionAboutToBeRemoved(currentIndex);
	QStandardItem* item = queueModel_.item(currentIndex);
	queueModel_.removeRow(currentIndex);
	// emit actionRemoved(currentIndex);

	// emit actionAboutToBeAdded(finalIndex);
	queueModel_.insertRow(finalIndex, item);
	// emit actionAdded(finalIndex);

	return true;
}

void AMActionRunner::setQueuePaused(bool isPaused) {
	if(isPaused == isPaused_)
		return;

	emit queuePausedChanged(isPaused_ = isPaused);

	// if we've gone from paused to running, and there is no action running right now, and we have actions we can run... Start the next.
	if(!isPaused_ && queuedActionCount() != 0 && !actionRunning())
		internalDoNextAction();
}

void AMActionRunner::internalDoNextAction()
{
	qDebug() << "AMActionRunner: Next Action";

	// signal that no action is running? (queue paused, or we're out of actions in the queue to run...)
	if(isPaused_ || queuedActionCount() == 0) {
		if(currentAction_) {
			AMAction* oldAction = currentAction_;
			emit currentActionChanged(currentAction_ = 0);
			delete oldAction;
		}
	}

	// Otherwise, keep on keepin' on. Disconnect and delete the old current action (if there is one... If we're resuming from pause or starting for the first time, there won't be)., and connect and start the next one.
	else {
		AMAction* oldAction = currentAction_;

		// todozzzz Handle special situation if newAction is a LoopAction: don't remove it.
		// emit actionAboutToBeRemoved(0);
		AMAction* newAction = queuedActionAt(0);
		queueModel_.removeRow(0);
		// emit actionRemoved(0);

		emit currentActionChanged(currentAction_ = newAction);

		if(oldAction) {
			disconnect(oldAction, 0, this, 0);
			delete oldAction;
		}

		connect(currentAction_, SIGNAL(stateChanged(int,int)), this, SLOT(onCurrentActionStateChanged(int,int)));
		connect(currentAction_, SIGNAL(progressChanged(double,double)), this, SIGNAL(currentActionProgressChanged(double,double)));
		connect(currentAction_, SIGNAL(statusTextChanged(QString)), this, SIGNAL(currentActionStatusTextChanged(QString)));
		connect(currentAction_, SIGNAL(expectedDurationChanged(double)), this, SIGNAL(currentActionExpectedDurationChanged(double)));

		currentAction_->start();
	}
}

// This is a non-GUI class, and we're popping up a QMessageBox. Not ideal; hope you'll let us get away with that.

#include <QMessageBox>
#include <QPushButton>
int AMActionRunner::internalAskUserWhatToDoAboutFailedAction(AMAction* action)
{
	QMessageBox box;
	box.setWindowTitle("Sorry! An action failed. What should we do?");
	box.setText("A '" % action->info()->typeDescription() % "' action in the workflow didn't complete successfully. What should we do?");
	box.setInformativeText("Action: " % action->info()->shortDescription() % "\n\nYou can try it again, or give up and move on to the next action in the workflow.");

	QPushButton* moveOnButton = new QPushButton("Move on to next action");
	QPushButton* tryAgainButton = new QPushButton("Try action again");

	box.addButton(moveOnButton, QMessageBox::RejectRole);
	box.addButton(tryAgainButton, QMessageBox::AcceptRole);
	box.setDefaultButton(tryAgainButton);

	box.exec();
	if(box.clickedButton() == tryAgainButton)
		return AMAction::AttemptAnotherCopyResponse;
	else
		return AMAction::MoveOnResponse;
}

void AMActionRunner::runActionImmediately(AMAction *action)
{
	connect(action, SIGNAL(stateChanged(int,int)), this, SLOT(onImmediateActionStateChanged(int,int)));
	action->start();
}

void AMActionRunner::onImmediateActionStateChanged(int state, int previousState)
{
	Q_UNUSED(previousState)

	AMAction* action = qobject_cast<AMAction*>(sender());
	if(!action)
		return;

	if(state == AMAction::Failed) {
		// What should we do?
		int failureResponse = action->failureResponseInActionRunner();
		if(failureResponse == AMAction::PromptUserResponse)
			failureResponse = internalAskUserWhatToDoAboutFailedAction(action);

		if(failureResponse == AMAction::AttemptAnotherCopyResponse) {
			// make a fresh copy of this action and run it now
			AMAction* retryAction = action->createCopy();
			connect(retryAction, SIGNAL(stateChanged(int,int)), this, SLOT(onImmediateActionStateChanged(int,int)));
			retryAction->start();
		}
	}

	// for all three final states, this is how we wrap things up for the current action:
	if(state == AMAction::Failed ||
			state == AMAction::Cancelled ||
			state == AMAction::Succeeded) {

		// log it:
		// todozzzz AMActionHistoryLogger::s()->logCompletedAction(action);

		// disconnect and delete it
		disconnect(action, 0, this, 0);
		delete action;
	}
}


AMActionModelItem::AMActionModelItem(AMAction *action) : QStandardItem()
{
	action_ = action;
}

QVariant AMActionModelItem::data(int role) const
{
	if(role == Qt::DisplayRole) {
		return action_->info()->shortDescription();
	}
	return QStandardItem::data(role);
}

#include "AMActionRunner3.h"
#include "actions3/AMAction3.h"
#include "actions3/AMActionLog3.h"

#include "util/AMErrorMonitor.h"

#include <QStringBuilder>
#include <QPixmapCache>
#include <QTimer>

#include "actions3/AMListAction3.h"
#include "actions3/AMLoopAction3.h"

#include <QDebug>

#include "actions3/AMActionRegistry3.h"

AMActionRunner3* AMActionRunner3::instance_ = 0;

AMActionRunner3::AMActionRunner3(QObject *parent) :
	QObject(parent)
{
	currentAction_ = 0;
	isPaused_ = true;
    queueModel_ = new AMActionRunnerQueueModel3(this, this);
}

AMActionRunner3::~AMActionRunner3() {
	while(queuedActionCount() != 0) {
		deleteActionInQueue(queuedActionCount()-1);
	}
}

AMActionRunner3 * AMActionRunner3::s()
{
	if(!instance_)
        instance_ = new AMActionRunner3();
	return instance_;
}

void AMActionRunner3::releaseActionRunner()
{
	delete instance_;
	instance_ = 0;
}

void AMActionRunner3::onCurrentActionStateChanged(int state, int previousState)
{
	emit currentActionStateChanged(state, previousState);

    if(state == AMAction3::Failed) {
		// What should we do?
		int failureResponse = currentAction_->failureResponseInActionRunner();
        if(failureResponse == AMAction3::PromptUserResponse)
			failureResponse = internalAskUserWhatToDoAboutFailedAction(currentAction_);

        if(failureResponse == AMAction3::AttemptAnotherCopyResponse) {
			// make a fresh copy of this action and put it at the front of the queue to run again.
			insertActionInQueue(currentAction_->createCopy(), 0);
		}
		// other failure response is to MoveOn, which we'll do anyway.
	}

    if(state == AMAction3::Cancelled) {
		// Usability guess for now: If the user intervened to cancel, they probably want to make some changes to something. Let's pause the workflow for now to let them do that, rather than automatically go on to the next.
		setQueuePaused(true);
	}

	// for all three final states, this is how we wrap things up for the current action:
    if(state == AMAction3::Failed ||
            state == AMAction3::Cancelled ||
            state == AMAction3::Succeeded) {

		// log it, unless it's a nested action that wants to take care of logging its sub-actions itself.
        AMListAction3* loopAction = qobject_cast<AMListAction3*>(currentAction_);
        if(!(loopAction && loopAction->shouldLogSubActionsSeparately())) {
            if(!AMActionLog3::logCompletedAction(currentAction_)) {
				AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -200, "There was a problem logging the completed action to your database.  Please report this problem to the Acquaman developers."));
			}
		}

		// move onto the next, if there is one, and disconnect and delete the old one.
		internalDoNextAction();
	}
}

void AMActionRunner3::insertActionInQueue(AMAction3 *action, int index)
{
	if(!action)
		return;

	if(index < 0 || index > queuedActions_.count())
		index = queuedActions_.count();

	emit queuedActionAboutToBeAdded(index);
	queuedActions_.insert(index, action);
	emit queuedActionAdded(index);

	// was this the first action inserted into a running but empty queue? Start it up!
	if(!isPaused_ && !actionRunning())
		internalDoNextAction();
}

bool AMActionRunner3::deleteActionInQueue(int index)
{
	if(index<0 || index>=queuedActions_.count())
		return false;

	emit queuedActionAboutToBeRemoved(index);
    AMAction3* action= queuedActions_.takeAt(index);
	emit queuedActionRemoved(index);

	delete action;

	return true;
}

bool AMActionRunner3::duplicateActionInQueue(int index)
{
	if(index<0 || index>=queuedActions_.count())
		return false;

	insertActionInQueue(queuedActionAt(index)->createCopy(), index+1);
	return true;
}

bool AMActionRunner3::duplicateActionsInQueue(const QList<int> &indexesToCopy)
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

	// todozzz use sort method?

	// note: because we're inserting at higher indexes than any of indexesToCopy, we know that any that we're supposed to copy won't change positions as we start inserting.
	foreach(int index, indexesToCopy) {
		insertActionInQueue(queuedActionAt(index)->createCopy(),
							++maxIndex);
	}
	return true;
}

bool AMActionRunner3::moveActionInQueue(int currentIndex, int finalIndex)
{
	if(currentIndex == finalIndex)
		return false;

	if(currentIndex < 0 || currentIndex >= queuedActionCount())
		return false;

	if(finalIndex <0 || finalIndex >= queuedActionCount())
		finalIndex = queuedActionCount()-1;

	emit queuedActionAboutToBeRemoved(currentIndex);
    AMAction3* moveAction = queuedActions_.takeAt(currentIndex);
	emit queuedActionRemoved(currentIndex);

	emit queuedActionAboutToBeAdded(finalIndex);
	queuedActions_.insert(finalIndex, moveAction);
	emit queuedActionAdded(finalIndex);

	return true;
}

void AMActionRunner3::setQueuePaused(bool isPaused) {
	if(isPaused == isPaused_)
		return;

	emit queuePausedChanged(isPaused_ = isPaused);

	// if we've gone from paused to running, and there is no action running right now, and we have actions we can run... Start the next.
	if(!isPaused_ && queuedActionCount() != 0 && !actionRunning())
		internalDoNextAction();
}

void AMActionRunner3::internalDoNextAction()
{
	// signal that no action is running? (queue paused, or we're out of actions in the queue to run...)
	if(isPaused_ || queuedActionCount() == 0) {
		if(currentAction_) {
            AMAction3* oldAction = currentAction_;
			emit currentActionChanged(currentAction_ = 0);
			oldAction->deleteLater(); // the action might have sent notifyFailed() or notifySucceeded() in the middle a deep call stack... In that case, we might actually still be executing inside the action's code, so better not delete it until that all has a chance to finish. (Otherwise... Crash!)
		}
	}

	// Otherwise, keep on keepin' on. Disconnect and delete the old current action (if there is one... If we're resuming from pause or starting for the first time, there won't be)., and connect and start the next one.
	else {
        AMAction3* oldAction = currentAction_;

		emit queuedActionAboutToBeRemoved(0);
        AMAction3* newAction = queuedActions_.takeAt(0);
		emit queuedActionRemoved(0);

		emit currentActionChanged(currentAction_ = newAction);

		if(oldAction) {
			disconnect(oldAction, 0, this, 0);
			oldAction->deleteLater();	// the action might have sent notifyFailed() or notifySucceeded() in the middle a deep call stack... In that case, we might actually still be executing inside the action's code, so better not delete it until that all has a chance to finish. (Otherwise... Crash!)
		}

		connect(currentAction_, SIGNAL(stateChanged(int,int)), this, SLOT(onCurrentActionStateChanged(int,int)));
		connect(currentAction_, SIGNAL(progressChanged(double,double)), this, SIGNAL(currentActionProgressChanged(double,double)));
		connect(currentAction_, SIGNAL(statusTextChanged(QString)), this, SIGNAL(currentActionStatusTextChanged(QString)));
		connect(currentAction_, SIGNAL(expectedDurationChanged(double)), this, SIGNAL(currentActionExpectedDurationChanged(double)));

		// to avoid a growing call stack if a long series of actions are all failing inside their start() method... We wait to run the next one until we get back to the even loop.
		QTimer::singleShot(0, currentAction_, SLOT(start()));
	}
}

// This is a non-GUI class, and we're popping up a QMessageBox. Not ideal; hope you'll let us get away with that.

#include <QMessageBox>
#include <QPushButton>
int AMActionRunner3::internalAskUserWhatToDoAboutFailedAction(AMAction3* action)
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
        return AMAction3::AttemptAnotherCopyResponse;
	else
        return AMAction3::MoveOnResponse;
}

void AMActionRunner3::runActionImmediately(AMAction3 *action)
{
	connect(action, SIGNAL(stateChanged(int,int)), this, SLOT(onImmediateActionStateChanged(int,int)));
	immediateActions_ << action;
	action->start();
}

void AMActionRunner3::onImmediateActionStateChanged(int state, int previousState)
{
	Q_UNUSED(previousState)

    AMAction3* action = qobject_cast<AMAction3*>(sender());
	if(!action)
		return;

    if(state == AMAction3::Failed) {
		// What should we do?
		int failureResponse = action->failureResponseInActionRunner();
        if(failureResponse == AMAction3::PromptUserResponse)
			failureResponse = internalAskUserWhatToDoAboutFailedAction(action);

        if(failureResponse == AMAction3::AttemptAnotherCopyResponse) {
			// make a fresh copy of this action and run it now
            AMAction3* retryAction = action->createCopy();
			connect(retryAction, SIGNAL(stateChanged(int,int)), this, SLOT(onImmediateActionStateChanged(int,int)));
			immediateActions_ << retryAction;
			QTimer::singleShot(0, retryAction, SLOT(start()));
		}
	}

	// for all three final states, this is how we wrap things up for the current action:
    if(state == AMAction3::Failed ||
            state == AMAction3::Cancelled ||
            state == AMAction3::Succeeded) {

		// remove from the list of current immediate actions
		immediateActions_.removeAll(action);

		// log it:
        if(!AMActionLog3::logCompletedAction(action)) {
			AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -201, "There was a problem logging the completed action to your database.  Please report this problem to the Acquaman developers."));
		}

		// disconnect and delete it
		disconnect(action, 0, this, 0);
		action->deleteLater();
	}
}







AMAction3 * AMActionRunner3::immediateActionAt(int index)
{
	if(index < 0 || index >= immediateActions_.count()) {
		return 0;
	}
	return immediateActions_.at(index);
}

bool AMActionRunner3::cancelCurrentAction()
{
	if(currentAction_) {
		currentAction_->cancel();
		return true;
	}
	return false;
}

bool AMActionRunner3::pauseCurrentAction()
{
	if(currentAction_ && currentAction_->canPause()) {
		return currentAction_->pause();
	}
	return false;
}

bool AMActionRunner3::resumeCurrentAction()
{
    if(currentAction_ && currentAction_->state() == AMAction3::Paused) {
		return currentAction_->resume();
	}
	return false;
}

bool AMActionRunner3::cancelImmediateActions()
{
	if(immediateActions_.isEmpty())
		return false;

    foreach(AMAction3* action, immediateActions_)
		action->cancel();
	return true;
}

int AMActionRunner3::indexOfQueuedAction(const AMAction3 *action) {
	for(int i=0,cc=queuedActions_.count(); i<cc; i++) {
		if(action == queuedActions_.at(i))
			return i;
	}
	return -1;
}

AMActionRunnerQueueModel3::AMActionRunnerQueueModel3(AMActionRunner3 *actionRunner, QObject *parent) : QAbstractItemModel(parent)
{
	actionRunner_ = actionRunner;
	connect(actionRunner_, SIGNAL(queuedActionAboutToBeAdded(int)), this, SLOT(onActionAboutToBeAdded(int)));
	connect(actionRunner_, SIGNAL(queuedActionAdded(int)), this, SLOT(onActionAdded(int)));
	connect(actionRunner_, SIGNAL(queuedActionAboutToBeRemoved(int)), this, SLOT(onActionAboutToBeRemoved(int)));
	connect(actionRunner_, SIGNAL(queuedActionRemoved(int)), this, SLOT(onActionRemoved(int)));
}

QModelIndex AMActionRunnerQueueModel3::index(int row, int column, const QModelIndex &parent) const
{
	if(column != 0)
		return QModelIndex();

	// if no parent: this is top-level. use AMActionRunner API.
	if(!parent.isValid()) {
		// check for range
		if(row < 0 || row >= actionRunner_->queuedActionCount())
			return QModelIndex();
		return createIndex(row, 0, actionRunner_->queuedActionAt(row));
	}
	// otherwise this is an index for a sub-action of an AMNestedAction.
	else {
        AMListAction3* parentAction = qobject_cast<AMListAction3*>(actionAtIndex(parent));
		if(!parentAction) {
			qWarning() << "AMActionRunnerQueueModel: Warning: requested a child index when the parent was not an AMNestedAction.";
			return QModelIndex();
		}
		if(row < 0 || row >= parentAction->subActionCount())
			return QModelIndex();
		return createIndex(row, 0, parentAction->subActionAt(row));
	}
}

QModelIndex AMActionRunnerQueueModel3::parent(const QModelIndex &child) const
{
	if(!child.isValid())
		return QModelIndex();

    AMAction3* childAction = actionAtIndex(child);
	if(!childAction)
		return QModelIndex();

	// if childAction->parentAction() returns 0 (ie: no parent -- its at the top level) then this will return an invalid index, meaning it has no parent.
	return indexForAction(childAction->parentAction());
}

int AMActionRunnerQueueModel3::rowCount(const QModelIndex &parent) const
{
	if(!parent.isValid()) {
		return actionRunner_->queuedActionCount();
	}

    AMListAction3* loopAction = qobject_cast<AMListAction3*>(actionAtIndex(parent));
    if(loopAction)
        return loopAction->subActionCount();
	else
		return 0;

}

int AMActionRunnerQueueModel3::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return 1;
}

QVariant AMActionRunnerQueueModel3::data(const QModelIndex &index, int role) const
{
    AMAction3* action = actionAtIndex(index);
	if(!action) {
		qWarning() << "AMActionRunnerQueueModel: Warning: No action at index " << index;
		return QVariant();
	}

	if(role == Qt::DisplayRole) {
		return action->info()->shortDescription();
	}
	else if(role == Qt::DecorationRole) {
		QPixmap p;
		QString iconFileName = action->info()->iconFileName();
		if(QPixmapCache::find("AMActionIcon" % iconFileName, &p))
			return p;
		else {
			p.load(iconFileName);
			p = p.scaledToHeight(32, Qt::SmoothTransformation);
			QPixmapCache::insert("AMActionIcon" % iconFileName, p);
			return p;
		}
	}
	else if(role == Qt::SizeHintRole) {
		return QSize(-1, 48);
	}

	return QVariant();
}

Qt::ItemFlags AMActionRunnerQueueModel3::flags(const QModelIndex &index) const
{
	// need to be able to drop onto the top-level index.
	if(!index.isValid())
		return Qt::ItemIsDropEnabled;

	Qt::ItemFlags rv = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

	// Nested actions can accept drops onto them (to insert actions)
    AMListAction3* loopAction = qobject_cast<AMListAction3*>(actionAtIndex(index));
    if(loopAction)
		rv |= Qt::ItemIsDropEnabled;

	// Actions are editable if they have an editor registered for their ActionInfo.
    if(AMActionRegistry3::s()->editorRegisteredForInfo(actionAtIndex(index)->info()))
		rv |= Qt::ItemIsEditable;

	return rv;
}

bool AMActionRunnerQueueModel3::hasChildren(const QModelIndex &parent) const
{
	if(!parent.isValid())
		return true;	// top level: must have children.
	else {
		// other levels: have children if its a nested action.
        AMListAction3* loopAction = qobject_cast<AMListAction3*>(actionAtIndex(parent));
        return (loopAction != 0);
	}
}

AMAction3 * AMActionRunnerQueueModel3::actionAtIndex(const QModelIndex &index) const
{
	if(!index.isValid())
		return 0;
    return static_cast<AMAction3*>(index.internalPointer());
}

void AMActionRunnerQueueModel3::onActionAboutToBeAdded(int i)
{
	beginInsertRows(QModelIndex(), i, i);
}

void AMActionRunnerQueueModel3::onActionAdded(int i)
{
	internalConnectNestedActions(actionRunner_->queuedActionAt(i));
	endInsertRows();
}

void AMActionRunnerQueueModel3::onActionAboutToBeRemoved(int i)
{
	internalDisconnectNestedActions(actionRunner_->queuedActionAt(i));
	beginRemoveRows(QModelIndex(), i, i);
}

void AMActionRunnerQueueModel3::onActionRemoved(int index)
{
	Q_UNUSED(index)
	endRemoveRows();
}

QModelIndex AMActionRunnerQueueModel3::indexForAction(AMAction3 *action) const
{
	if(!action)
		return QModelIndex();

    AMAction3* parentAction = action->parentAction();
	if(!parentAction) {
		// action is in the top-level. Do a linear search for it in the actionRunner_ API.
		int row = actionRunner_->indexOfQueuedAction(action);
		if(row == -1) {
			qWarning() << "AMActionRunnerQueueModel: Warning: action not found in AMActionRunner.";
			return QModelIndex();
		}
		return createIndex(row, 0, action);
	}
	else {
		// we do a have parent action. Do a linear search for ourself in the parent AMNestedAction.
        int row = ((AMListAction3 *)parentAction)->indexOfSubAction(action);
		if(row == -1) {
			qWarning() << "AMActionRunnerQueueModel: Warning: action not found in nested action.";
			return QModelIndex();
		}
		return createIndex(row, 0, action);
	}
}

void AMActionRunnerQueueModel3::internalConnectNestedActions(AMAction3 *action)
{
    AMListAction3* loopAction = qobject_cast<AMListAction3*>(action);
    if(loopAction) {
		// qDebug() << "Connecting and remembering nested action" << action->info()->shortDescription();
        disconnect(loopAction, 0, this, 0);
        connect(loopAction, SIGNAL(subActionAboutToBeAdded(int)), this, SLOT(onSubActionAboutToBeAdded(int)));
        connect(loopAction, SIGNAL(subActionAdded(int)), this, SLOT(onSubActionAdded(int)));
        connect(loopAction, SIGNAL(subActionAboutToBeRemoved(int)), this, SLOT(onSubActionAboutToBeRemoved(int)));
        connect(loopAction, SIGNAL(subActionRemoved(int)), this, SLOT(onSubActionRemoved(int)));

        for(int i=0,cc=loopAction->subActionCount(); i<cc; i++)
            internalConnectNestedActions(loopAction->subActionAt(i));
	}
}

void AMActionRunnerQueueModel3::internalDisconnectNestedActions(AMAction3 *action)
{
    AMListAction3* loopAction = qobject_cast<AMListAction3*>(action);
    if(loopAction) {
		// qDebug() << "Disconnecting and forgetting nested action" << action->info()->shortDescription();
        disconnect(loopAction, 0, this, 0);

        for(int i=0,cc=loopAction->subActionCount(); i<cc; i++)
            internalDisconnectNestedActions(loopAction->subActionAt(i));
	}
}

void AMActionRunnerQueueModel3::onSubActionAboutToBeAdded(int index)
{
    AMListAction3* parentAction = qobject_cast<AMListAction3*>(sender());
	lastSender_ = parentAction;
	if(!parentAction) {
		qWarning() << "AMActionRunnerQueueModel: Warning: invalid parent sent subActionAboutToBeAdded().";
		return;
	}

	QModelIndex parentIndex = indexForAction(parentAction);
	if(!parentIndex.isValid()) {
		qWarning() << "AMActionRunnerQueueModel: Warning: Nested Action parent not found for the added sub-action.";
		return;
	}

	beginInsertRows(parentIndex, index, index);
}

void AMActionRunnerQueueModel3::onSubActionAdded(int index)
{
    AMListAction3* parentAction = qobject_cast<AMListAction3*>(sender());
	if(parentAction != lastSender_) {
		qWarning() << "AMActionRunnerQueueModel: Warning: Unmatched calls to onSubActionAboutToBeAdded/onSubActionAdded.";
	}
	else {
		internalConnectNestedActions(parentAction->subActionAt(index));
	}
	endInsertRows();
}

void AMActionRunnerQueueModel3::onSubActionAboutToBeRemoved(int index)
{
    AMListAction3* parentAction = qobject_cast<AMListAction3*>(sender());
	lastSender_ = parentAction;
	if(!parentAction) {
		qWarning() << "AMActionRunnerQueueModel: Warning: invalid parent sent subActionAboutToBeRemoved().";
		return;
	}

	internalDisconnectNestedActions(parentAction->subActionAt(index));

	QModelIndex parentIndex = indexForAction(parentAction);
	if(!parentIndex.isValid()) {
		qWarning() << "AMActionRunnerQueueModel: Warning: Nested Action parent not found for the removed sub-action.";
		return;
	}

	beginRemoveRows(parentIndex, index, index);
}

void AMActionRunnerQueueModel3::onSubActionRemoved(int index)
{
	Q_UNUSED(index)
    if(qobject_cast<AMListAction3*>(sender()) != lastSender_) {
		qWarning() << "AMActionRunnerQueueModel: Warning: Unmatched calls to onSubActionAboutToBeRemoved/onSubActionRemoved.";
	}
	endRemoveRows();
}

void AMActionRunnerQueueModel3::deleteActionsInQueue(QModelIndexList indexesToDelete)
{
	QList<int> topLevelIndexesToDelete;
	QList<QPersistentModelIndex> nestedIndexesToDelete;

	foreach(QModelIndex i, indexesToDelete) {
		// Sort out the top-level actions...
		if(!i.parent().isValid()) {
			topLevelIndexesToDelete << i.row();
		}
		// ... from those that are inside an AMNestedAction
		else {
			nestedIndexesToDelete << QPersistentModelIndex(i);
		}
	}
	// Delete the top-level actions. Need to delete from largest index to smallest index, otherwise the indexes will change as we go. [We could use persistent indexes to avoid this problem, but the performance is better if we don't.]
	qSort(topLevelIndexesToDelete);
	for(int i=topLevelIndexesToDelete.count()-1; i>=0; i--)
		actionRunner_->deleteActionInQueue(topLevelIndexesToDelete.at(i));

	// Now, do we have any nested actions left to delete? They might have been deleted if they were inside parents that were deleted, so make sure to check for valid persistent indexes.
	foreach(QPersistentModelIndex i, nestedIndexesToDelete) {
		if(i.isValid()) {
			// get their parent model item... which should represent a nested action
            AMListAction3* loopAction = qobject_cast<AMListAction3*>(actionAtIndex(i.parent()));
            if(loopAction) {
                loopAction->deleteSubAction(i.row());
			}
		}
	}
}


void AMActionRunnerQueueModel3::duplicateActionsInQueue(const QModelIndexList &indexesToCopy) {
	// nothing to do:
	if(indexesToCopy.isEmpty())
		return;

	// This function works if all the indexes are at the same level of the model tree hierarchy.  Let's confirm that first.
	QModelIndex firstParent = indexesToCopy.at(0).parent();
	foreach(QModelIndex i, indexesToCopy)
		if(i.parent() != firstParent)
			return;

	// OK.  Are they all top-level indexes?
	if(!firstParent.isValid()) {
		QList<int> topLevelIndexes;
		foreach(QModelIndex i, indexesToCopy)
			topLevelIndexes << i.row();
		// call the top-level version:
		actionRunner_->duplicateActionsInQueue(topLevelIndexes);
	}
	// Otherwise, these indexes are sub-actions of some AMNestedAction
	else {
		// find the nested action:
        AMLoopAction3* loopAction = qobject_cast<AMLoopAction3*>(actionAtIndex(firstParent));
        if(loopAction) {
			QList<int> subActionIndexes;
			foreach(QModelIndex i, indexesToCopy)
				subActionIndexes << i.row();
			// use the AMNestedAction API to duplicate these subactions.
            loopAction->duplicateSubActions(subActionIndexes);
		}
	}
}


bool AMActionRunnerQueueModel3::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(column)

	// Option 1: Handle internal move actions, where the \c data is a list of the qmodelindexes to move
	if(action == Qt::MoveAction) {
        const AMModelIndexListMimeData3* milData = qobject_cast<const AMModelIndexListMimeData3*>(data);
		if(milData) {
			QList<QPersistentModelIndex> mil = milData->modelIndexList();
			// go through and make sure all indexes are valid, and have the same parent index. If we're asked to move multiple indexes from different levels of the hierarchy, I have no idea how we'd interpret that.
			if(mil.isEmpty())
				return false;	// nothing to do!
			QPersistentModelIndex first = mil.at(0);
			foreach(const QPersistentModelIndex& mi, mil) {
				if(!mi.isValid()) {
					return false;	// this item was removed from the model while the drag was in progress.  Be faster next time, user.
				}
				if(mi.parent() != first.parent())
					return false;	// we need all indexes to move to be at the same level of the hierarchy.
			}
			// OK, these indexes are OK to use.  Sort them:
			qSort(mil);

			// Case A: Source: These are all top-level actions in the queue.
			if(!first.parent().isValid()) {

				// Subcase A1: destination: They are being moved to another location at the top level. We can move them using the AMActionRunner API.
				if(!parent.isValid()) {

					// qDebug() << "Moving from top level to top level.";

					// Get a persistent model index corresponding to the destination. (It might move as we move things around.)
					QPersistentModelIndex destinationIndex(index(row, 0, parent));
					// go backward through the list of indexes to move, inserting at destination. This will ensure that we maintain ordering. The persistent indexes adjust themselves to keep pointing to the same objects as we insert/remove.
					for(int i=0,cc=mil.count(); i<cc; i++) {
						int destinationRow = destinationIndex.row();
						if(destinationRow > mil.at(i).row())
							destinationRow--;
						actionRunner_->moveActionInQueue(mil.at(i).row(), destinationRow);
					}
					return true;
				}
				// Subcase A2: destination: They are being moved to inside a nested action.
				else {
					// qDebug() << "Moving from top level to nested action.";

					// parent is valid... It represents the nested action we're supposed to drop these actions inside of.
                    AMListAction3* loopAction = qobject_cast<AMListAction3*>(actionAtIndex(parent));
                    if(!loopAction) {
						qWarning() << "AMActionRunnerQueueModel: Warning: Asked to drop actions inside a nested action that wasn't valid.";
						return false;
					}
					int targetRow = row;
                    if(targetRow < 0 || targetRow > loopAction->subActionCount())
                        targetRow = loopAction->subActionCount();
					for(int i=0,cc=mil.count(); i<cc; i++) {
                        loopAction->insertSubAction(actionRunner_->queuedActionAt(mil.at(i).row())->createCopy(), targetRow++);
						actionRunner_->deleteActionInQueue(mil.at(i).row());
					}
					return true;
				}
			}
			// Case B: Source: These are sub-actions inside a nested action...
			else {
                AMListAction3* sourceParentAction = qobject_cast<AMListAction3*>(actionAtIndex(first.parent()));
				if(!sourceParentAction) {
					qWarning() << "AMActionQueueModel: Warning: Asked to move actions from inside a nested action that wasn't valid.";
					return false;
				}

				// Subcase B0: The destination is the same as the source. Just rearranging within one AMNestedAction.
				if(first.parent() == parent) {
					QPersistentModelIndex destinationIndex(index(row, 0, parent));
					for(int i=0,cc=mil.count(); i<cc; i++) {
                        AMAction3* moveAction = sourceParentAction->takeSubActionAt(mil.at(i).row());
						if(moveAction)
							sourceParentAction->insertSubAction(moveAction, destinationIndex.row());
						else
							qWarning() << "AMActionRunnerQueueModel: Warning: Received an invalid item when rearranging actions inside a nested action.";
					}
					return true;
				}
				// otherwise, moving to a different parent
				else {
					// Subcase B1: The destination is the top level. Need to add the copied actions directly to AMActionRunner.
					if(!parent.isValid()) {
						// qDebug() << "Move from one nested action to top level.";
						int targetRow = row;
						if(targetRow < 0 || targetRow > actionRunner_->queuedActionCount())
							targetRow = actionRunner_->queuedActionCount();
						for(int i=0,cc=mil.count(); i<cc; i++) {
                            actionRunner_->insertActionInQueue(sourceParentAction->takeSubActionAt(mil.at(i).row()), targetRow++);
						}
						return true;
					}
					// Subcase B2: The destination is a different sub-action.
					else {
						// qDebug() << "Move from one nested action to another.";
                        AMListAction3* destParentAction = qobject_cast<AMListAction3*>(actionAtIndex(parent));
						if(!destParentAction) {
							qWarning() << "AMActionQueueModel: Warning: Asked to move actions into a nested action that wasn't valid.";
							return false;
						}
						int targetRow = row;
						if(targetRow < 0 || targetRow > destParentAction->subActionCount())
							targetRow = destParentAction->subActionCount();
						for(int i=0,cc=mil.count(); i<cc; i++) {
                            destParentAction->insertSubAction(sourceParentAction->takeSubActionAt(mil.at(i).row()), targetRow++);
						}
						return true;
					}
				}
			}
		}
		return false;
	}

	// if we return false to the DropAction, it might retry with IgnoreAction. We need to accept that one.
	else if(action == Qt::IgnoreAction) {
		qDebug() << "AMActionRunnerQueueModel: Wow: Qt actually behaved according to spec and offered the IgnoreAction. Too bad this never happens...";
		return true;
	}

	return false;
}

QMimeData * AMActionRunnerQueueModel3::mimeData(const QModelIndexList &indexes) const
{
	if(indexes.isEmpty())
		return 0;
	// We only support dragging if all the indexes are at the same level of the hierarchy. Let's check that here, and return 0 if not.
	QModelIndex firstParent = indexes.at(0).parent();
	foreach(const QModelIndex& index, indexes)
		if(index.parent() != firstParent)
			return 0;

	// alright, looks good. Turn them into persistent indexes and pass them off inside our custom (not externally-sharable) mime type container.
    return new AMModelIndexListMimeData3(indexes);
}

QStringList AMActionRunnerQueueModel3::mimeTypes() const
{
	return QStringList() << "application/octet-stream";
}

bool AMActionRunnerQueueModel3::removeRows(int row, int count, const QModelIndex &parent)
{
	Q_UNUSED(row)
	Q_UNUSED(count)
	Q_UNUSED(parent)

	qWarning() << "AMActionRunnerQueueModel: Warning: Ignoring request from view to remove rows.";

	return false;
}

bool AMActionRunner3::runActionImmediatelyInQueue(AMAction3 *action)
{
	if(actionRunning())
		return false;
	if(!action)
		return false;

	bool queueWasPaused = queuePaused();
	insertActionInQueue(action, 0);
	setQueuePaused(false);	// this will start up the first action, but only that one.
	setQueuePaused(queueWasPaused);
	return true;
}



//void AMActionQueueModel::traverse1(const QModelIndex &parent, QString &outstring, int level)
//{
//	for(int row=0, cc=rowCount(parent); row<cc; row++) {
//		QModelIndex i = index(row, 0, parent);
//		// print the action:
//		AMActionQueueModelItem* item = static_cast<AMActionQueueModelItem*>(itemFromIndex(i));
//		if(item) {
//			AMAction* action = item->action();
//			if(action) {
//				outstring.append(nSpaces(level*4)).append(action->info()->shortDescription()).append("\n");
//			}
//			else
//				outstring.append(nSpaces(level*4)).append("[Invalid Action?]").append("\n");
//		}
//		else
//			outstring.append(nSpaces(level*4)).append("[Invalid Item?]").append("\n");

//		traverse1(i, outstring, level+1);
//	}
//}

//QString AMActionQueueModel::nSpaces(int n)
//{
//	QString rv;
//	for(int i=0; i<n; i++)
//		rv.append(" ");
//	return rv;
//}

//void AMActionQueueModel::traverse2(const AMNestedAction *parent, QString &outstring, int level)
//{
//	if(!parent) {
//		for(int i=0,cc=actionRunner_->queuedActionCount(); i<cc; i++) {
//			AMAction* action = actionRunner_->queuedActionAt(i);
//			if(action) {
//				outstring.append(nSpaces(level*4)).append(action->info()->shortDescription()).append("\n");
//				AMNestedAction* na = qobject_cast<AMNestedAction*>(action);
//				if(na) {
//					traverse2(na, outstring, level+1);
//				}
//			}
//			else {
//				outstring.append(nSpaces(level*4)).append("[Invalid Action?]").append("\n");
//			}
//		}
//	}
//	else {
//		for(int i=0, cc=parent->subActionCount(); i<cc; i++) {
//			const AMAction* action = parent->subActionAt(i);
//			if(action) {
//				outstring.append(nSpaces(level*4)).append(action->info()->shortDescription()).append("\n");
//				const AMNestedAction* na = qobject_cast<const AMNestedAction*>(action);
//				if(na) {
//					traverse2(na, outstring, level+1);
//				}
//			}
//			else {
//				outstring.append(nSpaces(level*4)).append("[Invalid Action?]").append("\n");
//			}
//		}
//	}
//}

//void AMActionQueueModel::checkTreeConsistency()
//{
//	QString t1;
//	traverse1(QModelIndex(), t1);
//	QString t2;
//	traverse2(0, t2);

//	if(t1 == t2)
//		qDebug() << "****MODEL CHECKED OK***";
//	else {
//		qDebug() << "T1:";
//		qDebug() << t1;
//		qDebug() << "\n\nT2:";
//		qDebug() << t2;
//		qDebug() << "\n\n***MODEL CHECK FAILED***";
//	}
//}

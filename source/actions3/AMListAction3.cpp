#include "AMListAction3.h"
#include "actions3/AMActionLog3.h"

#include <QStringBuilder>

// Used for qWarning() messages that may be useful to developers trying to debug action implementatons
#include <QDebug>

AMListAction3::AMListAction3(AMActionInfo3* info, SubActionMode subActionMode, QObject *parent) :
    AMAction3(info, parent)
{
	currentSubActionIndex_ = -1;	// prior to running any subactions
	subActionMode_ = subActionMode;
}

// Copy constructor. Takes care of making copies of the sub-actions
AMListAction3::AMListAction3(const AMListAction3& other) : AMAction3(other) {
	subActionMode_ = other.subActionMode_;
	currentSubActionIndex_ = -1; // prior to running an subactions

    foreach(AMAction3* action, other.subActions_)
		subActions_ << action->createCopy();
}
// Destructor: deletes the sub-actions
AMListAction3::~AMListAction3() {
	while(!subActions_.isEmpty())
		delete subActions_.takeLast();
}

int AMListAction3::indexOfSubAction(const AMAction3 *action) const
{
    for(int i=0, count = subActionCount(); i<count; i++) {
        if(action == subActionAt(i))
            return i;
    }
    return -1;
}

AMAction3 * AMListAction3::subActionAt(int index)
{
    if(index < 0 || index >= subActionCount())
		return 0;

	return subActions_.at(index);
}

const AMAction3 * AMListAction3::subActionAt(int index) const
{
    if(index < 0 || index >= subActionCount())
        return 0;

	return subActions_.at(index);
}

bool AMListAction3::insertSubAction(AMAction3 *action, int index)
{
    if(state() != Constructed) {
		qWarning() << "AMListAction: Cannot add sub-actions once the action is already running.";
		return false;
	}

    if (!action)
        return false;

    if(index<0 || index > subActionCount())
        index = subActionCount();

	emit subActionAboutToBeAdded(index);
    action->setParentAction(this);
	subActions_.insert(index, action);
	emit subActionAdded(index);

	return true;
}

AMAction3 * AMListAction3::takeSubActionAt(int index)
{
    if(state() != Constructed) {
		qWarning() << "AMListAction: Cannot remove sub-actions once the action is already running.";
		return 0;
	}

	if(index <0 || index >= subActions_.count())
		return 0;

	emit subActionAboutToBeRemoved(index);
    AMAction3* action = subActions_.takeAt(index);
    action->setParentAction(0);
	emit subActionRemoved(index);

	return action;
}

bool AMListAction3::deleteSubAction(int index)
{
    AMAction3* action = takeSubActionAt(index);
    delete action;	// harmless if 0

    return (action != 0);
}

bool AMListAction3::canPause() const
{
    if(subActionMode() == Sequential) {
		// if we have no actions, then cannot pause; we'll complete instantly.
		if(subActionCount() == 0)
			return false;
		// if we just have one sub-action and it cannot pause, then we can't pause.
		if(subActionCount() == 1)
            return subActionAt(0)->canPause();
		// More than one action. Are we on the last action? Then whether we can pause depends on whether that last action can pause
		if(currentSubActionIndex() == subActionCount()-1)
            return currentSubAction()->canPause();
		// If we've made it here, we have more than one action and we're not on the last action. Therefore, at least we can pause between actions even if they can't pause themselves.
		return true;
	}

    // in parallel mode, we can pause if all still-running sub-actions can.
	else {

		bool canDoPause = true;
        foreach(AMAction3* action, subActions_)
            canDoPause &= action->canPause();

		return canDoPause;
	}
}

void AMListAction3::startImplementation()
{
	// if this was called by the base class, we know that we are in the Starting state.

	// no actions? That's easy...
	if(subActionCount() == 0) {
		setStarted();
		setSucceeded();	// done and done.
		return;
	}

    if(subActionMode() == Sequential) {

        setStarted();
        internalDoNextAction();
	}
	// parallel mode
	else {
		setStarted();
        foreach(AMAction3* action, subActions_) {
			internalConnectAction(action);
		}
        foreach(AMAction3* action, subActions_) {
			action->start();
		}
	}
}
void AMListAction3::pauseImplementation()
{
	// if this was called by the base class, we know that we are in the Pausing state, and were in the Running state. We also know that canPause() is true.

	// sequential mode:
	////////////////////////
    if(subActionMode() == Sequential) {
		if(currentSubAction()) {
            // Can this action pause?  Required to be asked since the list can always pause, but the current action might not be able to.
            if(currentSubAction()->canPause())
                currentSubAction()->pause();
			// If it can't, we'll pause by stopping at the next action. When the current action transitions to Succeeded, we simply won't start the next one and will notifyPaused(). In this case, we could stay at Pausing for quite some time until the current action finishes.
		}
		else {
			qWarning() << "AMListAction: Warning: pauseImplementation() was called at an unexpected time. No action is running.";
		}
		// when the action changes to the Paused state, we'll pick that up in internalOnSubActionStateChanged() and notifyPaused().
	}

	// parallel mode:
	/////////////////////
	else {
		// because we can trust that canPause() was true for all, let's just tell all of our actions to pause.
        foreach(AMAction3* action, subActions_) {
            action->pause();	// this will fail for completed actions, but that's OK.  Addition DH: You should never make it into this function if you're done.
		}
		// when ALL of the still-running actions change to the Paused state, we'll pick that up in internalOnSubActionStateChanged() and notifyPaused().
	}
}

void AMListAction3::resumeImplementation()
{
	// If this is called by the base class, we know that we're now in Resuming and used to be in Pause.
    if(subActionMode() == Sequential) {
		// The currentSubAction() will either be Paused (if it supported pausing when we were paused), or Constructed (if the last action didn't support pausing and completed; now we're at the beginning of the next one).
		if(currentSubAction()) {
			if(currentSubAction()->state() == Paused)
				currentSubAction()->resume();
			else if(currentSubAction()->state() == Constructed) {
				internalConnectAction(currentSubAction());
				currentSubAction()->start();
			}
			else
				qWarning() << "AMListAction: Warning: Asked to resume, but the current action is not paused.";
		}
		else {
			qWarning() << "AMListAction: Warning: Asked to resume unexpectedly: there is no current action to resume.";
		}
	}
	// Parallel mode:
	////////////////////////
	else {
		// we know that we were in Paused (aka: all actions finished pausing), and that canPause() was true, so we can tell them all to resume.
        foreach(AMAction3* action, subActions_) {
			if(!action->inFinalState())
				action->resume();
		}
	}
}

void AMListAction3::cancelImplementation()
{
	// sequential mode
    if(subActionMode() == Sequential) {
		if(currentSubAction()) {
            if(currentSubAction()->state() == Constructed)	// this sub-action not connected or run yet. Don't need to cancel it. (This could happen if we are paused between actions and currentSubAction() hasn't been started yet.)
                setCancelled();
            else
                currentSubAction()->cancel(); // action is already connected and running. Need to cancel it. We'll pick it up in internalOnSubActionStateChanged
		}
		else {
			setCancelled();
		}
	}
	// Parallel mode:
	else {
		// this won't be called unless startImplementation() has already been given. In that case, all actions have been connected and started. Try to cancel all the ones that aren't finished already.
        foreach(AMAction3* action, subActions_)
			if(!action->inFinalState())
				action->cancel();
	}
}


void AMListAction3::internalOnSubActionStateChanged(int newState, int oldState)
{
	Q_UNUSED(oldState)

	// sequential mode: could only come from the current action
    if(subActionMode() == Sequential) {

		switch(newState) {
        case Starting:
			// If we were paused between actions and resuming, the next action is now running...
			if(state() == Resuming)
				setResumed();
			return;
		case Running:
			// If we had a current action paused:
			if(state() == Resuming)
				setResumed();
			return;
		case Pausing:
			return;
		case Paused:
			// the current action paused, so now we're paused. This will only happen if the current action supports pause and transitioned to it.
			if(state() == Pausing) {
				setPaused();
			}
			else {
				qWarning() << "AMListAction: Warning: A sub-action was paused without cancelling its parent list action. This should not happen.";
			}
			return;
		case Resuming:
			return;
		case Cancelling:
			return;
		case Cancelled:
			if(state() == Cancelling) {
                internalCleanupAction();
				setCancelled();
			}
			else {
				qWarning() << "AMListAction: Warning: A sub-action was cancelled without cancelling its parent list action. This should not happen.";
			}
			return;
		case Succeeded:
            internalDoNextAction();
            return;
		case Failed:
            internalCleanupAction();
            setFailed();
			return;
		}
	}

	// parallel mode:
	/////////////////////////
	else {
		switch(newState) {
		case Starting:
			return;
		case Running:
			if(state() == Resuming) {
				if(internalAllActionsRunningOrFinal())
					setResumed();
			}
			return;
		case Pausing:
			return;
		case Paused:
			if(state() == Pausing) {
				// one of them paused. Are all the actions paused now?
				if(internalAllActionsPausedOrFinal())
					setPaused();
			}
			else
				qWarning() << "AMListAction: Warning: A sub-action was paused without pausing its parent list action. This should not happen.";
			return;
		case Resuming:
			return;
		case Cancelling:
			return;
		case Cancelled:
			if(state() != Cancelling)
				qWarning() << "AMListAction: Warning: A sub-action was cancelled with cancelling its parent list action. This should not happen.";
			// Continue on to common handling with Succeeded and Failed:
		case Succeeded:
		case Failed:
			// only do something here if all actions are done.
			if(internalAllActionsInFinalState()) {
				// disconnect all actions
                foreach(AMAction3* action, subActions_)
                    internalCleanupAction(action);
				// Any failures?
				if(internalAnyActionsFailed())
					setFailed();
				// Any cancelled?
				else if(internalAnyActionsCancelled())
					setCancelled();
				else	 // well, I guess they're all good then. We're done!
					setSucceeded();
			}
			return;
		}
	}
}

void AMListAction3::internalDoNextAction()
{
    // in sequential mode, always move on to the next action here.
    if (currentSubActionIndex_ >= 0)
        internalCleanupAction();

    // Are we done the last action? If we are, we're done no matter what [even if supposed to be pausing]
    if(currentSubActionIndex_ < subActionCount()-1) {

        emit currentSubActionChanged(++currentSubActionIndex_);
        internalConnectAction(currentSubAction());

        if (state() == Pausing)
            setPaused();

        else if (state() == Running)
            currentSubAction()->start();
    }

    else
        setSucceeded();
}

void AMListAction3::internalOnSubActionProgressChanged(double numerator, double denominator)
{
	// sequential mode:
    if(subActionMode() == Sequential) {
		// If all still-running actions specify an expected duration, then we use this to specify the relative size of each action's "work chunk". The denominator of our progress will be in total (expected) seconds for all actions to complete.
		if(internalAllActionsHaveExpectedDuration()) {
			double totalNumerator = 0, totalDenominator = 0;
			for(int i=0, cc=subActionCount(); i<cc; i++) {
                AMAction3* action = subActionAt(i);

				// completed action? Add total run time to both denominator and numerator.
				if(action->inFinalState()) {
					double runTimeSeconds = action->startDateTime().secsTo(action->endDateTime());
					totalDenominator += runTimeSeconds;
					totalNumerator += runTimeSeconds;
				}
				// incomplete action.
				else {
					double expectedDuration = action->info()->expectedDuration();
					totalDenominator += expectedDuration;
					if(i == currentSubActionIndex())	// if it's the current action, we have some partial progress.
						totalNumerator += numerator/denominator * expectedDuration;
				}
			}
			setProgress(totalNumerator, totalDenominator);
            setExpectedDuration(totalDenominator);
		}
		// Otherwise, assume every subaction makes up an equal fraction of the total amount of work. Our denominator will be the number of sub-actions, and our numerator will be the sub-action we are on (plus its fraction done). For ex: with 8 sub-actions: At the beginning, this will be 0/8.  Half-way through the first action it will be (0+0.5)/8.  At the very end it will be (7+1)/8.
		else {
			setProgress(currentSubActionIndex() + numerator/denominator, subActionCount());
		}
	}
	// parallel mode:
	else {
		// if they all have an expected duration, try to return total progress in secondsCompleted of secondsTotal.
		if(internalAllActionsHaveExpectedDuration()) {
			double totalNumerator = 0, totalDenominator = 0;
			for(int i=0, cc=subActionCount(); i<cc; i++) {
                AMAction3* action = subActionAt(i);

				// completed action? Add total run time to both denominator and numerator.
				if(action->inFinalState()) {
					double runTimeSeconds = action->startDateTime().secsTo(action->endDateTime());
					totalDenominator += runTimeSeconds;
					totalNumerator += runTimeSeconds;
				}
				// incomplete action.
				else {
					double expectedDuration = action->info()->expectedDuration();
					totalDenominator += expectedDuration;
					totalNumerator += action->startDateTime().secsTo(action->endDateTime());
				}
			}
			setProgress(totalNumerator/subActionCount(), totalDenominator/subActionCount());
		}
		else {
			double totalNumerator = 0, totalDenominator = 0;
            foreach(AMAction3* action, subActions_) {
				QPair<double, double> p = action->progress();
				// Will return (0,0) if unknown, or (-1,-1) if not yet started.
				if(p.second == 0) {
					setProgress(0,0);
					return;
				}
				if(p.first < 0 && p.second < 0) { // hasnt started yet
					totalDenominator += 1;
				}
				else {
					totalNumerator += p.first;
					totalDenominator += p.second;
				}
			}
			setProgress(totalNumerator, totalDenominator);
		}
	}
}

void AMListAction3::internalOnSubActionStatusTextChanged(const QString &statusText)
{
	// sequential mode:
    if(subActionMode() == Sequential) {
		setStatusText("Step " % QString::number(currentSubActionIndex()+1) % " (" % currentSubAction()->info()->shortDescription() % "): " % statusText);
	}
	// parallel mode:
	else {
		QStringList allStatuses;
		for(int i=0, cc=subActionCount(); i<cc; i++)
			allStatuses << "Step " % QString::number(i+1) % " (" % subActionAt(i)->info()->shortDescription() % "): " % subActionAt(i)->statusText();

		setStatusText(allStatuses.join("\n"));
	}
}

void AMListAction3::internalConnectAction(AMAction3 *action)
{
	connect(action, SIGNAL(stateChanged(int,int)), this, SLOT(internalOnSubActionStateChanged(int,int)));
	connect(action, SIGNAL(progressChanged(double,double)), this, SLOT(internalOnSubActionProgressChanged(double,double)));
	connect(action, SIGNAL(statusTextChanged(QString)), this, SLOT(internalOnSubActionStatusTextChanged(QString)));
}

void AMListAction3::internalDisconnectAction(AMAction3 *action)
{
	disconnect(action, SIGNAL(stateChanged(int,int)), this, SLOT(internalOnSubActionStateChanged(int,int)));
	disconnect(action, SIGNAL(progressChanged(double,double)), this, SLOT(internalOnSubActionProgressChanged(double,double)));
	disconnect(action, SIGNAL(statusTextChanged(QString)), this, SLOT(internalOnSubActionStatusTextChanged(QString)));
}

void AMListAction3::internalCleanupAction(AMAction3 *action)
{
    AMAction3 *cleanupAction = action ? action : currentSubAction();

    internalDisconnectAction(cleanupAction);
    if (internalShouldLogSubAction(cleanupAction))
        AMActionLog3::logCompletedAction(cleanupAction);
}

bool AMListAction3::internalAllActionsRunningOrFinal() const
{
	bool rv = true;
    foreach(AMAction3* action, subActions_)
		rv &= (action->state() == Running || action->inFinalState());
	return rv;
}

bool AMListAction3::internalAllActionsPausedOrFinal() const
{
	bool rv = true;
    foreach(AMAction3* action, subActions_)
		rv &= (action->state() == Paused || action->inFinalState());
	return rv;
}

bool AMListAction3::internalAllActionsInFinalState() const
{
	bool rv = true;
    foreach(AMAction3* action, subActions_)
		rv &= action->inFinalState();
	return rv;
}

bool AMListAction3::internalAnyActionsFailed() const
{
    foreach(AMAction3* action, subActions_)
		if(action->state() == Failed)
			return true;
	return false;
}

bool AMListAction3::internalAnyActionsCancelled() const
{
    foreach(AMAction3* action, subActions_)
		if(action->state() == Cancelled)
			return true;
	return false;
}

bool AMListAction3::internalAllActionsHaveExpectedDuration() const
{
    foreach(AMAction3* action, subActions_)
		if(action->info()->expectedDuration() < 0)
			return false;
	return true;
}

bool AMListAction3::internalShouldLogSubAction(AMAction3 *action)
{
    AMListAction3* nestedAction = qobject_cast<AMListAction3*>(action);
    return shouldLogSubActionsSeparately() && !(nestedAction && nestedAction->shouldLogSubActionsSeparately());
}
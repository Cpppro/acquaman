/*
Copyright 2010-2012 Mark Boots, David Chevrier, and Darren Hunter.

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


#ifndef AMACTION_H
#define AMACTION_H

#include <QObject>
#include "actions2/AMActionInfo.h"

/// You can subclass this to define a prerequisite for AMActions: a condition that must be satisfied before the action can start. It should return satisfied() and have a signal satisfiedChanged() to notify whenever that changes.
class AMActionPrereq : public QObject {
	Q_OBJECT
public:
	/// Returns whether this prereq is satisfied
	virtual bool satisfied() const = 0;
	/// Returns an explanation that will make sense to users when we tell them why we can't proceed with their action
	virtual QString explanation() const = 0;
	/// This function is used as a virtual copy constructor. It should return an independent copy of the prereq instance.
	virtual AMActionPrereq* createCopy() const = 0;

signals:
	void satisfiedChanged(bool isSatisfied);
};

class AMNestedAction;

/// AMAction defines the interface for "actions" which can be run asynchronously in Acquaman's Workflow system. Actions (especially AMListAction) can also be useful on their own, outside of the workflow environment, to (a) make a set of events happen in a defined order, and (b) receive notification when those events succeed or fail. They can be used to easily build up complicated behaviour when moving beamlines around or building scan controllers.
/*!
  <b>Running actions</b>

Actions can be run on their own (by simply calling start()), or within the workflow system by using AMActionRunner.

When using AMActionRunner, they can be queued up to run one-by-one with AMActionRunner::addQueuedAction(), or run immediately (in parallel with whatever else might be running) with AMActionRunner::runActionImmediately().  In both cases, the actions are logged in the database after running.

<b>Actions and States</b>
\todo detailed docs

<b>Actions and AMActionInfo</b>

Most of the time, an AMAction subclass has a corresponding AMActionInfo subclass that describes the content/details/specs of the action, but omits all the implementation code required to actually run the action. (Users should typically configure/set up actions by <i>configuring the info</i> inside a corresponding AMActionInfoView.)  This separation allows users to review/copy/manage the AMActionInfos from their completed actions inside Acquaman programs that do not (or should not) have access to the implementation code.  For example, a user might want to review some scan actions done on Beamline A, while working on Beamline B; the user could open and review a BeamlineAScanActionInfo even though their program doesn't have access to BeamlineAScanAction.  (A more typical example might be reviewing a log of completed actions from many beamlines inside the Dataman take-home program.)

<b><i>Action/Info Registry</i></b>

When this one-to-one relationship between AMActions and AMActionInfos is used, the programmer should register the pair in the AMAction/Info Registry (AMActionRegistry::registerInfoAndAction()). This allows the workflow system to recreate valid AMActions given a particular AMActionInfo -- allowing a user to re-instantiate and re-run actions from their history of stored AMActionInfos.

<b>AMActionInfo, views, and editors</b>

\todo detailed description

<b>Prerequisites</b>

AMActionPrereq defines an interface for "prerequisites" which must be satisfied before an action can run.  One example of this (for a scan action) could be to ensure that no other scans are currently running on the beamline.

When you add one or more prereqs to an action with addPrereq(), the action can do one of four things (depending on the chosen prereqBehavior()) if any of them are not satisfied:

- Wait patiently for all the prerequisites to be satisfied
- Automatically cancel the action (like calling cancel())
- Automatically fail the action  (like the action going into the Failed state)
- Ask the user for whether to wait or cancel the action


 */
class AMAction : public QObject
{
	Q_OBJECT
public:
	// enum types defined
	/////////////////////////

	/// This enum describes the states that an action can be in.
	enum State { Constructed = 0,
				 WaitingForPrereqs,
				 Starting,
				 Running,
				 Pausing,
				 Paused,
				 Resuming,
				 Cancelling,
				 Cancelled,
				 Succeeded,
				 Failed
			   };
	/// When running inside AMActionRunner, or as a sub-action in AMListAction, this enum specifies what to do if the action fails
	enum FailureResponse { MoveOnResponse = 0,
						   AttemptAnotherCopyResponse,
						   PromptUserResponse
						 };
	/// When there are pre-requisites for the action, this specifies what to do if any pre-reqs aren't satisfied
	enum PrereqBehaviour { WaitPrereqBehaviour = 0,
						   CancelActionPrereqBehaviour,
						   FailActionPrereqBehaviour,
						   PromptUserPrereqBehaviour
						 };



	// Constructor and life-cycle management
	/////////////////

	/// Constructor: create an action to run the specified AMActionInfo.
	AMAction(AMActionInfo* info, QObject *parent = 0);

	/// Copy constructor. Takes care of making copies of the info and prerequisites. NOTE that the state() is reset to Constructed: the copy should look like a new action that hasn't been run yet.
	AMAction(const AMAction& other);

	/// This virtual function takes the role of a virtual copy constructor. All actions MUST RE-IMPLEMENT to be able to create and return a an independent copy of themselves. (This allows us to get a detailed subclass copy without knowing the type of the action.) NOTE that the returned instance should be a perfect copy except for the state() -- which will be reset to Constructed -- and any other subclass-specific state information: the copy should look like a new action that hasn't been run yet.
	/*! It's recommended to make use of the copy constructor when implementing this, to ensure that the base class is copied properly.*/
	virtual AMAction* createCopy() const { return new AMAction(*this); }

	/// Destructor: deletes the info and prerequisites
	virtual ~AMAction();


	// Info API
	//////////////////////

	/// Returns the AMActionInfo subclass describing this action. The Info subclasses are a way to separate the description/content of actions from their implementation. This allows storing and viewing future/past actions inside programs which shouldn't have the capability to run them
	/*! (For example: a user viewing a history of their SGM beamline actions inside their take-home Dataman program, or when on another beamline. Theoretically, this approach could even let someone plan and queue up beamline actions at home to run once they get to the beamline.)

You can use a generic AMActionInfo in an AMAction-subclass constructor, but if you want to be able to re-create live actions from historical ones, you should provide a unique info subclass with sufficient information to completely specify/re-create the action, and register your Info-Action pair with AMActionRegistry::registerInfoAndAction().*/
	const AMActionInfo* info() const { return info_; }
	AMActionInfo* info() { return info_; }

	// Action Timing: start time, end time, cumulative elapsed time, etc.  All of this is managed by the base class for you.
	////////////////////////
	/// Returns the date and time when this action was started, or an invalid QDateTime if it hasn't started yet
	QDateTime startDateTime() const { return startDateTime_; }
	/// Returns the date and time when this action finished, or an invalid QDateTime if it hasn't finished yet.
	QDateTime endDateTime() const { return endDateTime_; }
	/// Returns the total number of seconds since this action was start()ed, or -1 if it hasn't started running yet. As long as the action has started, this will be equivalent to QDateTime::currentDateTime() - startDateTime().
	/*! \note This will include all the time spent in the Paused state as well as WaitingForPrereqs. To get just the actual running time, \see runningTime(). */
	double elapsedTime() const { if(!startDateTime_.isValid()) return -1.0; return double(startDateTime_.msecsTo(QDateTime::currentDateTime()))/1000.0; }
	/// Returns the number of seconds that this action has been in the Paused and Pausing states for.
	double pausedTime() const;
	/// Returns the number of seconds that this action has been in the WaitingForPrereqs state for
	double waitingForPrereqsTime() const;
	/// Returns the number of seconds that this action has actually been running for, <i>excluding</i> time spent in the Paused (and Pausing) and WaitingForPrereqs states.
	double runningTime() const { if(!startDateTime_.isValid()) return -1.0; return elapsedTime() - pausedTime() - waitingForPrereqsTime(); }

	/// Returns a description of the action's status, for example, "Waiting for the beamline energy to reach the setpoint".  Implementations should update this while the action is running using setStatusText().
	/*! Even if you don't ever call this, the base class implementation will at at least call it on every state change with the name of the state. (For example: when the state changes to running, the status text will change to "Running".) You can always call it again after the state change/in-between state changes to provide more details to the user.*/
	QString statusText() const { return statusText_; }


	// States
	///////////////////

	/// Returns the current state of the action
	State state() const { return state_; }
	/// Returns a string describing the given \c state
	QString stateDescription(State state);
	/// Returns whether the action is in a final state (Succeeded, Failed, or Cancelled). All cleanup should be done before entering these states, so it should be OK to delete an action once it is in a final state.
	bool inFinalState() const { return state_ == Succeeded || state_ == Failed || state_ == Cancelled; }
	/// Returns the state the action was in before the current state
	State previousState() const { return previousState_; }

	/// This virtual function can be re-implemented to specify whether the action has the capability to pause. By default, it returns false (ie: cannot pause).
	virtual bool canPause() const { return false; }


	// Nesting actions
	///////////////////////

	/// Sometimes, actions are formally bundled inside other actions using the AMNestedAction API. If this action happens to be found inside a nested action, this returns a pointer to the nested action. Normally, it returns 0.
	const AMNestedAction* parentAction() const { return parentAction_; }
	/// Sometimes, actions are formally bundled inside other actions using the AMNestedAction API. If this action happens to be found inside a nested action, this returns a pointer to the nested action. Normally, it returns 0.
	AMNestedAction* parentAction() { return parentAction_; }
	/// This function should not be considered part of the public interface. It is used by nested actions to set the parent action. NEVER CALL THIS FUNCTION.
	void internalSetParentAction(AMNestedAction* parentAction) { parentAction_ = parentAction; }


public slots:
	// External requests to change the state: start(), cancel(), pause(), and resume().
	//////////////////
	// All of these requests to change the state return false if not allowed from the current state.

	/// Start running the action. Allowed from Constructed. State will change to WaitingForPrereqs or Starting.
	bool start();

	/// Explicitly cancel the action. Allowed from anything except Cancelling, Succeeded, or Failed. The state will change Cancelling. The action could take a while to finish cancelling itself; the state will change to Cancelled when that finally happens.
	bool cancel();

	/// For actions that support pausing, request to pause the action. Allowed from WaitingForPrereqs or Running, if canPause() is true. The state will change to Paused or Pausing (respectively).
	bool pause();

	/// For actions that support pausing, request to pause the action. Allowed from Paused only. The state will change to Resuming.
	bool resume();

public:
	// Progress API
	////////////////////////

	/// Returns the numerator and denominator of the progress. (ie: completed steps out of total steps). If the progress is unknown, this will return (0,0). If the action hasn't started yet, this will return (-1, -1).
	QPair<double, double> progress() const { return progress_; }
	/// Returns the total expected duration in seconds. If the time to completion is unknown, this will return -1.
	double expectedDuration() const { return info_->expectedDuration(); }


	// Prerequisite API
	/////////////////////////

	/// Returns the current list of prerequisites
	QList<AMActionPrereq*> prereqs() const { return prereqs_; }
	/// Returns the prereq behaviour: what to do when a prereq is not satisfied. The options are to wait for the prereq to be satisfied, cancel the action, fail the action, or prompt the user for what to do.
	PrereqBehaviour prereqBehaviour() const { return prereqBehaviour_; }

	/// Add a new prerequisite for this action. Prereqs (defined by a subclass of AMActionPrereq) specify whether the action can run right now or not. The action takes ownership of the prereq and will delete it when deleted. Returns false if the action is already running and it would be meaningless to add a prereq.
	bool addPrereq(AMActionPrereq* newPrereq);
	/// Specify what should be done in the event that a prereq is not satisfied. The options are to wait for the prereq to be satisfied, cancel the action, fail the action, or prompt the user for what to do. In the future, we might add an attempt to fix the situation causing the prereq.  The default is to wait for the prereq to be satistifed.
	/*! This function returns false if the action is already running and it would be meaningless to set the prereq behaviour.

   \note The difference between cancelling and failing the action has to do with the failure response... When cancelled, the failure response isn't invoked; when the action fails, it is.
*/
	bool setPrereqBehaviour(PrereqBehaviour prereqBehaviour);

	/// Returns true if all the prerequisites are satisfied; false if any aren't.
	bool prereqsSatisfied() const;


	// Recommended Response to failure:
	//////////////////////////

	/// When this action is run in a workflow using AMActionRunner, this specifies the recommended response if the action fails (ie: state() changes to Failed, due to some internal reason. Note that this is distinct from intentionally cancelled.) The possibilities are to give up and move on, or attempt to run the action again by generating a copy of it and running the copy -- before going on to any further actions.  As a third option, the user can be prompted to choose one of these options.  The default is to give up and move on.
	FailureResponse failureResponseInActionRunner() const { return failureResponseInActionRunner_; }
	/// When this action is run as a sub-action of another, this specifies the recommended response if the action fails (ie: state() changes to Failed, due to some internal reason. Note that this is distinct from intentionally cancelled.) The possibilities are to give up and move on, or to attempt to run the action again by generating a copy of it and running the copy -- before going on to any further actions.  As a third option, the user can be prompted to choose one of these options.  The default is to give up and move on.
	FailureResponse failureResponseAsSubAction() const { return failureResponseAsSubAction_; }

	/// Set the failure response that we would recommend when this action is run in a workflow using AMActionRunner.
	void setFailureResponseInActionRunner(FailureResponse fr) { failureResponseInActionRunner_ = fr; }
	/// Set the failure response that we would recommend when this action is run as a sub-action of another action.
	void setFailureResponseAsSubAction(FailureResponse fr) { failureResponseAsSubAction_ = fr; }

signals:

	/// Emitted whenever the state() of the action changes
	void stateChanged(int newActionState, int previousActionState);

	// convenience synonyms for state changed to a final state.
	//////////////////////////
	/// Emitted when the state changes to Succeeded
	void succeeded();
	/// Emitted when the state changes to Failed
	void failed();
	/// Emitted when the state changes to Cancelled
	void cancelled();


	/// Emitted when the progress changes. (\c numerator gives the amount done, relative to the total expected amount \c denominator. For example, \c numerator could be a percentage value, and \c denominator could be 100.)
	void progressChanged(double numerator, double denominator);
	/// If the action knows how long it will take, this signal is emitted with the total expected run time. If it doesn't know how long things will take, it could be emitted with (-1).
	void expectedDurationChanged(double expectedTotalDurationInSeconds);

	/// Emitted when the statusText() changes.
	void statusTextChanged(const QString& statusText);



protected slots:
	/// Implementations should call this to notify of their progress.   \c numerator gives the amount done, relative to the total expected amount \c denominator. For example, \c numerator could be a percentage value, and \c denominator could be 100.  If you don't know the level of progress, call this with (0,0).
	void setProgress(double numerator, double denominator) { progress_ = QPair<double,double>(numerator,denominator); emit progressChanged(numerator, denominator); }
	/// Implementations should call this to notify of the expected total duration (in seconds) of the action. If you don't know how long the action will take, call this with -1.
	void setExpectedDuration(double expectedTotalTimeInSeconds) { info_->setExpectedDuration(expectedTotalTimeInSeconds); }
	/// Implementations should call this to describe the action's status while running; for example, "Moving motor X to 30.4mm"
	void setStatusText(const QString& statusText) { emit statusTextChanged(statusText_ = statusText); }

protected:

	// The following functions are used to define the unique behaviour of the action.  We set them up in this way so that subclasses don't need to worry about (and cannot) break the state machine logic; they only need to fill in their pieces.

	// These virtual functions allow subclasses to implement their unique action behaviour.  They are called at the appropriate time by the base class, when base-class-initiated state changes happen: ->Starting, ->Cancelling, ->Pausing, ->Resuming
	/////////////////////////
	/// This function is called from the Starting state when the implementation should initiate the action. Once the action is started, you should call notifyStarted().
	virtual void startImplementation() { notifyStarted(); }

	/// For actions which support pausing, this function is called from the Pausing state when the implementation should pause the action. Once the action is paused, you should call notifyPaused().  The base class implementation does nothing and must be re-implemented.
	virtual void pauseImplementation() { notifyPaused(); }

	/// For actions that support resuming, this function is called from the Paused state when the implementation should resume the action. Once the action is running again, you should call notifyResumed().
	virtual void resumeImplementation() { notifyResumed(); }

	/// All implementations must support cancelling. This function will be called from the Cancelling state. Implementations will probably want to examine the previousState(), which could be any of Starting, Running, Pausing, Paused, or Resuming. Once the action is cancelled and can be deleted, you should call notifyCancelled().
	/*! \note If startImplementation() was never called, you won't receive this when a user tries to cancel(); the base class will handle it for you. */
	virtual void cancelImplementation() { notifyCancelled(); }



	// These functions should be called _by the implementation_ notify the base class when implementation-initiated state changes happen: Starting->Running, Running->Succeeded, Anything->Failed, Pausing->Paused, Cancelling->Cancelled.
	///////////////////////

	/// Call this after receiving startImplementation() to inform the base class that the action has started, and we should go from Starting to Running.
	void notifyStarted();
	/// Call this to inform the base class that the action has succeeded. It should be OK to delete the action after receiving this.
	void notifySucceeded();
	/// Call this to inform the base class that the action has failed. It should be OK to delete the action after receiving this.
	void notifyFailed();
	/// Call this after receiving pauseImplementation() to inform the base class that the action has been paused, and we should go from Pausing to Paused.
	void notifyPaused();
	/// Call this after receiving resumeImplementation() to inform the base class that the action has been resumed, and we should go from Resuming to Running.
	void notifyResumed();
	/// Call this after receiving cancelImplementation() to inform the base class that the action has been cancelled, and we should go from Cancelling to Cancelled.
	void notifyCancelled();

private:
	State state_, previousState_;
	void setState(State newState);

	QList<AMActionPrereq*> prereqs_;
	PrereqBehaviour prereqBehaviour_;

	FailureResponse failureResponseInActionRunner_;
	FailureResponse failureResponseAsSubAction_;

	QPair<double,double> progress_;
	QDateTime startDateTime_, endDateTime_;
	QString statusText_;

	AMNestedAction* parentAction_;

private slots:
	// These slots are used to respond to events as the state machine runs.
	///////////////////
	void internalOnPrereqChanged();

private:
	/// Helper function to prompt the user for whether to cancel the action or wait for the prereqs to be satisfied
	PrereqBehaviour promptUserForPrereqBehaviour();

private:
	/// A pointer to our associated AMActionInfo object
	AMActionInfo* info_;
	/// This variable tracks the number of seconds that the action has spent in the Paused or Pausing states; we use it to implement runningTime().
	/*! \note It is only updated _after_ the action has resume()d.*/
	double secondsSpentPaused_;
	/// This variable tracks the number of seconds that the action spent in the WaitingForPrereqs state; we use it to implement runningTime(). It is not valid until we get past that state.
	double secondsSpentWaitingForPrereqs_;
	/// This variable stores the time at which we were last paused. It is set in pause().
	QDateTime lastPausedAt_;
};

#endif // AMACTION_H

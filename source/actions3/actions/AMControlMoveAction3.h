#ifndef AMCONTROLMOVEACTION3_H
#define AMCONTROLMOVEACTION3_H

#include "actions3/AMAction3.h"
#include "actions3/actions/AMControlMoveActionInfo3.h"

#include <QTimer>

class AMControl;

/// This implementation of AMAction takes care of moving an AMControl into position.  Essentially, it wraps AMControl for use with the Action/Workflow system.
class AMControlMoveAction3 : public AMAction3
{
	Q_OBJECT
public:
	// Constructors and copying
	//////////////////////////////

	/// Constructor. Requires and takes ownership of an existing AMControlMoveActionInfo \c info.  Provides a AMControl \param control that will be controlled.  If the default is used instead, then a lookup based on AMBeamline::exposedControls will be used instead.
	Q_INVOKABLE AMControlMoveAction3(AMControlMoveActionInfo3* info, AMControl *control = 0, QObject *parent = 0);
	/// Copy constructor: must re-implement, but can simply use the AMAction copy constructor to make copies of the info and prereqs. We need to reset any internal state variables to make the copy a "like new" action - ie, not run yet.
	AMControlMoveAction3(const AMControlMoveAction3& other);
	/// Virtual copy constructor
	virtual AMAction3* createCopy() const { return new AMControlMoveAction3(*this); }


	/// Returns the control that this action will move.
	AMControl *control() const { return control_; }
	/// Sets the control used by this action.  This will generally not be used because the control will be provided by the constructor.
	void setControl(AMControl *control) { control_ = control; }

	// Re-implemented public functions
	///////////////////////////////

	/// Specify that we cannot pause (since AMControl cannot pause).  If we wanted to get fancy, we might check if the control could stop, (and stop it for pause, and then start it again to resume). But this is much simpler for now.
	virtual bool canPause() const { return false; }

	/// Virtual function that denotes that this action has children underneath it or not.
	virtual bool hasChildren() const { return false; }
	/// Virtual function that returns the number of children for this action.
	virtual int numberOfChildren() const { return 0; }

signals:

public slots:

protected:

	// The following functions are used to define the unique behaviour of the action.  We set them up in this way so that subclasses don't need to worry about (and cannot) break the state machine logic; they only need to fill in their pieces.

	// These virtual functions allow us to implement our unique action behaviour.  They are called at the appropriate time by the base class, when base-class-initiated state changes happen: ->Starting, ->Cancelling, ->Pausing, ->Resuming
	/////////////////////////
	/// This function is called from the Starting state when the implementation should initiate the action. Once the action is started, you should call notifyStarted().
	virtual void startImplementation();

	/// For actions which support pausing, this function is called from the Pausing state when the implementation should pause the action. Once the action is paused, you should call notifyPaused().  The base class implementation does nothing and must be re-implemented.
	virtual void pauseImplementation() { setPaused(); }

	/// For actions that support resuming, this function is called from the Paused state when the implementation should resume the action. Once the action is running again, you should call notifyResumed().
	virtual void resumeImplementation() { setResumed(); }

	/// All implementations must support cancelling. This function will be called from the Cancelling state. Implementations will probably want to examine the previousState(), which could be any of Starting, Running, Pausing, Paused, or Resuming. Once the action is cancelled and can be deleted, you should call notifyCancelled().
	/*! \note If startImplementation() was never called, you won't receive this when a user tries to cancel(); the base class will handle it for you. */
	virtual void cancelImplementation();

protected slots:
	/// Every second, we emit a progress update with setProgress()
	void onProgressTick();

	/// Handle signals from our control:
	void onMoveStarted();
	void onMoveFailed(int reason);
	void onMoveSucceeded();

protected:

	/// We can always access our info object via info_ or info(), but it will come back as a AMActionInfo* pointer that we would need to cast to AMControlMoveActionInfo. This makes it easier to access.
	const AMControlMoveActionInfo3* controlMoveInfo() const { return qobject_cast<const AMControlMoveActionInfo3*>(info()); }
	/// We can always access our info object via info_ or info(), but it will come back as a AMActionInfo* pointer that we would need to cast to AMControlMoveActionInfo. This makes it easier to access.
	AMControlMoveActionInfo3* controlMoveInfo() { return qobject_cast<AMControlMoveActionInfo3*>(info()); }

	// Internal variables:

	/// Timer used to issue progress updates on a per-second basis
	QTimer progressTick_;

	/// A pointer to the AMControl we use to implement the action
	AMControl* control_;
	/// Stores the start position, which we use for calculating progress
	AMControlInfo startPosition_;

};

#endif // AMCONTROLMOVEACTION3_H
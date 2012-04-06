#ifndef AMCONTROLMOVEACTIONINFO3_H
#define AMCONTROLMOVEACTIONINFO3_H

#include "actions3/AMActionInfo3.h"
#include "dataman/info/AMControlInfoList.h"

/// This AMActionInfo-subclass specifies the information for AMControlMoveAction -- an action that moves a control to a setpoint.  This info specifies the setpoint (the name of the control and where to move it to), in the form of an AMControlInfo.
class AMControlMoveActionInfo3 : public AMActionInfo3
{
    Q_OBJECT
	Q_PROPERTY(AMDbObject* controlInfo READ dbReadControlInfo WRITE dbLoadControlInfo)
	Q_PROPERTY(bool isRelativeMove READ isRelativeMove WRITE setIsRelativeMove)

public:
	/// Constructor. You should always specify a valid \c setpoint, but we provide the default argument because we need a default constructor for database loading.
	Q_INVOKABLE AMControlMoveActionInfo3(const AMControlInfo &setpoint = AMControlInfo(), QObject *parent = 0);

	/// Copy Constructor
	AMControlMoveActionInfo3(const AMControlMoveActionInfo3 &other);

	/// This function is used as a virtual copy constructor
	virtual AMActionInfo3* createCopy() const { return new AMControlMoveActionInfo3(*this); }

	// Re-implemented public functions
	/////////////////////////////////

	/// This should describe the type of the action
	virtual QString typeDescription() const { return "Control Move"; }

	// New public functions
	//////////////////////////

	/// Returns a pointer to our move destination setpoint
	const AMControlInfo* controlInfo() const { return &controlInfo_; }
	/// Returns true if this is to be a relative move (otherwise returns false for absolute).
	bool isRelativeMove() const { return isRelative_; }

	/// Set the move destination setpoint, including the control name, value, and description.
	/*! \note We make a copy of \c controlInfo's values, and do not retain any reference to it afterward. */
	void setControlInfo(const AMControlInfo& controlInfo);
	/// Set the move destination setpoint (value only).
	void setSetpoint(double setpoint);
	/// Sets whether this should be a relative (rather than absolute) move.  Absolute is the default.
	void setIsRelativeMove(bool isRelative);

	// Database loading/storing
	////////////////////////////

	/// For database storing only.
	AMControlInfo* dbReadControlInfo() { return &controlInfo_; }
	/// For database loading only. This function will never be called since dbReadControlInfo() always returns a valid setpoint, but it needs to be here.
	void dbLoadControlInfo(AMDbObject* newLoadedObject) { delete newLoadedObject; }

signals:

public slots:

protected:
	/// The AMControlInfo that specifies where to move to
	AMControlInfo controlInfo_;
	/// A flag to indicate that this should be a relative (rather than absolute) move
	bool isRelative_;

	/// A short helper function to update the action's description
	void updateDescriptionText();

};

#endif // AMCONTROLMOVEACTIONINFO3_H
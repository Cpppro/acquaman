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


#ifndef ACQMAN_BEAMLINE_H_
#define ACQMAN_BEAMLINE_H_

#include "beamline/AMControl.h"

/// One good way for components in the Acquaman framework to access and set a variety of beamline controls is through a centralized AMBeamline object.  This class provides the basic functionality expected of every beamline, and can be subclassed to include the specific controls available on a particular machine.  It uses the singleton design pattern to ensure that only a single instance of the beamline object exists; you can access this object through AMBeamline::bl().

/*! <b>Note on subclassing and inheritance</b>
  Singleton classes are designed so that only a single instance of the object can be created.  This is accomplished by making the constructor private (or protected), and always accessing the object through a static member function (in this case, AMBeamline::bl()).

  This static function first checks to see if an instance of the object has already been created. If it hasn't, it creates a new instance, stores it in the static instance_ variable, and then returns it. In other cases, bl() simply returns the existing instance.

  When you subclass AMBeamline, it's important to provide your own accessor function, for example:
\code
static YOURBeamline* bl() {
	if(!instance_)
		instance_ = new YOURBeamline();
	return static_cast<YOURBeamline*>(instance_);
}
\endcode
which sets AMBeamline's protected instance_ variable.

As long as the FIRST call to use the beamline is through YOURBeamline::bl(), then all successive calls to AMBeamline::bl() will return the instance of your specific beamline.  If anything calls AMBeamline::bl() before this, there will be no instance_ yet, and it will return 0.  Therefore, it's necessary to call YOURBeamline::bl() to initialize the beamline object before any other code might access AMBeamline; we would normally place this intialization inside your specific version of AMAppController::startup().
*/

class AMBeamline : public AMControl {

	Q_OBJECT

public:
	/// Singleton class accessor function. Use this to access the application-wide Beamline object.
	/*! See note on subclassing and inheritance above. It's important that your own beamline's initialization function be called before this one (ex: YourBeamline::bl()) */
	static AMBeamline* bl() { return instance_; }
	/// Call this to delete the beamline object instance
	static void releaseBl();

	virtual ~AMBeamline();


	/// Reports whether the beamline is currently in exclusive use, and should not be changed. (For example: you or some other program is running a scan). The base class always returns false; your should re-implement this function if you know better.
	virtual bool isBeamlineScanning() const { return false; }

signals:
	/// Emit this signal whenever isBeamlineScanning() changes.
	void beamlineScanningChanged(bool isScanning);

protected:
	/// Singleton classes have a protected constructor; all access is through AMBeamline::bl() or YourBeamline::bl()
	AMBeamline(const QString& controlName);
	/// Instance variable
	static AMBeamline* instance_;

};

#endif /*BEAMLINE_H_*/

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


#ifndef AMLOOPACTIONINFO_H
#define AMLOOPACTIONINFO_H

#include "actions2/AMActionInfo.h"

/// This class specifies the information for an AMLoopAction. (In this case, the information is only the number of iterations to loop for.)  It provides a simple example of a how to subclass AMActionInfo.
class AMLoopActionInfo : public AMActionInfo
{
	Q_OBJECT
	Q_PROPERTY(int loopCount READ loopCount WRITE setLoopCount NOTIFY loopCountChanged)

public:
	/// Constructor. Specify the \c loopCount: number of iterations you want to loop for
	Q_INVOKABLE AMLoopActionInfo(int iterations = 3, QObject *parent = 0);

	/// Copy Constructor
	AMLoopActionInfo(const AMLoopActionInfo& other) : AMActionInfo(other), loopCount_(other.loopCount_) {}

	/// This function is used as a virtual copy constructor
	virtual AMActionInfo* createCopy() const { return new AMLoopActionInfo(*this); }

	// Re-implemented public functions
	/////////////////////////////////

	/// This should describe the type of the action
	virtual QString typeDescription() const { return "Loop (Repeat)"; }

	// New public functions
	//////////////////////////

	/// The total number of iterations that we're supposed to loop for
	int loopCount() const { return loopCount_; }

	/// Set the total number of iteration to loop for.
	void setLoopCount(int loopCount);

signals:

	/// Emitted when the prescribed number of iterations changes
	void loopCountChanged(int newLoopCount);

public slots:

protected:
	/// The number of iterations to loop for.
	int loopCount_;

};


#endif // AMLOOPACTIONINFO_H

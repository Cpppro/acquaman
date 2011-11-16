/*
Copyright 2010, 2011 Mark Boots, David Chevrier, and Darren Hunter.

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


#ifndef AMDETECTORVIEW_H
#define AMDETECTORVIEW_H

#include "AMControlSetView.h"
#include "AMDetectorViewSupport.h"
#include "beamline/AMSingleControlDetector.h"
#include "beamline/MCPDetector.h"
#include "beamline/PGTDetector.h"
#include "ui/beamline/AMControlEditor.h"
#include "QMessageBox"
#include "QMetaMethod"

class AMDetectorView : public QWidget
{
Q_OBJECT

public:
	Q_INVOKABLE explicit AMDetectorView(bool configureOnly = false, QWidget *parent = 0);

	virtual AMDetector* detector();

	virtual AMDetectorInfo* configurationSettings() const;

public slots:

signals:
	void settingsChangeRequested();
	void settingsConfigureRequested();

protected:
	/// We are trusting createDetectorView to pass in the correct type of detector, sub classes should trust AMDetector is actually their type
	virtual bool setDetector(AMDetector *detector, bool configureOnly = false);
	friend AMDetectorView* AMDetectorViewSupport::createDetectorView(AMDetector *detector, bool configureOnly);
	friend AMDetectorView* AMDetectorViewSupport::createBriefDetectorView(AMDetector *detector, bool configureOnly);
	friend AMDetectorView* AMDetectorViewSupport::createDetailedDetectorView(AMDetector *detector, bool configureOnly);

protected:
	bool configureOnly_;
};

class AMBriefDetectorView : public AMDetectorView
{
Q_OBJECT
public:
	Q_INVOKABLE explicit AMBriefDetectorView(bool configureOnly = false, QWidget *parent = 0);

protected:
	/// We are trusting createDetectorView to pass in the correct type of detector, sub classes should trust AMDetector is actually their type
	virtual bool setDetector(AMDetector *detector, bool configureOnly = false);
};

class AMDetailedDetectorView : public AMDetectorView
{
Q_OBJECT
public:
	Q_INVOKABLE explicit AMDetailedDetectorView(bool configureOnly = false, QWidget *parent = 0);

protected:
	/// We are trusting createDetectorView to pass in the correct type of detector, sub classes should trust AMDetector is actually their type
	virtual bool setDetector(AMDetector *detector, bool configureOnly = false);
};

#endif // AMDETECTORVIEW_H
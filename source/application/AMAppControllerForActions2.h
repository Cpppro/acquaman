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


#ifndef AMAPPCONTROLLER_H
#define AMAPPCONTROLLER_H

#include <QObject>

#include "application/AMDatamanAppControllerForActions2.h"
#include <QHash>

class AMWorkflowView;
class AMMainWindow;
class AMScan;
class AMScanConfiguration;
class AMExporter;
class AMExporterOption;
class AMAction;

/// This class extends the base dataman app controller class by adding the workflow.  This is the base class for all beamline acquisition app controllers.  The reason for the distinction between this class and the dataman version is a result for the desire to be able to take the dataman version home with the user whereas this version is meant to reside on beamlines that always have access to beamline components and controls.
class AMAppControllerForActions2 : public AMDatamanAppControllerForActions2
{
Q_OBJECT
public:
	/// This constructor is empty. Call AMAppController::startup() to create all of the application windows, widgets, and data objects that are needed on program startup.
	explicit AMAppControllerForActions2(QObject *parent = 0);
	/// The destructor is empty.  Call AMAppController::shutdown() to delete all objects and clean up.
	virtual ~AMAppControllerForActions2() {}


	/// Re-implemented from AMDatamanAppControllerForActions2 to register editors for common actions
	virtual bool startupRegisterDatabases();
	/// Re-implemented from AMDatamanAppController to add the workflow pane, and show the run selection dialog / splash screen.
	virtual bool startupCreateUserInterface();
	/// Re-implemented from AMDatamanAppController to provide a menu action for changing the current run.
	virtual bool startupInstallActions();
	/// Re-implemented to connect action state changes to our onCurrentActionStateChanged slot.
	virtual bool startupAfterEverything();

	// Shutdown: nothing special to do except use the base class shutdown().
//	virtual void shutdown();

signals:

public slots:

	/// This function can be used to "hand-off" a scan to the app controller (for example, a new scan from a scan controller). An editor will be opened for the scan. (Thanks to AMScan's retain()/release() mechanism, the scan will be deleted automatically when all scan controllers and editors are done with it.)
	/*! By default, the new scan editor will be brought to the front of the main window and presented to the user. You can suppress this by setting \c bringEditorToFront to false.

If \c openInExistingEditor is set to true, and if there is an existing editor, the scan will be appended inside that editor. (If there is more than one open editor, the scan will be added to the most recently-created one.)  By default, a new editor window is created.*/
	void openScanInEditor(AMScan* scan, bool bringEditorToFront = true, bool openInExistingEditor = false);

	/// Bring the Workflow view to the front.
	virtual void goToWorkflow();


	/// Displays a dialog for changing the current run, if a user wants to do that while the app is still open.
	void showChooseRunDialog();

	///////////////////////////////////

protected:
	/// Top-level panes in the main window
	AMWorkflowView* workflowView_;

	/// Filters the closeEvent on the main window, in case there's any reason why we can't quit directly. (ie: scans modified and still open, or an action is still running)
	virtual bool eventFilter(QObject *, QEvent *);

	/// Checks to make sure no actions are currently running in the AMActionRunner. If there are, asks user if they want to cancel them, but still returns false.
	bool canCloseActionRunner();


protected slots:
	/// This watches when the state of the current action changes (as notified by AMActionRunner). For now, we notice when an AMScanControllerAction starts, and create an editor for its scan.
	virtual void onCurrentActionStateChanged(int newState, int oldState);

	/// This updates the progress bar in the bottom bar based on the current action
	void onCurrentActionProgressChanged(double elapsed, double total);
	/// This updates the text of the bottom bar with the name of the current action
	void onCurrentActionChanged(AMAction* action);
};

#endif // AMAPPCONTROLLER_H

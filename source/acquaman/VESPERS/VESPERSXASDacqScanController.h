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


#ifndef VESPERSXASDACQSCANCONTROLLER_H
#define VESPERSXASDACQSCANCONTROLLER_H

#include "acquaman/AMDacqScanController.h"
#include "acquaman/VESPERS/VESPERSXASScanConfiguration.h"
#include "dataman/AMXASScan.h"

/// Some defined error codes to help with controller crashes.
#define VESPERSXASDACQSCANCONTROLLER_CANT_INTIALIZE 77001
#define VESPERSXASDACQSCANCONTROLLER_CANT_START_BL_SCANNING 77002
#define VESPERSXASDACQSCANCONTROLLER_CANT_START_DETECTOR_SOURCE_MISMATCH 77003
#define VESPERSXASDACQSCANCONTROLLER_CANT_START_NO_CFG_FILE 77004

class VESPERSXASDacqScanController : public AMDacqScanController
{
	Q_OBJECT

public:
	/// Constructor.
	/// \param cfg is the XAS configuration that the controller will run.
	VESPERSXASDacqScanController(VESPERSXASScanConfiguration *cfg, QObject *parent = 0);

protected slots:
	/// Slot that handles the successful initialization of the scan.
	void onInitializationActionsSucceeded();
	/// Slot that handles the failed initialization of the scan.
	void onInitializationActionsFailed(int explanation);
	/// Slot that handles the initialization progress of the scan.
	void onInitializationActionsProgress(double elapsed, double total);

	// Re-implementing to change actual dwell times for the VESPERS Beamline
	void onDwellTimeTriggerChanged(double newValue);

protected:
	/// Specific implementation of the scan initialization.
	bool initializeImplementation();
	/// Specific implmentation of the scan start.
	bool startImplementation();

	AMnDIndex toScanIndex(QMap<int, double> aeData);

	/// Adds all the data sources that are still important but not visualized.
	void addExtraDatasources();

	/// Sets up the XAS scan based on no fluorescence detectors selected.
	bool setupTransmissionXAS();
	/// Sets up the XAS scan based on the single element vortex detector being selected.
	bool setupSingleElementXAS();
	/// Sets up the XAS scan based on the four element vortex detector being selected.
	bool setupFourElementXAS();

	/// Returns the home directory for Acquaman.
	QString getHomeDirectory();

	/// Pointer to the configuration used by this controller.
	VESPERSXASScanConfiguration *config_;

	/// A counter holding the current region index being scanned.
	int currentRegionIndex_;
};

#endif // VESPERSXASDACQSCANCONTROLLER_H

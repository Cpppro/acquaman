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


#include "VESPERSAppController.h"

#include "beamline/VESPERS/VESPERSBeamline.h"
#include "ui/VESPERS/VESPERSEndstationView.h"
#include "ui/AMMainWindow.h"
#include "ui/acquaman/AMScanConfigurationViewHolder.h"

#include "ui/VESPERS/XRFDetectorView.h"
#include "ui/VESPERS/VESPERSXRFFreeRunView.h"
#include "ui/VESPERS/VESPERSPersistentView.h"
#include "dataman/VESPERS/AMXRFScan.h"
#include "util/AMPeriodicTable.h"
#include "ui/VESPERS/VESPERSDeviceStatusView.h"
#include "ui/VESPERS/VESPERSEXAFSScanConfigurationView.h"
#include "ui/VESPERS/VESPERSExperimentConfigurationView.h"
#include "ui/VESPERS/VESPERSRoperCCDDetectorView.h"
#include "ui/VESPERS/VESPERS2DScanConfigurationView.h"

#include "dataman/AMScanEditorModelItem.h"
#include "ui/dataman/AMGenericScanEditor.h"

// Helper classes that technically shouldn't need to exist.
#include "util/VESPERS/ROIHelper.h"
#include "util/VESPERS/VortexDetectorStatusHelper.h"
#include "util/VESPERS/VESPERSWorkflowAssistant.h"
#include "ui/VESPERS/VESPERSWorkflowAssistantView.h"

#include "dataman/database/AMDbObjectSupport.h"
#include "application/AMAppControllerSupport.h"

#include "dataman/export/AMExportController.h"
#include "dataman/export/AMExporterOptionGeneralAscii.h"
#include "dataman/export/AMExporterGeneralAscii.h"
#include "dataman/export/AMExporterAthena.h"
#include "dataman/export/VESPERS/VESPERSExporter2DAscii.h"

#include <QFileDialog>
#include <QMessageBox>

#include "dataman/AMRun.h"

// For database registration:
#include "dataman/VESPERS/XRFDetectorInfo.h"

VESPERSAppController::VESPERSAppController(QObject *parent) :
	AMAppController(parent)
{

}

bool VESPERSAppController::startup() {

	// Get a destination folder.
	AMUserSettings::load();
	QString start = AMUserSettings::userDataFolder;
	start.chop(1);
	start = start.remove("/userData");
	QString dir = QFileDialog::getExistingDirectory(0, "Choose a destination folder for your data.", start, QFileDialog::ShowDirsOnly);
	if (!dir.isEmpty()){

		dir += "/";
		if (!dir.contains("userData")){

			QDir makeNewDir(dir);
			makeNewDir.mkdir("userData");
			makeNewDir.cd("userData");
			dir = makeNewDir.absolutePath() + "/";
		}

		if (dir.compare(AMUserSettings::userDataFolder) != 0){

			AMUserSettings::userDataFolder = dir;
			AMUserSettings::save();
		}
	}

	// Start up the main program.
	if(AMAppController::startup()) {

		// Initialize central beamline object
		VESPERSBeamline::vespers();
		// Initialize the periodic table object.
		AMPeriodicTable::table();

		registerClasses();

		// Testing and making the first run in the database, if there isn't one already.  Make this it's own function if you think startup() is getting too big ; )
		////////////////////////////////////////

		AMRun existingRun;
		if(!existingRun.loadFromDb(AMDatabase::database("user"), 1)) {
			// no run yet... let's create one.
			AMRun firstRun("VESPERS", 4);	/// \todo For now, we know that 4 is the ID of the VESPERS facility, but this is a hardcoded hack.
			firstRun.storeToDb(AMDatabase::database("user"));
		}

		setupExporterOptions();
		setupUserInterface();
		makeConnections();

		// Github setup for adding VESPERS specific comment.
		additionalIssueTypesAndAssignees_.append("I think it's a VESPERS specific issue", "dretrex");

		// THIS IS HERE TO PASS ALONG THE INFORMATION TO THE SUM AND CORRECTEDSUM PVS IN THE FOUR ELEMENT DETECTOR.
		ROIHelper *roiHelper = new ROIHelper(this);
		Q_UNUSED(roiHelper)
		// THIS IS HERE TO PASS ALONG THE INFORMATION TO THE MCA AND DXP STATUS UPDATE PVS THAT DON'T SEEM TO FOLLOW THE STANDARD NAMING CONVENTIONS.
//		VortexDetectorStatusHelper *statusHelper = new VortexDetectorStatusHelper(this);
//		Q_UNUSED(statusHelper)

		return true;
	}
	else
		return false;
}


void VESPERSAppController::shutdown() {
	// Make sure we release/clean-up the beamline interface
	AMBeamline::releaseBl();
	AMAppController::shutdown();
}

void VESPERSAppController::registerClasses()
{
	AMDbObjectSupport::s()->registerClass<XRFDetectorInfo>();
	AMDbObjectSupport::s()->registerClass<VESPERSXRFScanConfiguration>();
	AMDbObjectSupport::s()->registerClass<AMXRFScan>();
	AMDbObjectSupport::s()->registerClass<VESPERSEXAFSScanConfiguration>();
	AMDbObjectSupport::s()->registerClass<VESPERS2DScanConfiguration>();

	AMDetectorViewSupport::registerClass<XRFBriefDetectorView, XRFDetector>();
	AMDetectorViewSupport::registerClass<XRFDetailedDetectorView, XRFDetector>();
	AMDetectorViewSupport::registerClass<VESPERSRoperCCDDetectorView, VESPERSRoperCCDDetector>();

	AMExportController::registerExporter<VESPERSExporter2DAscii>();
}

void VESPERSAppController::setupExporterOptions()
{
	QList<int> matchIDs = AMDatabase::database("user")->objectsMatching(AMDbObjectSupport::s()->tableNameForClass<AMExporterOptionGeneralAscii>(), "name", "VESPERSDefault");

	AMExporterOptionGeneralAscii *vespersDefault = new AMExporterOptionGeneralAscii();

	if (matchIDs.count() != 0)
		vespersDefault->loadFromDb(AMDatabase::database("user"), matchIDs.at(0));

	vespersDefault->setName("VESPERSDefault");
	vespersDefault->setFileName("$name_$fsIndex.dat");
	vespersDefault->setHeaderText("Scan: $name #$number\nDate: $dateTime\nSample: $sample\nFacility: $facilityDescription\n\n$scanConfiguration[header]\nActual Horizontal Position:\t$controlValue[Horizontal Sample Stage] mm\nActual Vertical Position:\t$controlValue[Vertical Sample Stage] mm\n\n$notes\nNote that I0.X is the energy feedback.\n\n");
	vespersDefault->setHeaderIncluded(true);
	vespersDefault->setColumnHeader("$dataSetName $dataSetInfoDescription");
	vespersDefault->setColumnHeaderIncluded(true);
	vespersDefault->setColumnHeaderDelimiter("");
	vespersDefault->setSectionHeader("");
	vespersDefault->setSectionHeaderIncluded(true);
	vespersDefault->setIncludeAllDataSources(true);
	vespersDefault->setFirstColumnOnly(true);
	vespersDefault->setSeparateHigherDimensionalSources(true);
	vespersDefault->setSeparateSectionFileName("$name_$dataSetName_$fsIndex.dat");
	vespersDefault->storeToDb(AMDatabase::database("user"));
	qDebug() << "Added the VESPERSDefault to exporter options";

	// HEY DARREN, THIS CAN BE OPTIMIZED TO GET RID OF THE SECOND LOOKUP FOR ID
	matchIDs = AMDatabase::database("user")->objectsMatching(AMDbObjectSupport::s()->tableNameForClass<AMExporterOptionGeneralAscii>(), "name", "VESPERSDefault");
	if(matchIDs.count() > 0){

		AMAppControllerSupport::registerClass<VESPERSEXAFSScanConfiguration, AMExporterAthena, AMExporterOptionGeneralAscii>(matchIDs.at(0));
		AMAppControllerSupport::registerClass<VESPERS2DScanConfiguration, VESPERSExporter2DAscii, AMExporterOptionGeneralAscii>(matchIDs.at(0));
	}
}

void VESPERSAppController::setupUserInterface()
{
	// Create panes in the main window:
	////////////////////////////////////

	assistant_ = new VESPERSWorkflowAssistant(workflowManagerView_, this);
	VESPERSWorkflowAssistantView *assistantView = new VESPERSWorkflowAssistantView(assistant_);
	mw_->insertVerticalWidget(2, assistantView);

	// Setup the general endstation control view.
	VESPERSEndstationView *endstationView = new VESPERSEndstationView(VESPERSBeamline::vespers()->endstation());
	VESPERSDeviceStatusView *statusPage = new VESPERSDeviceStatusView;

	mw_->insertHeading("Beamline Control", 0);
	mw_->addPane(endstationView, "Beamline Control", "Endstation", ":/system-software-update.png");
	mw_->addPane(statusPage, "Beamline Control", "Device Status", ":/system-software-update.png");

	// Setup the XRF views for the single element vortex and the four element vortex detectors.  Since they have scans that are added to the workflow, it gets the workflow manager view passed into it as well.
	// This means that the FreeRunView kind of doubles as a regular detector view and a configuration view holder.
	xrf1EFreeRunView_ = new VESPERSXRFFreeRunView(new XRFFreeRun(VESPERSBeamline::vespers()->vortexXRF1E()), workflowManagerView_);
	xrf4EFreeRunView_ = new VESPERSXRFFreeRunView(new XRFFreeRun(VESPERSBeamline::vespers()->vortexXRF4E()), workflowManagerView_);

	VESPERSRoperCCDDetectorView *roperCCDView = new VESPERSRoperCCDDetectorView(VESPERSBeamline::vespers()->roperCCD());

	mw_->insertHeading("Free run", 1);
	mw_->addPane(xrf1EFreeRunView_, "Free run", "XRF 1-el", ":/utilities-system-monitor.png");
	mw_->addPane(xrf4EFreeRunView_, "Free run", "XRF 4-el", ":/utilities-system-monitor.png");
	mw_->addPane(roperCCDView, "Free run", "XRD - Roper", ":/utilities-system-monitor.png");

	// Setup page that auto-enables detectors.
	VESPERSExperimentConfigurationView *experimentConfigurationView = new VESPERSExperimentConfigurationView(VESPERSBeamline::vespers()->experimentConfiguration());

	// Setup XAS for the beamline.  Builds the config, view, and view holder.
	exafsScanConfig_ = new VESPERSEXAFSScanConfiguration();
	exafsScanConfig_->addRegion(0, -30, 0.5, 40, 1);
	VESPERSEXAFSScanConfigurationView *exafsConfigView = new VESPERSEXAFSScanConfigurationView(exafsScanConfig_);
	exafsConfigViewHolder_ = new AMScanConfigurationViewHolder( workflowManagerView_, exafsConfigView);

	// Setup 2D maps for the beamline.  Builds the config, view, and view holder.
	VESPERS2DScanConfiguration *mapScanConfiguration = new VESPERS2DScanConfiguration();
	mapScanConfiguration->setStepSize(0.005, 0.005);
	mapScanConfiguration->setTimeStep(1);
	VESPERS2DScanConfigurationView *mapScanConfigurationView = new VESPERS2DScanConfigurationView(mapScanConfiguration);
	AMScanConfigurationViewHolder *mapScanConfigurationViewHolder = new AMScanConfigurationViewHolder(workflowManagerView_, mapScanConfigurationView);
	connect(mapScanConfigurationView, SIGNAL(configureDetector(QString)), this, SLOT(onConfigureDetectorRequested(QString)));

	mw_->insertHeading("Scans", 2);
	mw_->addPane(experimentConfigurationView, "Scans", "Experiment Setup", ":/utilities-system-monitor.png");
	mw_->addPane(exafsConfigViewHolder_, "Scans", "XAS", ":/utilities-system-monitor.png");
	mw_->addPane(mapScanConfigurationViewHolder, "Scans", "2D Maps", ":/utilities-system-monitor.png");

	// This is the right hand panel that is always visible.  Has important information such as shutter status and overall controls status.  Also controls the sample stage.
	VESPERSPersistentView *persistentView = new VESPERSPersistentView;
	mw_->addRightWidget(persistentView);

	// Show the endstation control view first.
	mw_->setCurrentPane(experimentConfigurationView);
}

void VESPERSAppController::makeConnections()
{
	connect(AMScanControllerSupervisor::scanControllerSupervisor(), SIGNAL(currentScanControllerCreated()), this, SLOT(onCurrentScanControllerCreated()));
	connect(AMScanControllerSupervisor::scanControllerSupervisor(), SIGNAL(currentScanControllerStarted()), this, SLOT(onCurrentScanControllerStarted()));
	connect(AMScanControllerSupervisor::scanControllerSupervisor(), SIGNAL(currentScanControllerDestroyed()), this, SLOT(onCurrentScanControllerFinished()));

	// Bottom bar connections.
	connect(this, SIGNAL(pauseScanIssued()), this, SLOT(onPauseScanIssued()));
	connect(this, SIGNAL(stopScanIssued()), this, SLOT(onCancelScanIssued()));

	connect(this, SIGNAL(scanEditorCreated(AMGenericScanEditor*)), this, SLOT(onScanEditorCreated(AMGenericScanEditor*)));
}

void VESPERSAppController::onConfigureDetectorRequested(const QString &detector)
{
	if (detector == "Single Element")
		mw_->setCurrentPane(xrf1EFreeRunView_);
	else if (detector == "Four Element")
		mw_->setCurrentPane(xrf4EFreeRunView_);
}

void VESPERSAppController::onCurrentScanControllerStarted()
{
	QString fileFormat(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController()->scan()->fileFormat());

	if (fileFormat == "vespersXRF" || fileFormat == "vespers2011XRF")
		return;

	AMScan *scan = AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController()->scan();
	openScanInEditorAndTakeOwnership(scan);

	AMGenericScanEditor *newEditor = scanEditorAt(scanEditorCount() -1);

	VESPERSEXAFSScanConfiguration *config = qobject_cast<VESPERSEXAFSScanConfiguration *>(scan->scanConfiguration());

	if (config){

		switch(config->fluorescenceDetectorChoice()){

			case VESPERSEXAFSScanConfiguration::None:

				newEditor->setExclusiveDataSourceByName("trans");
				break;

			case VESPERSEXAFSScanConfiguration::SingleElement:
			case VESPERSEXAFSScanConfiguration::FourElement:
			{
				QStringList dataSources(newEditor->visibleDataSourceNames());
				int index = 0;

				for (int i = 0; i < dataSources.size(); i++){

					// Grabbing the Ka or La emission line because that will be the one people will want to see.
					if (dataSources.at(i).contains("norm") && dataSources.at(i).contains(config->edge().remove(" ")+"a"))
						index = i;
				}

				newEditor->setExclusiveDataSourceByName(dataSources.at(index));

				break;
			}
		}
	}

	VESPERS2DScanConfiguration *config2D = qobject_cast<VESPERS2DScanConfiguration *>(scan->scanConfiguration());

	if (config2D)
		newEditor->setExclusiveDataSourceByName(scan->analyzedDataSources()->at(0)->name());
}

void VESPERSAppController::onCurrentScanControllerCreated()
{
	QString fileFormat(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController()->scan()->fileFormat());

	if (fileFormat == "vespersXRF" || fileFormat == "vespers2011XRF")
		return;

	connect(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController(), SIGNAL(progress(double,double)), this, SLOT(onProgressUpdated(double,double)));
	connect(VESPERSBeamline::vespers(), SIGNAL(beamDumped()), this, SLOT(onBeamDump()));

	if (fileFormat == "vespersXAS" || fileFormat == "vespers2011XAS" || fileFormat == "vespers2011EXAFS")
		connect(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController(), SIGNAL(progress(double,double)), assistant_, SLOT(onCurrentProgressChanged(double,double)));
}

void VESPERSAppController::onCurrentScanControllerFinished()
{
	QString fileFormat(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController()->scan()->fileFormat());

	if (fileFormat == "vespersXRF" || fileFormat == "vespers2011XRF")
		return;

	if (AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController()->state() == AMScanController::Cancelled
			&& (fileFormat == "vespersXAS" || fileFormat == "vespers2011XAS" || fileFormat == "vespers2011EXAFS")){

		assistant_->onScanCancelled();
		disconnect(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController(), SIGNAL(progress(double,double)), assistant_, SLOT(onCurrentProgressChanged(double,double)));
	}

	disconnect(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController(), SIGNAL(progress(double,double)), this, SLOT(onProgressUpdated(double,double)));
	disconnect(VESPERSBeamline::vespers(), SIGNAL(beamDumped()), this, SLOT(onBeamDump()));
}

void VESPERSAppController::onBeamDump()
{
	AMScanController *controller = AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController();

	if (controller && controller->isRunning())
		controller->pause();
}

void VESPERSAppController::onPauseScanIssued()
{
	AMScanController *controller = AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController();

	if (controller && controller->isRunning())
		controller->pause();

	else if (controller && controller->isPaused())
		controller->resume();
}

void VESPERSAppController::onCancelScanIssued()
{
	AMScanController *controller = AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController();

	if (controller)
		controller->cancel();
}

void VESPERSAppController::onScanEditorCreated(AMGenericScanEditor *editor)
{
	if (editor->using2DScanView())
		connect(editor, SIGNAL(dataPositionChanged(AMGenericScanEditor*,QPoint)), this, SLOT(onDataPositionChanged(AMGenericScanEditor*,QPoint)));
}

void VESPERSAppController::onDataPositionChanged(AMGenericScanEditor *editor, const QPoint &pos)
{
	QString text = "Setup at: (";
	text.append(QString::number(editor->dataPosition().x(), 'f', 3));
	text.append(" mm, ");
	text.append(QString::number(editor->dataPosition().y(), 'f', 3));
	text.append(" mm)");

	QMenu popup(text, editor);
	QAction *temp = popup.addAction(text);
	popup.addSeparator();
	popup.addAction("XANES scan");
	popup.addAction("EXAFS scan");
	popup.addSeparator();
	popup.addAction("Go to immediately");

	temp = popup.exec(pos);

	if (temp){

		if (temp->text() == "XANES scan")
			setupXASScan(editor, false);

		else if (temp->text() == "EXAFS scan")
			setupXASScan(editor, true);

		else if (temp->text() == "Go to immediately"){

			cleanMoveImmediatelyAction();	// Clean up the action just in case.

			AMBeamlineParallelActionsList *moveImmediatelyList = new AMBeamlineParallelActionsList;
			moveImmediatelyAction_ = new AMBeamlineListAction(moveImmediatelyList);
			moveImmediatelyList->appendStage(new QList<AMBeamlineActionItem *>());
			moveImmediatelyList->appendAction(0, VESPERSBeamline::vespers()->pseudoSampleStage()->createHorizontalMoveAction(pos.x()));
			moveImmediatelyList->appendStage(new QList<AMBeamlineActionItem *>());
			moveImmediatelyList->appendAction(1, VESPERSBeamline::vespers()->pseudoSampleStage()->createVerticalMoveAction(pos.y()));

			connect(moveImmediatelyAction_, SIGNAL(succeeded()), this, SLOT(onMoveImmediatelySuccess()));
			connect(moveImmediatelyAction_, SIGNAL(failed(int)), this, SLOT(onMoveImmediatelyFailure()));
			moveImmediatelyAction_->start();
		}
	}
}


void VESPERSAppController::onMoveImmediatelySuccess()
{
	cleanMoveImmediatelyAction();
}

void VESPERSAppController::onMoveImmediatelyFailure()
{
	cleanMoveImmediatelyAction();

	QMessageBox::warning(mw_, "Sample Stage Error", "The sample stage was unable to complete the desired movement.");
}

void VESPERSAppController::cleanMoveImmediatelyAction()
{
	if (moveImmediatelyAction_ == 0)
		return;

	// Disconnect all signals and return all memory.
	moveImmediatelyAction_->disconnect();
	AMBeamlineParallelActionsList *actionList = moveImmediatelyAction_->list();

	for (int i = 0; i < actionList->stageCount(); i++){

		while (actionList->stage(i)->size())
			actionList->stage(i)->takeAt(0)->deleteLater();
	}

	moveImmediatelyAction_->deleteLater();
	moveImmediatelyAction_ = 0;
}

void VESPERSAppController::setupXASScan(const AMGenericScanEditor *editor, bool setupEXAFS)
{
	exafsScanConfig_->setGoToPosition(true);
	exafsScanConfig_->setX(editor->dataPosition().x());
	exafsScanConfig_->setY(editor->dataPosition().y());

	QString edge = editor->exclusiveDataSourceName();
	edge = edge.remove("norm_");
	edge.chop(2);
	edge.insert(edge.size()-1, " ");

	if (edge.at(edge.size()-1) == 'L')
		edge.append("1");

	exafsScanConfig_->setEdge(edge);

	// This should always succeed because the only way to get into this function is using the 2D scan view which currently only is accessed by 2D scans.
	VESPERS2DScanConfiguration *config = qobject_cast<VESPERS2DScanConfiguration *>(editor->currentScan()->scanConfiguration());
	if (config){

		exafsScanConfig_->setName(config->name());
		exafsScanConfig_->setFluorescenceDetectorChoice(config->fluorescenceDetectorChoice());
		exafsScanConfig_->setIncomingChoice(config->incomingChoice());
	}

	while (exafsScanConfig_->regionCount() != 1)
		exafsScanConfig_->deleteRegion(0);

	if (setupEXAFS){

		exafsScanConfig_->exafsRegions()->setType(0, AMEXAFSRegion::Energy);
		exafsScanConfig_->setRegionStart(0, -200);
		exafsScanConfig_->setRegionDelta(0, 10);
		exafsScanConfig_->setRegionEnd(0, -30);
		exafsScanConfig_->setRegionTime(0, 1);

		exafsScanConfig_->regions()->addRegion(1, -30, 0.5, 40, 1);
		exafsScanConfig_->exafsRegions()->setType(1, AMEXAFSRegion::Energy);

		exafsScanConfig_->regions()->addRegion(2, 40, 0.05, 857.4627, 10); // 857.4627 = 15k
		exafsScanConfig_->exafsRegions()->setType(2, AMEXAFSRegion::kSpace);
		exafsScanConfig_->exafsRegions()->setEndByType(2, 15, AMEXAFSRegion::kSpace);
	}

	else {

		exafsScanConfig_->setRegionStart(0, -30);
		exafsScanConfig_->setRegionDelta(0, 0.5);
		exafsScanConfig_->setRegionEnd(0, 40);
		exafsScanConfig_->setRegionTime(0, 1);
		exafsScanConfig_->exafsRegions()->setType(0, AMEXAFSRegion::Energy);
	}

	mw_->undock(exafsConfigViewHolder_);
}

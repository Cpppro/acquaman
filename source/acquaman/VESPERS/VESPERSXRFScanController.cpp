#include "VESPERSXRFScanController.h"
#include "dataman/AMUser.h"
#include "dataman/AMRawDataSource.h"
#include "beamline/VESPERS/VESPERSBeamline.h"
#include "dataman/VESPERS/VESPERSXRFDataLoader.h"

#include <QDir>

VESPERSXRFScanController::VESPERSXRFScanController(VESPERSXRFScanConfiguration *scanConfig, QObject *parent)
	: AMScanController(scanConfig, parent)
{
	if (scanConfig->detectorChoice() == VESPERSBeamline::SingleElement)
		detector_ = VESPERSBeamline::vespers()->vortexXRF1E();
	else
		detector_ = VESPERSBeamline::vespers()->vortexXRF4E();

	scanConfig->setDetectorInfo(detector_->toXRFInfo());

	scan_ = new AMXRFScan;
	generalScan_ = scan_;
	scan_->setScanConfiguration(scanConfig);
	scan_->setName(QString("XRF Scan - %1 el").arg(detector_->elements()));
	QString fullPath = AMUserSettings::defaultFilePath(QDateTime::currentDateTime());
	QString path = fullPath.section('/', 0, -2);
	QString file = fullPath.section('/', -1);
	file.append(".dat");
	QDir dir;
	dir.mkpath(path);
	scan_->setFilePath(fullPath + ".dat");
	scan_->setFileFormat("vespersXRF");
	scan_->setRunId(AMUser::user()->currentRunId());

	for (int i = 0; i < detector_->elements(); i++){

		scan_->rawData()->addMeasurement(AMMeasurementInfo(QString("raw%1").arg(i+1), QString("Element %1").arg(i+1), "eV", detector_->axes()));
		scan_->addRawDataSource(new AMRawDataSource(scan_->rawData(), i));
	}

	/// \todo Add analyzed data sources.
}

void VESPERSXRFScanController::onProgressUpdate()
{
	emit progress(detector_->elapsedTime(), detector_->integrationTime());
}

void VESPERSXRFScanController::onStatusChanged()
{
	if (!detector_->status())
		onDetectorAcquisitionFinished();
}

bool VESPERSXRFScanController::startImplementation()
{
	connect(detector_, SIGNAL(statusChanged()), this, SLOT(onStatusChanged()));
	connect(detector_->elapsedTimeControl(), SIGNAL(valueChanged(double)), this, SLOT(onProgressUpdate()));
	detector_->start();

	setStarted();

	return true;
}

void VESPERSXRFScanController::onDetectorAcquisitionUpdate()
{
	for (int i = 0; i < detector_->elements(); i++)
		scan_->rawData()->setValue(AMnDIndex(), i, detector_->spectraAt(i), detector_->channels());
}

void VESPERSXRFScanController::onDetectorAcquisitionFinished()
{
	disconnect(detector_, SIGNAL(statusChanged()), this, SLOT(onStatusChanged()));
	disconnect(detector_->elapsedTimeControl(), SIGNAL(valueChanged(double)), this, SLOT(onProgressUpdate()));

	onDetectorAcquisitionUpdate();

	AMScan *scan = this->scan();
	if(scan->database())
		scan->storeToDb(scan->database());
	else
		scan->storeToDb(AMDatabase::userdb());
	saveData();

	setFinished();
}

void VESPERSXRFScanController::saveData()
{
	VESPERSXRFDataLoader exporter(scan_);
	if (!exporter.saveToFile(scan_->filePath()))
		AMErrorMon::report(AMErrorReport(this, AMErrorReport::Serious, 0, QString("Could not save XRF data.")));
}
#include "REIXSXESRawFileLoader.h"

#include <QFile>
#include <QDataStream>
#include "util/AMErrorMonitor.h"

#include <QPair>

/// Magic string found and the beginning and end of the file to identify the file type and version
#define REIXSXESRAWFILELOADER_MAGIC_STRING "REIXSXESRAW2011.1"

typedef QPair<int, QString> REIXSXESRawFileException;

REIXSXESRawFileLoader::REIXSXESRawFileLoader(AMXESScan *scan)
	: AMAbstractFileLoader(scan)
{
}


bool REIXSXESRawFileLoader::loadFromFile(const QString &filepath, bool setMetaData, bool setRawDataSources, bool createDefaultAnalysisBlocks) {

	AMXESScan* scan = qobject_cast<AMXESScan*>(scan_);

	if(!scan) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1, "REIXS XES Raw File Loader: Could not load XES data into a non-XES scan."));
		return false;
	}

	QFile file(filepath);
	qint32 pixelsX, pixelsY;
	qint32* image;
	QList<AMAxisInfo> detectorAxes;

	try {

		if(!file.open(QIODevice::ReadOnly))
			throw REIXSXESRawFileException(-1, "Could not open the data file");

		// check the magic string (ascii encoded string at the start of the file)
		QByteArray magicTextShouldBe(REIXSXESRAWFILELOADER_MAGIC_STRING);
		QByteArray magicText = file.read(magicTextShouldBe.length());
		if(magicText != magicTextShouldBe)
			throw REIXSXESRawFileException(-2, "Couldn't load invalid file format");


		// read number of pixels in X and Y
		QDataStream ds(&file);
		ds >> pixelsX;
		ds >> pixelsY;

		if(pixelsX == 0 || pixelsY == 0)
			throw REIXSXESRawFileException(-3, "Couldn't load corrupted file");

		// read all the pixel values
		image = new qint32[pixelsX*pixelsY];
		for(int i=0; i<pixelsX*pixelsY; i++)
			ds >> *(image++);

		// read the magic string at the end of the file (makes sure we had all the data)
		magicText = file.read(magicTextShouldBe.length());
		if(magicText != magicTextShouldBe)
			throw REIXSXESRawFileException(-2, "Couldn't load corrupted file -- there wasn't enough data.");

		file.close();
	}
	catch(REIXSXESRawFileException e) {
		AMErrorMon::report(AMErrorReport(scan, AMErrorReport::Alert, e.first, QString("REIXS XES Raw File Loader: %1\n\nFile: '%2'").arg(e.second).arg(filepath)));
		file.close();
		return false;
	}

	// clear the existing raw data (and raw data sources, if we're supposed to)
	if(setRawDataSources)
		scan->clearRawDataPointsAndMeasurementsAndDataSources();
	else
		scan->clearRawDataPointsAndMeasurements();

	detectorAxes.append(AMAxisInfo("x", pixelsX, "x - energy axis", "pixels"));
	detectorAxes.append(AMAxisInfo("y", pixelsY, "y - vertical axis", "pixels"));
	scan->rawData()->addMeasurement(AMMeasurementInfo("xesImage",
													  "XES Detector Image",
													  "counts",
													  detectorAxes));

	scan->rawData()->setValue(AMnDIndex(), 0, image, pixelsX*pixelsY);
	delete [] image;


	if(setRawDataSources) {
		scan->addRawDataSource(new AMRawDataSource(scan->rawData(), 0));
	}
	else {
		// There's pre-existing raw data sources. make sure that there's only one raw data source: (integrity check: we only have one measurement in the raw data)
		while(scan->rawDataSources()->count() > 1) {
			int i = scan->rawDataSources()->count()-1;
			AMErrorMon::report(AMErrorReport(scan, AMErrorReport::Debug, -97, QString("The data in the file (%1 columns) didn't match the raw data columns we were expecting (column %2). Removing the raw data column '%3')").arg(scan->rawData()->measurementCount()).arg(scan->rawDataSources()->at(i)->measurementId()).arg(scan->rawDataSources()->at(i)->name())));
			scan->deleteRawDataSource(i);
		}
	}


	// We don't create any analysis blocks or parse meta data for now.
	Q_UNUSED(createDefaultAnalysisBlocks)
	Q_UNUSED(setMetaData)


	return true;
}


bool REIXSXESRawFileLoader::saveToFile(const QString &filepath) {

	AMXESScan* scan = qobject_cast<AMXESScan*>(scan_);

	if(!scan) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1, "REIXS XES Raw File Loader: Could not save XES data for a non-XES scan."));
		return false;
	}

	QFile file(filepath);

	try {

		if(!file.open(QIODevice::WriteOnly))
			throw REIXSXESRawFileException(-1, "Could not open the data file for writing.");

		int measurementId = scan->rawData()->idOfMeasurement("xesImage");
		if(measurementId < 0)
			throw REIXSXESRawFileException(-2, "Could not find any XES data to save.");

		AMMeasurementInfo mi = scan->rawData()->measurementAt(measurementId);
		if(mi.rank() != 2)
			throw REIXSXESRawFileException(-3, "Could not find any XES data to save.");

		qint32 pixelsX = mi.size(0), pixelsY = mi.size(1);

		// output the magic number (ascii encoded string at the start of the file)
		QByteArray magicText(REIXSXESRAWFILELOADER_MAGIC_STRING);
		QDataStream ds(&file);
		ds << magicText;

		// Output the number of pixels in each dimension
		ds << pixelsX;
		ds << pixelsY;

		// output the pixels.  (outer loop on x, inner loop on y)
		for(int i=0; i<pixelsX; i++) {
			for(int j=0; j<pixelsY; j++) {
				qint32 counts = scan->rawData()->value(AMnDIndex(), measurementId, AMnDIndex(i,j));
				ds << counts;
			}
		}

		// output the magic string at the end of the file (used as check on reading to make sure we have it all)
		ds << magicText;

		file.close();
	}
	catch(REIXSXESRawFileException e) {
		AMErrorMon::report(AMErrorReport(scan, AMErrorReport::Alert, e.first, QString("REIXS XES Raw File Saver: %1\n\nFile: '%2'").arg(e.second).arg(filepath)));
		file.close();
		return false;
	}

	return true;
}
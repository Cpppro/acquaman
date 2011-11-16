#include "SGM2011XASFileLoaderPlugin.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "util/AMErrorMonitor.h"
#include "analysis/AM1DExpressionAB.h"
#include "analysis/AM2DSummingAB.h"

#include "dataman/AMScan.h"

bool SGM2011XASFileLoaderPlugin::accepts(AMScan *scan){
	qDebug() << "SGM2011XAS trying to accept " << scan->fileFormat();
	if(scan->fileFormat() == "sgm2011XAS")
		return true;
	return false;
}

bool SGM2011XASFileLoaderPlugin::load(AMScan *scan, const QString &userDataFolder){
	qDebug() << "\n\nTRYING TO LOAD WITH PLUGIN";

	if(columns2pvNames_.count() == 0) {
		columns2pvNames_.set("Event-ID", "Event-ID");
		columns2pvNames_.set("eV", "BL1611-ID-1:Energy");
		columns2pvNames_.set("Photodiode_PICO", "A1611-4-13:A:fbk");
		columns2pvNames_.set("I0_PICO", "A1611-4-14:A:fbk");
		columns2pvNames_.set("TEY_PICO", "A1611-4-15:A:fbk");
		columns2pvNames_.set("TFY_PICO", "A1611-4-16:A:fbk");
		columns2pvNames_.set("Photodiode_SCALER", "BL1611-ID-1:mcs03:fbk");
		columns2pvNames_.set("I0_SCALER", "BL1611-ID-1:mcs01:fbk");
		columns2pvNames_.set("TEY_SCALER", "BL1611-ID-1:mcs00:fbk");
		columns2pvNames_.set("TFY_SCALER", "BL1611-ID-1:mcs02:fbk");
		columns2pvNames_.set("EnergyFeedback", "BL1611-ID-1:Energy:fbk");

		columns2pvNames_.set("SDD", "MCA1611-01:GetChannels");
		columns2pvNames_.set("OceanOptics65000Old", "SA0000-03:Spectra");
		columns2pvNames_.set("OceanOptics65000", "SA0000-03:DarkCorrectedSpectra");
	}

	if(offsets2MeasurementInfos_.isEmpty()) {
		offsets2MeasurementInfos_ << "SDD";
		offsets2MeasurementInfos_ << "OceanOptics65000";
	}

	if(defaultUserVisibleColumns_.isEmpty()) {
		defaultUserVisibleColumns_ << "TEY";
		defaultUserVisibleColumns_ << "TFY";
		defaultUserVisibleColumns_ << "I0";
		defaultUserVisibleColumns_ << "Photodiode";
		defaultUserVisibleColumns_ << "EnergyFeedback";
		defaultUserVisibleColumns_ << "SDD";
		defaultUserVisibleColumns_ << "OceanOptics65000";
	}

	if(!scan)
		return false;

	QFileInfo sourceFileInfo(scan->filePath());
	if(sourceFileInfo.isRelative()){
		//qDebug() << "Path IS relative, user data folder is " << AMUserSettings::userDataFolder;
		//sourceFileInfo.setFile(AMUserSettings::userDataFolder + "/" + scan->filePath());
		qDebug() << "Path IS relative, user data folder is " << userDataFolder;
		sourceFileInfo.setFile(userDataFolder + "/" + scan->filePath());
	}
	else
		qDebug() << "Path IS NOT relative.";

	// used in parsing the data file
	QString line;
	QStringList lp;

	// names of the columns, taken from headers in the data file. (MUST be translated from PV strings into something meaningful)
	QStringList colNames1, colNames2;

	// open the file:
	QFile f(sourceFileInfo.filePath());
	if(!f.open(QIODevice::ReadOnly)) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -1, "SGM2011XASFileLoader parse error while loading scan data from file. Missing file."));
		return false;
	}
	QTextStream fs(&f);

	// find out what columns exist. Looking for line starting with '#(1) '
	// find out what information we've got in event ID 1
	line.clear();
	while(!fs.atEnd() && !line.startsWith("#(1) "))
		line = fs.readLine();
	if(fs.atEnd()) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -2, "SGM2011XASFileLoader parse error while loading scan data from file. Missing #(1) event line."));
		return false;	// bad format; missing the #1 event header
	}
	colNames1 = line.split(QChar(' '));
	// the first column is not a column name, it's just the event description header ("#(1)")
	colNames1.removeFirst();
	for(int i=0; i<colNames1.count(); i++){
		if(!pv2columnName(colNames1[i])){
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -2, QString("SGM2011XASFileLoader parse error while loading scan data from file. Unknown PV in header at %1 .").arg(i)));
			return false;	// bad format; no conversion column for this PV name
		}
	}


	// ensure that we have the basic "eV" column
	int eVIndex = colNames1.indexOf("eV");
	if(eVIndex < 0) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -3, "SGM2011XASFileLoader parse error while loading scan data from file. I couldn't find the energy (eV) column."));
		return false;	// bad format; no primary column

	}

	/*
	// clear the existing raw data (and raw data sources, if we're supposed to)
	if(setRawDataSources)
		scan->clearRawDataPointsAndMeasurementsAndDataSources();
	else*/
	scan->clearRawDataPointsAndMeasurements();


	// There is a rawData scan axis called "eV" created in the constructor.  AMAxisInfo("eV", 0, "Incident Energy", "eV")
	/// \todo What if there isn't? Should we check, and create the axis if none exist? What if there's more than one scan axis? Can't remove from AMDataStore... [The rest of this code assumes a single scan axis]

	// Record the indices of any columns that are actually offsets
	QList<int> offsetColumns;
	// Prepare a list of lists for the actual offsets into the file
	QList<QList<int> > initialFileOffsets;
	// A list of lists of final file offset pairs (start byte and end byte)
	QList<QList<QPair<int, int> > > fileOffsets;
	QString spectraFile = "";
	QFileInfo spectraFileInfo;
	for(int x = 0; x < colNames1.count(); x++){
		/*
		if(offsets2MeasurementInfos_.containsF(colNames1.at(x))){
		*/
		/**/
		if(offsets2MeasurementInfos_.contains(colNames1.at(x))){
			/**/
			offsetColumns << x;
			initialFileOffsets << QList<int>();
			fileOffsets << QList<QPair<int, int> >();
		}
	}
	// If there are any such columns, then check to see that there is a spectra.dat file
	if(offsetColumns.count() > 0){
		foreach(QString afp, scan->additionalFilePaths())
			if(afp.contains("_spectra.dat"))
				spectraFile = afp;
		if(spectraFile == ""){
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -3, "SGM2011XASFileLoader parse error while loading scan data from file. I couldn't find the the spectra.dat file when I need one."));
			return false;	// bad format; no spectra.dat file in the additional files paths
		}
		spectraFileInfo.setFile(spectraFile);
		if(spectraFileInfo.isRelative())
			spectraFileInfo.setFile(userDataFolder + "/" + spectraFile);
	}

	for(int x = 0; x < colNames1.count(); x++){
		QString colName = colNames1.at(x);
		if(colName != "eV" && colName != "Event-ID"){
			if(offsetColumns.contains(x)){
				/*
				AMMeasurementInfo spectraInfo(offsets2MeasurementInfos_.valueF(colName));
				scan->rawData()->addMeasurement(spectraInfo);
				*/
				/**/
				qDebug() << "Column is a spectrum offset with name " << colName;
				if(colName == "SDD"){
					qDebug() << "Adding SDD column at " << x;
					AMAxisInfo sddEVAxisInfo("energy", 1024, "SDD Energy", "eV");
					QList<AMAxisInfo> sddAxes;
					sddAxes << sddEVAxisInfo;
					AMMeasurementInfo sddInfo("SDD", "Silicon Drift Detector", "counts", sddAxes);
					scan->rawData()->addMeasurement(sddInfo);
				}
				else if(colName == "OceanOptics65000"){
					qDebug() << "Adding OOS column at " << x;
					AMAxisInfo oosWavelengthAxisInfo("wavelength", 1024, "Wavelength", "nm");
					QList<AMAxisInfo> oosAxes;
					oosAxes << oosWavelengthAxisInfo;
					AMMeasurementInfo oosInfo("OceanOptics65000", "OceanOptics 65000", "counts", oosAxes);
					scan->rawData()->addMeasurement(oosInfo);
				}
				/**/
			}
			else
				scan->rawData()->addMeasurement(AMMeasurementInfo(colName, colName));
		}
	}

	// read all the data. Add to data columns or offset lists.

	int eVAxisIndex = 0;	// counter, incremented for every data point along the scan (eV) axis.
	fs.seek(0);
	while(!fs.atEnd()) {

		line = fs.readLine();

		// event id 1.  If the line starts with "1," and there are the correct number of columns:
		if(line.startsWith("1,") && (lp = line.split(',')).count() == colNames1.count() ) {

			scan->rawData()->beginInsertRows(0);
			scan->rawData()->setAxisValue(0, eVAxisIndex, lp.at(eVIndex).toDouble()); // insert eV

			// add data from all columns (but ignore the first (Event-ID) and the eV column)
			int measurementId = 0;
			int offsetId = 0;
			for(int i=1; i<colNames1.count(); i++) {
				// Save the initial offsets for each of the spectra saved in the spectra file
				if(offsetColumns.contains(i)){
					initialFileOffsets[offsetId++].append(lp.at(i).toDouble());
					measurementId++;
				}
				else if(i!=eVIndex) {
					scan->rawData()->setValue(eVAxisIndex, measurementId++, AMnDIndex(), lp.at(i).toDouble());
				}
			}

			eVAxisIndex++;
			scan->rawData()->endInsertRows();
		}
	}

	//Check for a spectraFile, load it if we can
	if(spectraFile != ""){
		//QFile sf(spectraFile);
		QFile sf(spectraFileInfo.filePath());
		if(!sf.open(QIODevice::ReadOnly)) {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -1, "SGM2011XASFileLoader parse error while loading scan data from file. Missing spectra.dat file."));
			return false;
		}

		qDebug() << "Playing the spectra game, raw data measurement count is " << scan->rawData()->measurementCount();
		//Prep the list of start and end bytes
		// The initialFileOffsets is a list of lists of ints. The first list is the start offsets for the first spectra
		//  This is not enough, as we as need the end (need to read from start to end). End is the start of the next spectra.
		//  Just need to grab start and end (only problem is the last list needs the next point in the first list)
		//  Saved into fileOffsets, which are pairs of start and end values (much easier to use)
		int myStart, myEnd;
		for(int x = 0; x < initialFileOffsets.at(0).count(); x++){
			for(int y = 0; y < initialFileOffsets.count(); y++){
				myStart = initialFileOffsets.at(y).at(x);
				if(y+1 == initialFileOffsets.count()){
					if(x+1 == initialFileOffsets.at(0).count())
						myEnd = -1;
					else
						myEnd = initialFileOffsets.at(0).at(x+1);
				}
				else
					myEnd = initialFileOffsets.at(y+1).at(x);
				fileOffsets[y].append(QPair<int,int>(myStart, myEnd));
			}
		}

		int startByte, endByte;
		// We know the scan size ahead of time (how many points in this XAS scan)
		int scanSize = scan->rawData()->scanSize(0);
		//
		QList<double*> allSpecValues;
		//
		QList<int> allSpecSizes;
		//
		QList<int> allSpecCounters;
		// Loop over all of the offset columns we found:
		//  Append the size of each particular spectrum to the list of spectrum sizes (whether its 1024, or 2048 or something else)
		//  Alloc a double array of that size and append it to the list of spectrum values
		//  Start each spectrum counter at 0
		for(int x = 0; x < offsetColumns.count(); x++){
			qDebug() << "x is " << x << " means column is " << offsetColumns.at(x)-2;
			qDebug() << "means size is " << scan->rawData()->measurementAt(offsetColumns.at(x)-2).size(0);

			//Offset two columns for event-ID and eV
			allSpecSizes.append(scan->rawData()->measurementAt(offsetColumns.at(x)-2).size(0));
			double* theseSpecValues = new double[allSpecSizes.last()];
			allSpecValues.append(theseSpecValues);
			allSpecCounters.append(0);
		}

		// Loop over the number of points in this scan
		for(int x = 0; x < scanSize; x++){
			// Loop over all of the spectrums we found offsets for
			for(int y = 0; y < allSpecValues.count(); y++){
				// Grab the start and end bytes from the fileOffsets ... only a problem for the last item
				startByte = fileOffsets.at(y).at(x).first;
				endByte = fileOffsets.at(y).at(x).second;
				if(endByte == -1){
					endByte = spectraFileInfo.size();
					qDebug() << "Old way says " << endByte << " info way says " << spectraFileInfo.size();
				}

				// Grab the text from the specified start to end and put it into a QByteArray
				sf.seek(startByte);
				QByteArray row = sf.read( (qint64)(endByte-startByte) );

				// optimized parsing of the spectrum file. Avoid QTextStream because of unicode conversion. Avoid memory allocation as much as possible. The row is a list of integers, separated by commas and/or spaces.
				int rowLength = row.length();
				bool insideWord = false;
				bool isDouble = false;
				QString word;
				word.reserve(12);
				// Loop over the number of bytes in the row
				for(int rowb = 0; rowb<rowLength; rowb++) {
					char c = row.at(rowb);
					if(c == ' ' || c == ',') {
						// the end of a word, so convert the value to a double or an int and place it in the list of spectrum values
						if(insideWord) {
							if( (allSpecCounters[y] < allSpecSizes.at(y)) && (!isDouble) )
								(allSpecValues[y])[allSpecCounters[y]++] = word.toInt();
							else if( (allSpecCounters[y] < allSpecSizes.at(y)) && (isDouble) )
								(allSpecValues[y])[allSpecCounters[y]++] = word.toDouble();
							word.clear();
							insideWord = false;
							isDouble = false;
						}
					}
					else {
						// Still inside a word to append the byte
						if(c == '.')
							isDouble = true;
						word.append(c);
						insideWord = true;
					}
				}
				if(insideWord){ // possibly the last word (with no terminators after it)
					if( (allSpecCounters[y] < allSpecSizes.at(y)) && (!isDouble) )
						(allSpecValues[y])[allSpecCounters[y]++] = word.toInt();
					else if( (allSpecCounters[y] < allSpecSizes.at(y)) && (isDouble) )
						(allSpecValues[y])[allSpecCounters[y]++] = word.toDouble();
				}

				// By now we should have specCounter[i] = specSize[i]. If there wasn't sufficient values in the spectra file to fill the whole measurement array...
				if(allSpecCounters[y] < allSpecSizes.at(y))
					memset( allSpecValues[y]+allSpecCounters[y], 0,  (allSpecSizes.at(y)-allSpecCounters[y])*sizeof(double));

				// insert the detector values (all at once, for performance)
				//offset two columns for event-ID and eV
				scan->rawData()->setValue(x, offsetColumns.at(y)-2, allSpecValues[y], allSpecSizes.at(y));
				// Check specCounter is the right size... Not too big, not too small.
				if(allSpecCounters.at(y) != allSpecSizes.at(y)) {
					AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1, QString("SGM2011XASFileLoader found corrupted data in the SDD spectra file '%1' on row %2. There should be %3 elements in the spectra, but we only found %4").arg(spectraFile).arg(x).arg(allSpecSizes.at(y)).arg(allSpecCounters.at(y))));
				}
				allSpecCounters[y] = 0;
			}
		}
		for(int x = 0 ; x < allSpecValues.count(); x++)
			delete allSpecValues.at(x);
	}


	/*
	// If we need to create the raw data sources...
	if(setRawDataSources){
		for(int x = 0; x < scan->rawData()->measurementCount(); x++){
			if(defaultUserVisibleColumns_.contains(scan->rawData()->measurementAt(x).name))
				scan->addRawDataSource(new AMRawDataSource(scan->rawData(), x));
		}
	}


	/// Not supposed to create the raw data sources.  Do an integrity check on the pre-existing data sources instead... If there's a raw data source, but it's pointing to a non-existent measurement in the data store, that's a problem. Remove it.  \todo Is there any way to incorporate this at a higher level, so that import-writers don't need to bother?
	else {*/
		for(int i=0; i<scan->rawDataSources()->count(); i++) {
			if(scan->rawDataSources()->at(i)->measurementId() >= scan->rawData()->measurementCount()) {
				AMErrorMon::report(AMErrorReport(scan, AMErrorReport::Debug, -97, QString("The data in the file (%1 columns) didn't match the raw data columns we were expecting (column %2). Removing the raw data column '%3')").arg(scan->rawData()->measurementCount()).arg(scan->rawDataSources()->at(i)->measurementId()).arg(scan->rawDataSources()->at(i)->name())));
				scan->deleteRawDataSource(i);
			}
		}
	/*
	}
	*/


	// If the scan doesn't have any channels yet, it would be helpful to create some.
	/*
	if(createDefaultAnalysisBlocks) {

		QList<AMDataSource*> raw1DDataSources;
		for(int i=0; i<scan->rawDataSources()->count(); i++)
			if(scan->rawDataSources()->at(i)->rank() == 1)
				raw1DDataSources << scan->rawDataSources()->at(i);

		int rawTeyIndex = scan->rawDataSources()->indexOfKey("TEY");
		int rawTfyIndex = scan->rawDataSources()->indexOfKey("TFY");
		int rawI0Index = scan->rawDataSources()->indexOfKey("I0");

		if(rawTeyIndex != -1 && rawI0Index != -1) {
			AM1DExpressionAB* teyChannel = new AM1DExpressionAB("TEYNorm");
			teyChannel->setDescription("Normalized TEY");
			teyChannel->setInputDataSources(raw1DDataSources);
			teyChannel->setExpression("TEY/I0");

			scan->addAnalyzedDataSource(teyChannel);
		}

		if(rawTfyIndex != -1 && rawI0Index != -1) {
			AM1DExpressionAB* tfyChannel = new AM1DExpressionAB("TFYNorm");
			tfyChannel->setDescription("Normalized TFY");
			tfyChannel->setInputDataSources(raw1DDataSources);
			tfyChannel->setExpression("-TFY/I0");

			scan->addAnalyzedDataSource(tfyChannel);
		}


		int rawSddIndex = scan->rawDataSources()->indexOfKey("SDD");
		if(rawSddIndex != -1) {
			AMRawDataSource* sddRaw = scan->rawDataSources()->at(rawSddIndex);
			AM2DSummingAB* sddSum = new AM2DSummingAB("PFY");
			QList<AMDataSource*> sddSumSource;
			sddSumSource << sddRaw;
			sddSum->setInputDataSources(sddSumSource);
			sddSum->setSumAxis(1);
			sddSum->setSumRangeMax(sddRaw->size(1)-1);

			scan->addAnalyzedDataSource(sddSum);
		}

		int rawOOSIndex = scan->rawDataSources()->indexOfKey("OceanOptics65000");
		if(rawOOSIndex != -1) {
			AMRawDataSource* oosRaw = scan->rawDataSources()->at(rawOOSIndex);
			AM2DSummingAB* ply = new AM2DSummingAB("PLY");
			QList<AMDataSource*> plySource;
			plySource << oosRaw;
			ply->setInputDataSources(plySource);
			ply->setSumAxis(1);
			ply->setSumRangeMax(oosRaw->size(1)-1);
			scan->addAnalyzedDataSource(ply);
			if(rawOOSIndex != -1 && rawI0Index != -1) {
				AM1DExpressionAB* plyNormChannel = new AM1DExpressionAB(QString("PLYNorm"));
				plyNormChannel->setDescription("PLYNorm");
				QList<AMDataSource*> plyNormSources;
				plyNormSources.append(raw1DDataSources);
				plyNormSources.append(ply);
				plyNormChannel->setInputDataSources(plyNormSources);
				plyNormChannel->setExpression(QString("%1/%2").arg("PLY").arg("I0"));

				scan->addAnalyzedDataSource(plyNormChannel);
			}
		}
	}
	*/

	/// scan->onDataChanged(); \todo Is this still used? What does it mean?

	return true;
}

bool SGM2011XASFileLoaderFactory::accepts(AMScan *scan)
{
	return (scan->fileFormat() == "sgm2011XAS");
}

Q_EXPORT_PLUGIN2(SGM2011XASFileLoaderFactory, SGM2011XASFileLoaderFactory)

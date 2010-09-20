#include "SGM2004FileLoader.h"
#include <QDir>

AMBiHash<QString, QString> SGM2004FileLoader::columns2pvNames_;

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDateTime>
#include "dataman/AMXASScan.h"
#include "dataman/AMDetectorInfo.h"

#include <QDebug>


SGM2004FileLoader::SGM2004FileLoader(AMXASScan* scan) : AMAbstractFileLoader(scan)
{
	// this static storage can be shared across all instances, but if we're the first, need to populate it.
	if(columns2pvNames_.count() == 0) {
		columns2pvNames_.set("eV", "BL1611-ID-1:Energy");
		columns2pvNames_.set("ringCurrent", "PCT1402-01:mA:fbk");
		columns2pvNames_.set("I0_2", "A1611-4-13:A:fbk");
		columns2pvNames_.set("I0", "A1611-4-14:A:fbk");
		columns2pvNames_.set("tey", "A1611-4-15:A:fbk");
		columns2pvNames_.set("tfy", "A1611-4-16:A:fbk");
		columns2pvNames_.set("eV_fbk", "BL1611-ID-1:Energy:fbk");
		columns2pvNames_.set("pressure", "TCGC1611-423:pressure:fbk");

		columns2pvNames_.set("integrationTime", "A1611I1:cont_interval");
		columns2pvNames_.set("grating", "SG16114I1001:choice");
		columns2pvNames_.set("time", "Absolute-Time-Stamp");
	}
	if(sgmLegacyDetectors_.count() == 0){
		AMDetectorInfo *sgmDetector;
		sgmDetector = new AMDetectorInfo("tey", false);
		sgmLegacyDetectors_ << sgmDetector;
		sgmLegacyDetectorNames_ << sgmDetector->name();
		sgmDetector = new AMDetectorInfo("tfy", false);
		sgmLegacyDetectors_ << sgmDetector;
		sgmLegacyDetectorNames_ << sgmDetector->name();
		QStringList yElementNames;
		yElementNames << "count";
		sgmDetector = new AMSpectralOutputDetectorInfo("pgt", "Silicon Drift Detector", 1024, "eV", yElementNames);
		sgmLegacyDetectors_ << sgmDetector;
		sgmLegacyDetectorNames_ << sgmDetector->name();
		sgmDetector = new AMDetectorInfo("I0", false);
		sgmLegacyDetectors_ << sgmDetector;
		sgmLegacyDetectorNames_ << sgmDetector->name();
		sgmDetector = new AMDetectorInfo("eVFbk", false);
		sgmLegacyDetectors_ << sgmDetector;
		sgmLegacyDetectorNames_ << sgmDetector->name();
	}
}

/// load raw data from the SGM legacy file format into a scan's data tree.  If \c extractMetaData is set to true, this will also set the 'notes' and 'dateTime' meta-data fields.  If \c createChannels is set to true, it will create some default channels based on the data columns.
bool SGM2004FileLoader::loadFromFile(const QString& filepath, bool extractMetaData, bool createChannels) {

	// not initialized to have a scan target, or scan target is not an AMXASScan...
	AMXASScan* scan = qobject_cast<AMXASScan*>(scan_);
	if(!scan)
		return false;

	// information about the scan we hope to locate:
	QString comments;
	QDateTime datetime;
	double integrationTime;
	QString grating;

	// used in parsing the data file
	QString line;
	QStringList lp;

	// names of the columns, taken from headers in the data file. (Will be translated from PV strings into something meaningful)
	QStringList colNames1, colNames2;

	// open the file:
	QFile f(filepath);
	if(!f.open(QIODevice::ReadOnly)) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -1, "SGM2004FileLoader parse error while loading scan data from file. Missing file."));
		return false;
	}
	QTextStream fs(&f);


	if(extractMetaData) {
		// Start reading the file. look for comment line.
		while( !fs.atEnd() && fs.readLine() != QString("# COMMENT"))
			;
		if(fs.atEnd()) {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -2, "SGM2004FileLoader parse error while loading scan data from file. Could not find the comment."));
			fs.seek(0);
			// return false;	// bad format; missing the comment string
		}
		else {

			// read the comment
			line = fs.readLine();
			// chop off the "# "
			comments = line.remove(0,1).trimmed();
		}
	}


	// find out what columns exist. Looking for line starting with '#(1) '
	// find out what information we've got in event ID 1
	line.clear();
	while(!fs.atEnd() && !line.startsWith("#(1) "))
		line = fs.readLine();
	if(fs.atEnd()) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -2, "SGM2004FileLoader parse error while loading scan data from file. Missing #(1) event line."));
		return false;	// bad format; missing the #1 event header
	}
	colNames1 = line.split(QChar(' '));
	// the first column is not a column name, it's just the event description header ("#(1)")
	colNames1.removeFirst();
	for(int i=0; i<colNames1.count(); i++)
		pv2columnName(colNames1[i]);


	// find out what information we've got in event ID 2
	fs.seek(0);
	line.clear();
	while(!fs.atEnd() && !line.startsWith("#(2) "))
		line = fs.readLine();
	if(fs.atEnd()) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -2, "SGM2004FileLoader parse error while loading scan data from file. Missing #(2) event line."));
		return false;	// bad format; missing the #2 event header
	}
	colNames2 = line.split(QChar(' '));
	// the first column is not a column name, it's just the event description header ("#(1)")
	colNames2.removeFirst();
	for(int i=0; i<colNames2.count(); i++)
		pv2columnName(colNames2[i]);


	// ensure that we have the basic "eV" column
	int eVIndex = colNames1.indexOf("eV");
	if(eVIndex < 0) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, -3, "SGM2004FileLoader parse error while loading scan data from file. I couldn't find the energy (eV) column."));
		return false;	// bad format; no primary column

	}

	// ensure raw data columns exist:
	scan->d_->removeAll();	// remove all non-pricipal columns

	AMDetectorInfo *sgmDetector;
	foreach(QString colName, colNames1) {
		if(colName != "eV" && colName != "Event-ID" && sgmLegacyDetectorNames_.contains(colName))
			scan->addDetector(sgmLegacyDetectors_.at(sgmLegacyDetectorNames_.indexOf(colName)));
		else if(colName != "eV" && colName != "Event-ID"){
			sgmDetector = new AMDetectorInfo(colName, false);	/// \bug memory allocation... who's freeing?
			scan->addDetector(sgmDetector);
		}
	}

	// read all the data. Add to data columns or scan properties depending on the event-ID.
	while(!fs.atEnd()) {

		line = fs.readLine();

		// event id 1.  If the line starts with "1," and there are the correct number of columns:
		if(line.startsWith("1,") && (lp = line.split(',')).count() == colNames1.count() ) {

			// append a new datapoint to the data tree (supply primary eV value here)
			scan->d_->append(lp.at(eVIndex).toDouble());	// insert eV

			// add all columns (but ignore the first (Event-ID) and the eV column)
			for(int i=1; i<colNames1.count(); i++) {
				if(i!=eVIndex)
					scan->d_->setLastValue(colNames1.at(i), lp.at(i).toDouble());
			}
		}

		// event id 2.  If the line starts with "# 2," and there are the correct number of columns:
		else if(line.startsWith("# 2,") && (lp = line.split(',')).count() == colNames2.count() ) {

			// see if we recognize any of the translated column names as useful:
			for(int i=1; i<colNames2.count(); i++) {

				if(colNames2.at(i) == "time")
					datetime = QDateTime::fromTime_t(lp.at(i).toDouble());
				if(colNames2.at(i) == "grating")
					grating = lp.at(i);
				if(colNames2.at(i) == "integrationTime")
					integrationTime = lp.at(i).toDouble();
			}
		}
	}


	if(extractMetaData) {
		scan->setNotes(QString("Grating: %1\nIntegration Time: %2\nComments:\n%3").arg(grating).arg(integrationTime).arg(comments));
		scan->setDateTime(datetime);
	}

	// If the scan doesn't have any channels yet, it would be helpful to create some.
	if(createChannels) {
		/// \todo defaults for what channels to create?
		// scan->addChannel("eV", "eV");
		if(colNames1.contains("tey") && colNames1.contains("I0"))
			scan->addChannel("tey_n", "tey/I0");
		if(colNames1.contains("tfy") && colNames1.contains("I0"))
			scan->addChannel("tfy_n", "-1*tfy/I0");
		if(colNames1.contains("tey"))
			scan->addChannel("tey_raw", "tey");
		if(colNames1.contains("tfy"))
			scan->addChannel("tfy_raw", "tfy");
		if(colNames1.contains("I0"))
			scan->addChannel("I0", "I0");
	}

	scan->onDataChanged();

	return true;
}

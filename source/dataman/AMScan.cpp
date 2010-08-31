#include "AMScan.h"
#include "dataman/AMDatabase.h"
#include "dataman/AMDatabaseDefinition.h"
#include "acquaman.h"
#include <QDebug>

QVariant AMChannelListModel::data(const QModelIndex &index, int role) const {
	if(!index.isValid())
		return QVariant();
	if(role == Qt::DisplayRole && index.row() < ch_.count() )
		return QVariant(ch_.at(index.row())->name());
	if(role == AM::PointerRole && index.row() < ch_.count() )
		return qVariantFromValue(ch_.at(index.row()));
	return QVariant();
}
QVariant AMChannelListModel::headerData ( int section, Qt::Orientation orientation, int role) const {
	if(role != Qt::DisplayRole)
		return QVariant();
	if(orientation == Qt::Horizontal)
		return QString("Channel");
	if(orientation == Qt::Vertical)
		return QVariant(section);
	return QVariant();
}


/// returns a list of channel names currently stored.
QStringList AMChannelListModel::channelNames() const {
	QStringList names;
	foreach(AMChannel* ch, ch_) {
		names << ch->name();
	}
	return names;
}

/// returns a list of the channel expressions. (Channels are ordered the same as channelNames(). )
QStringList AMChannelListModel::channelExpressions() const {
	QStringList rv;
	foreach(AMChannel* ch, ch_)
		rv << ch->expression();
	return rv;
}



bool AMChannelListModel::addChannel(AMChannel* newChannel) {

	beginInsertRows(QModelIndex(), ch_.count(), ch_.count());
	ch_.append(newChannel);
	name2chIndex_.set(newChannel->name(), ch_.count()-1);
	endInsertRows();
	return true;
}

bool AMChannelListModel::deleteChannel(unsigned index) {
	if(index >= (unsigned)ch_.count())
		return false;

	beginRemoveRows(QModelIndex(), index, index);

	// update the name-to-index lookup... Get rid of the current index, and get rid of the highest index, since everything will move down
	name2chIndex_.removeR(index);
	name2chIndex_.removeR(ch_.count()-1);

	// remove from list
	AMChannel* deleteMe = ch_.takeAt(index);

	// in the name-to-index lookup, move everyone above this channel down
	for(int i=index; i<ch_.count(); i++)
		name2chIndex_.set(ch_.at(i)->name(), i);

	endRemoveRows();
	delete deleteMe;
	return true;
}







AMScan::AMScan(QObject *parent)
	: AMDbObject(parent)
{

	// created a new top-level data tree (not shared with anyone). Assigning it to dshared_ gives it a reference count of 1. The tree will be automatically deleted when dshared_ goes out of scope (ie: when dshared_ gets deleted, moving the reference count to 0.)
	dshared_ = d_ = new AMDataTree(0, "x", true);

	metaData_["number"] = 0;
	metaData_["dateTime"] = QDateTime::currentDateTime();
	metaData_["runId"] = QVariant();
	metaData_["sampleId"] = QVariant();
	metaData_["notes"] = QString();
	metaData_["fileFormat"] = QString("unknown");
	metaData_["filePath"] = QString();

	autoLoadData_ = true;

	sampleNameLoaded_ = false;
}

#include <QDebug>
AMScan::~AMScan() {
	// delete channels first.
	while(ch_.rowCount() != 0)
		ch_.deleteChannel(0);

}

QList<AMMetaMetaData> AMScan::metaDataUniqueKeys() {
	QList<AMMetaMetaData> rv;
	rv << AMMetaMetaData(QVariant::Int, "number", true);
	rv << AMMetaMetaData(QVariant::DateTime, "dateTime", true);
	rv << AMMetaMetaData(QVariant::Int, "runId", false);
	rv << AMMetaMetaData(QVariant::Int, "sampleId", true);
	rv << AMMetaMetaData(QVariant::String, "notes", true);
	rv << AMMetaMetaData(QVariant::StringList, "channelNames", false);
	rv << AMMetaMetaData(QVariant::StringList, "channelExpressions", false);
	rv << AMMetaMetaData(QVariant::String, "fileFormat", false);
	rv << AMMetaMetaData(QVariant::String, "filePath", false);
	return rv;
}

QVariant AMScan::metaData(const QString& key) const {

	if(key == "channelNames") {
		return channelNames();
	}

	if(key == "channelExpressions") {
		return channelExpressions();
	}

	return AMDbObject::metaData(key);
}

#include <QDebug>
bool AMScan::setMetaData(const QString& key, const QVariant& value) {

	if(key == "channelNames" || key == "channelExpressions")
		return false;

	if(key == "sampleId")
		sampleNameLoaded_ = false;

	if(key == "dateTime")
		qDebug() << "AMScan:: who is setting the dateTime?";

	return AMDbObject::setMetaData(key, value);
}

/// Convenience function: returns the name of the sample (if a sample is set)
/// \todo Is performance of this okay? Should be cached?
QString AMScan::sampleName() const {

	if(!sampleNameLoaded_)
		retrieveSampleName();

	return sampleName_;

}

void AMScan::retrieveSampleName() const {

	if(sampleId() <1 || database() == 0)
		sampleName_ = "[no sample]";

	else {
		sampleNameLoaded_ = true;	// don't set sampleNameLoaded_ above. That way we will keep checking until there's a database set (for ex: we get saved/stored.) The sampleNameLoaded_ cache is meant to speed up this database call.
		QVariant vSampleName;
		QList<QVariant*> vList;
		vList << &vSampleName;
		if(database()->retrieve(sampleId(), AMDatabaseDefinition::sampleTableName(), QString("name").split(','), vList))
			sampleName_ =  vSampleName.toString();
		else
			sampleName_ = "[no sample]";
	}
}



/// Delete a channel from scan: (All return true on success)
bool AMScan::deleteChannel(AMChannel* channel) {
	if( ch_.deleteChannel(ch_.indexOf(channel)) ) {
		setModified(true);
		return true;
	}
	else
		return false;
}

bool AMScan::deleteChannel(const QString& channelName) {
	if( deleteChannel(ch_.channel(channelName)) ) {
		setModified(true);
		return true;
	}

	else
		return false;
}

bool AMScan::deleteChannel(unsigned index) {
	if( ch_.deleteChannel(index) ) {
		setModified(true);
		return true;
	}
	else
		return false;
}



bool AMScan::validateChannelExpression(const QString& expression) {
	AMChannel tmp(this, "testChannel", expression);
	return tmp.isValid();
}

/// create a new channel. The channel is created with a QObject parent of 0, but will be owned and deleted by this Scan.  This function protects against creating channels with duplicate names.
bool AMScan::addChannel(const QString& chName, const QString& expression, bool ensureValid) {

	if(channelNames().contains(chName))
		return false;

	// invalid channel expression, and you requested validation
	if(ensureValid && !validateChannelExpression(expression))
		return false;

	ch_.addChannel(new AMChannel(this, chName, expression));

	setModified(true);
	return true;
}


// DBObject database implementation:
///////////////////////////////


#include "dataman/AMDatabaseDefinition.h"


/// Store or update self in the database. (returns true on success)
/*! Re-implemented from AMDbObject::storeToDb(), this version saves all of the meta data found for keys metaDataAllKeys(), as well as saving the channel names and channel formulas.
  */
bool AMScan::storeToDb(AMDatabase* db) {

	// the base class version is good at saving all the values in the metaData_ hash. Let's just exploit that.
	metaData_["channelNames"] = channelNames();
	metaData_["channelExpressions"] = channelExpressions();

	// Call the base class implementation
	// Return false if it fails.
	bool retval = AMDbObject::storeToDb(db);

	// This was cheating... channelNames and channelExpressions aren't stored authoritatively in the metaData_. Let's get rid of them.
	metaData_.remove("channelNames");
	metaData_.remove("channelExpressions");

	return retval;
}



bool AMScan::loadFromDb(AMDatabase* db, int sourceId) {

	QString oldFilePath = filePath();

	// always call the base class implementation first. This retrieves/loads all the base-class properties.
	// return false if it fails:
	if( !AMDbObject::loadFromDb(db, sourceId))
		return false;

	// retrieve channelNames and channelExpressions: they've been "accidentally" loaded into the hash by AMDbObject::loadFromDb().
	QStringList chNames = metaData_.take("channelNames").toStringList();
	QStringList chExpressions = metaData_.take("channelExpressions").toStringList();

	// If the file path is different than the old one, clear and reload the raw data.
	if( autoLoadData_ && filePath() != oldFilePath ) {
		if(!loadData())
			return false;
	}

	// clear the existing channels:
	while(numChannels() != 0)
		deleteChannel(numChannels()-1);


	if(chNames.count() != chExpressions.count()) {
		AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -1, "AMScan: couldn't restore saved channels. (The data was corrupted.)"));
		return false;
	}
	for(int i=0; i<chNames.count(); i++)
		addChannel(chNames[i], chExpressions[i]);
	setModified(false);

	return true;
}



#include <QPixmap>
#include <QBuffer>
#include <QByteArray>

// hackish... just needed for colors. Move the color table somewhere else.
#include "dataman/AMScanSetModel.h"

#include "MPlot/MPlot.h"

/// Return a thumbnail picture of the channel
AMDbThumbnail AMScan::thumbnail(int index) const {

	if((unsigned)index >= (unsigned)numChannels())
		return AMDbThumbnail(QString(), QString(), AMDbThumbnail::InvalidType, QByteArray());

	QPixmap pixmap(240, 180);
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);
	QGraphicsScene gscene(QRectF(0,0,240,180));
	MPlot plot(QRectF(0,0,240,180));
	gscene.addItem(&plot);

	plot.setMarginLeft(0);
	plot.setMarginRight(0);
	plot.setMarginTop(0);
	plot.setMarginBottom(10);
	plot.axisRight()->setTicks(0);
	plot.axisTop()->setTicks(0);
	plot.axisBottom()->setTicks(2);
	plot.axisLeft()->showGrid(false);
	plot.axisBottom()->showGrid(false);
	plot.axisBottom()->showAxisName(false);
	plot.axisLeft()->showAxisName(false);

	MPlotSeriesBasic series(channel(index));
	/// Todo: non-arbitrary colors here; don't mess up the ordering in AMScanSetModelChannelMetaData
	QPen seriesPen(QBrush(AMScanSetModelChannelMetaData::nextColor()), 1);
	series.setLinePen(seriesPen);
	series.setMarker(MPlotMarkerShape::None);

	plot.enableAutoScale(MPlotAxis::Left | MPlotAxis::Bottom);
	plot.addItem(&series);
	plot.doDelayedAutoScale();

	gscene.render(&painter);
	painter.end();

	/// todo: pretty names like "Total Electron Yield" instead of "tey_n"
	return AMDbThumbnail(channel(index)->name(), QString(), pixmap);

}

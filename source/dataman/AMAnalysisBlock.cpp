#include "AMAnalysisBlock.h"
#include "AMErrorMonitor.h"

/// Note that AMDbObject and AMDataSource both have a name(). (These are not virtual, for now.)  In this constructor, we initialize AMDataSource() with \c outputName, and also AMDbObject::setName() with the same.  As long as no one calls AMDbObject::setName(), these will stay consistent.  AMDataSource names are not supposed to change...
AMAnalysisBlock::AMAnalysisBlock(const QString& outputName, QObject* parent)
	: AMDbObject(parent), AMDataSource(outputName)
{
	AMDbObject::setName(outputName);
	state_ = AMDataSource::InvalidFlag;
}

bool AMAnalysisBlock::setInputDataSources(const QList<AMDataSource*>& dataSources) {
	// if a non-empty set of data sources has been provided, and they are not acceptable, return false.  (An empty list must always be acceptable)
	if(!dataSources.isEmpty() && !areInputDataSourcesAcceptable(dataSources)) {
		AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -98, QString("There was an error connecting the input data sources to this analysis component '%1: %2'. The data sources provided weren't acceptable. This can happen if they have the wrong dimension, don't provide enough data, etc.").arg(name()).arg(description())));
		return false;
	}

	for(int i=0; i<inputDataSourceCount(); i++) {
		AMDataSource* oldSource = inputDataSourceAt(i);
		disconnect(oldSource->signalSource(), SIGNAL(deleted(void*)), this, SLOT(onInputSourceDeleted(void*)));
		oldSource->deregisterObserver(this);
	}

	for(int i=0; i<dataSources.count(); i++) {
		AMDataSource* newSource = dataSources.at(i);
		connect(newSource->signalSource(), SIGNAL(deleted(void*)), this, SLOT(onInputSourceDeleted(void*)));
		dataSources.at(i)->registerObserver(this);
	}

	setInputDataSourcesImplementation(dataSources);

	emit inputSourcesChanged();
	return true;
}

void AMAnalysisBlock::onInputSourceDeleted(void* deletedSource) {
	// this implementation is just like calling setInputDataSources() with an empty list, except we don't want to call deregisterObserver() on the deleted input source. (In a single-threaded situation, this would be okay, but if the deleted() signal came through a queued signal-slot connection, then that object might already be deleted)
	for(int i=0; i<inputDataSourceCount(); i++) {
		AMDataSource* oldSource = inputDataSourceAt(i);
		if(oldSource != deletedSource) {
			disconnect(oldSource->signalSource(), SIGNAL(deleted(void*)), this, SLOT(onInputSourceDeleted(void*)));
			oldSource->deregisterObserver(this);
		}
	}

	// tell implementation to set its sources to an empty list (inactive/invalid)
	setInputDataSourcesImplementation(QList<AMDataSource*>());
	emit inputSourcesChanged();
}
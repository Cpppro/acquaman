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

#include "AM2DSummingABPlugin.h"

AM2DSummingAB::AM2DSummingAB() :
	AMStandardAnalysisBlock("Default Name", 0)
{
	sumAxis_ = 0;
	sumRangeMin_ = 0;
	sumRangeMax_ = 0;

	inputSource_ = 0;
	cacheCompletelyInvalid_ = false;
	// leave sources_ empty for now.

	axes_ << AMAxisInfo("invalid", 0, "No input data");
	setState(AMDataSource::InvalidFlag);
}

/// Constructor. \c outputName is the name() for the output data source.
AM2DSummingAB::AM2DSummingAB(const QString& outputName, QObject* parent)
	: AMStandardAnalysisBlock(outputName, parent) {

	sumAxis_ = 0;
	sumRangeMin_ = 0;
	sumRangeMax_ = 0;

	inputSource_ = 0;
	cacheCompletelyInvalid_ = false;
	// leave sources_ empty for now.

	axes_ << AMAxisInfo("invalid", 0, "No input data");
	setState(AMDataSource::InvalidFlag);

}

/// This constructor is used to reload analysis blocks directly out of the database
AM2DSummingAB::AM2DSummingAB(AMDatabase* db, int id)
	: AMStandardAnalysisBlock("tempName") {

	sumAxis_ = 0;
	sumRangeMin_ = 0;
	sumRangeMax_ = 0;

	inputSource_ = 0;
	cacheCompletelyInvalid_ = false;
	// leave sources_ empty for now.

	axes_ << AMAxisInfo("invalid", 0, "No input data");
	setState(AMDataSource::InvalidFlag);

	loadFromDb(db, id);
		// will restore sumAxis, sumRangeMin, and sumRangeMax. We'll remain invalid until we get connected.
	AMDataSource::name_ = AMDbObject::name();	// normally it's not okay to change a dataSource's name. Here we get away with it because we're within the constructor, and nothing's watching us yet.
}


/// Check if a set of inputs is valid. The empty list (no inputs) must always be valid. For non-empty lists, our specific requirements are...
/*! - there must be a single input source
	- the rank() of that input source must be 2 (two-dimensiona)
	*/
bool AM2DSummingAB::areInputDataSourcesAcceptable(const QList<AMDataSource*>& dataSources) const {

	if(dataSources.isEmpty())
		return true;	// always acceptable; the null input.

	// otherwise we need a single input source, with a rank of 2.
	if(dataSources.count() == 1 && dataSources.at(0)->rank() == 2)
		return true;

	return false;
}


/// Set the data source inputs.
void AM2DSummingAB::setInputDataSourcesImplementation(const QList<AMDataSource*>& dataSources) {

	// disconnect connections from old source, if it exists.
	if(inputSource_) {
		disconnect(inputSource_->signalSource(), SIGNAL(valuesChanged(AMnDIndex,AMnDIndex)), this, SLOT(onInputSourceValuesChanged(AMnDIndex,AMnDIndex)));
		disconnect(inputSource_->signalSource(), SIGNAL(sizeChanged(int)), this, SLOT(onInputSourceSizeChanged()));
		disconnect(inputSource_->signalSource(), SIGNAL(stateChanged(int)), this, SLOT(onInputSourceStateChanged()));
	}

	if(dataSources.isEmpty()) {
		inputSource_ = 0;
		sources_.clear();

		axes_[0] = AMAxisInfo("invalid", 0, "No input data");
		setDescription("-- No input data --");
	}

	// we know that this will only be called with valid input source
	else {
		inputSource_ = dataSources.at(0);
		sources_ = dataSources;

		int otherAxis = (sumAxis_ == 0) ? 1 : 0;
		axes_[0] = inputSource_->axisInfoAt(otherAxis);

		setDescription(QString("%1 Summed (over %2)")
					   .arg(inputSource_->name())
					   .arg(inputSource_->axisInfoAt(sumAxis_).name));

		connect(inputSource_->signalSource(), SIGNAL(valuesChanged(AMnDIndex,AMnDIndex)), this, SLOT(onInputSourceValuesChanged(AMnDIndex,AMnDIndex)));
		connect(inputSource_->signalSource(), SIGNAL(sizeChanged(int)), this, SLOT(onInputSourceSizeChanged()));
		connect(inputSource_->signalSource(), SIGNAL(stateChanged(int)), this, SLOT(onInputSourceStateChanged()));

	}

	invalidateCache();
	reviewState();

	emitSizeChanged(0);
	emitValuesChanged();
	emitAxisInfoChanged(0);
	emitInfoChanged();
}




/// Connected to be called when the values of the input data source change
void AM2DSummingAB::onInputSourceValuesChanged(const AMnDIndex& start, const AMnDIndex& end) {

	if(start.isValid() && end.isValid()) {
		int otherAxis = (sumAxis_ == 0) ? 1 : 0;
		int startIndex = start.at(otherAxis);
		int endIndex = end.at(otherAxis);
		for(int i=startIndex; i<=endIndex; i++)
			cachedValues_[i] = AMNumber();	// invalidate the changed region
		emitValuesChanged(startIndex, endIndex);
	}
	else {
		invalidateCache();
		emitValuesChanged();
	}

	/// \todo start an idle-time computation of the cached values?
}

/// Connected to be called when the size of the input source changes
void AM2DSummingAB::onInputSourceSizeChanged() {

	int otherAxis = (sumAxis_ == 0) ? 1 : 0;
	if(axes_.at(0).size != inputSource_->size(otherAxis)) {
		axes_[0].size = inputSource_->size(otherAxis);
		cachedValues_.resize(axes_.at(0).size);	// resize() will fill in with default-constructed value for AMNumber(), which is AMNumber::Null. So this keeps cache invalid (if was previously invalid), and nulls the new values so they are calculated as required.
		emitSizeChanged(0);
	}
}

/// Connected to be called when the state() flags of any input source change
void AM2DSummingAB::onInputSourceStateChanged() {

	reviewState();

	// just in case the size has changed while the input source was invalid, and now it's going valid.  Do we need this? probably not, if the input source is well behaved. But it's pretty inexpensive to do it twice... and we know we'll get the size right everytime it goes valid.
	onInputSourceSizeChanged();
}

#include "analysis/AM2DSummingABEditor.h"
QWidget* AM2DSummingAB::createEditorWidget() {
	return new AM2DSummingABEditor(this);
}

Q_EXPORT_PLUGIN2(AM2DSummingAB, AM2DSummingAB)

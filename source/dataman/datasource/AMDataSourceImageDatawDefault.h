#ifndef AMDATASOURCEIMAGEDATAWDEFAULT_H
#define AMDATASOURCEIMAGEDATAWDEFAULT_H

#include <QObject>
#include "dataman/datasource/AMDataSourceImageData.h"

/// This class extends the standard AMDataSourceImageData to include a default value that is not used when computing the range.
/*!
	The purpose of this class is to provide a model for images that can give an accurate visual
	representation of the data even if there are preset data values.  As an example, the preset data values could
	be used for ensuring the image is the correct size before real data values have been added
	to the model.
  */
class AMDataSourceImageDatawDefault : public AMDataSourceImageData
{
	Q_OBJECT

public:
	/// Constructor. \param dataSource is the data source we wish to encapsulate and \param defaultValue is the value that is ignored when computing the range.
	AMDataSourceImageDatawDefault(const AMDataSource *dataSource, double defaultValue, QObject *parent = 0);

	/// Returns the default value.
	double defaultValue() const { return defaultValue_; }
	/// Sets the default value to \param value.
	void setDefaultValue(double value) { defaultValue_ = value; MPlotAbstractImageData::emitDataChanged(); }

protected:
	/// Searches for minimum z value
	virtual qreal minZ() const;

	/// The default value.
	qreal defaultValue_;
};

#endif // AMDATASOURCEIMAGEDATAWDEFAULT_H

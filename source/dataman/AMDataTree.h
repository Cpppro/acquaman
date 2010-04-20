#ifndef AMDATATREE_H
#define AMDATATREE_H

#include "AMErrorMonitor.h"

 #include <QSharedData>
#include <QHash>
#include <QVector>

/// The type of (numeric?) data stored. We're taking the overhead of using doubles for everything, for the simplicity of being able to accurately hold any numeric type.
typedef double AMNumericType;

/// This class is a protected support class for AMDataTree, enabling the "implicit sharing" (or "copy-on-write") of data trees.  This makes copying a tree (and passing it by value) fast until a deep copy is actually required.
class AMDataTreeProtectedData : public QSharedData
{
  public:
	/// Default constructor
	AMDataTreeProtectedData(unsigned count = 0, bool hasXValues = false) {
		count_ = count;
		if( (hasXValues_ = hasXValues) ) {
			x_.resize(count_);
		}
	}
	/// Required copy constructor for deep copies.  Although this is a "deep copy" at the AMDataTree level, the column vectors (QHash, QVector) in x_, and y_ also take advantage of implicit sharing, so they are copied quickly.
	AMDataTreeProtectedData(const AMDataTreeProtectedData &other)
		: QSharedData(other),
		  count_(other.count_),
		  x_(other.x_),
		  xName_(other.xName_),
		  hasXValues_(other.hasXValues_),
		  y_(other.y_),
		  yD_(other.yD_)	///< wrong
	{
		/// \todo here: deep copies of subtrees. (recursive?)
	}

	~AMDataTreeProtectedData() {
		/// \todo If we allocated subtrees trees, deletes them.
	}

	// Internal data:
	//////////////////////////////
	/// The number of datapoints (in any column) of this dataset
	int count_;

	/// The x values: values in the principal column of the dataset. Must be bare numbers.
	QVector<AMNumericType> x_;
	/// The name of the principal column (the independent variable that you scanned). ex: "eV" or "temp"
	QString xName_;
	/// Sometimes (for example: evenly spaced data, or pixel data) the index is all we need for an x-axis; there's no need to store actual x-axis values in memory. In this case, hasXValues_ = false.
	bool hasXValues_;

	/// A <columnName, columnData> map of the dependent data columns. The keys are the column names (ex: "tey", "tfy", "etc"). The values are a vector containing count() datapoints. These are single data points; higher-dimensionality data is stored in yD_;
	QHash< QString, QVector<AMNumericType> > y_;
	/// Higher-dimensional data is represented by referencing another AMDataTree for each datapoint.
	QHash< QString, QVector<AMDataTree*> > yD_;
};

/// This class is an attempt at supporting arbitrary-dimensionality data for AMScan objects, while maintaining simple (programmer-easy) and fast (high-performance) access to the data.
/*! Data must have a principal column (usually the "x" axis or main independent variable), and the values stored in this column must be true data values.
The dataset can have an arbitrary number of additional columns (ex: "tey_raw", "tfy_raw", and "sddSpectrums").  These columns can contain single datapoints (stored in \c y_) OR links to separate AMDataTables containing higher-dimensional data (stored in \c yD_).

For example, in the case of typical absorption scan data taken on the SGM beamline, "tey_raw" and "tfy_raw" would be columns of numbers stored in y_ and accessed as:
\code
AMDataTree myXasData;
/// The 6th element in the "tey" column:
printf( "(%d, %d)", myXasData.x(5), myXasData.value("tey", 5) );
\endcode

However, all of the datapoints in the "sddSpectrums" column would actually be links to other AMDataTables.  (In the case of the SDD (silicon drift detector), the linked tables would contain an x column (pixel value or energy value) and a y column of intensities.)
Externally, an actual data value would be accessed as:
\code
myXasData.more("sddSpectrums", 5)->value("sddIntensities", 512);
\endcode

Internally...
\code
///    v----------------------------------------- A vector of AMDataTrees (number of elements = count()
///                 v---------------------------- The tree for the SDD spectrum at the 6th data point (corresponding to an energy of myXasData.x_[5])
///                            v----------------- It has one column, the sdd detector intensities (across 1024 energy pixels)
///                                         v----- The intensity of the middle pixel of the SDD spectrum. (Finally, a number!)
yD_("sddSpectrums", 5)->y_["sddIntensity"][512]
\endcode




The number of datapoints (whether actual values or AMDataTable links) in any column must be count().

\todo Figure out memory management for subtrees. What happens if they're shared? Can they be shared?  Destroy subtrees in destructor? Note that QHashs and QVectors are implicitly shared... Can copy them without memory allocation as long as not changed.
\todo copy and assignment operators
*/

class AMDataTree {

public:
	/// Constructor. Creates a tree with just a single (x) column. If it has explicit x values (specify hasXValues = true), space is allocated but no initialization is done; all the x values will be 0.
	explicit AMDataTree(unsigned count = 0, bool hasXValues = false) {

		d_ = new AMDataTreeProtectedData(count, hasXValues);

	}

	/// Copy constructor. It implements implicit sharing, so this can be fast.
	/*!	The "copy-on-write" method is used: both AMDataTrees will share the same data columns in memory until one of them needs to modify it.


Consider the scenario of myXASData with an x column (eV), one y column (tey), and higher-dimension column of sddSpectrum. The sddSpectrum  subtrees each have an x- and y-column.

\code
copyXASData = myXASData;
(the column vectors eV and tey are shared implicitly.)
(the column of subtree pointers is shared implicitly, and with no changes. Both copyXASData and myXASData's sddSpectrum pointers are pointing to the same subtrees.)

copyXASData.more("sddSpectrums",5)->setValue("y", 512, 49.3);
(Now, we need to make a copy of the sddSpectrum subtree #5, with the new value 49.3 at index 512 in its 'y' column. This causes a deep copy of the 'y' column in the subtree; since the data is modified, it can't be implicitly shared anymore.)
(We also need to change the pointer in the sddSpectrums column (at index 5) to point to this new subtree. This causes a deep copy of the sddSpectrums column.)

*/
	AMDataTree(const AMDataTree& other)
		: d_ (other.d_)
	{

	}

	/// \todo Assignment operator. (Ditto on the implicit sharing.) Need to figure out allocation of subtrees.


	// Constant (access) member functions:
	////////////////////////////////////////

	/// The number of datapoints in each column
	unsigned count() const { return d_->count_; }

	/// Access the primary column by index
	AMNumericType x(unsigned i) const {

		if(i >= d_->count()) {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1, "AMDataTree: accessed value out of range. Returning an (incorrect) default value."));
			return AMNumericType();
		}

		if(d_->hasXValues_)
			return d_->x_.at(i);
		else
			return i;
	}


	/// Access the value in any column by name and index
	AMNumericType value(const QString& columnName, unsigned i) const {

		if(i >= d_->count()) {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1, "AMDataTree: accessed value out of range. Returning an (incorrect) default value."));
			return AMNumericType();
		}

		if(d_->y_.contains(columnName))
			return d_->y_[columnName].at(i);

		if(columnName == d_->xName_)
			return d_->x_.at(i);

		if(d_->yD_.contains(columnName)) {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -2, "AMDataTree: accessed multi-dimensional data as a single value. Returning an (incorrect) default value."));
			return AMNumericType();
		}

		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -3, "AMDataTree: accessed non-existent data column. Returning an (incorrect) default value."));
		return AMNumericType();
	}

	/// Access a pointer to higher-dimensional data tree. Returns 0 if column not found or index out of range.
	/*! The tree returned is a read-only pointer. Make a copy using the copy constructor or assignment operator if you need to change the data in it.
		\code
		AMDataTree myTopTree;
		AMDataTree* readOnlySubtree = myTopTree.more("sddSpectrums, 42);

		AMDataTree modifySubtree(*readOnlySubtree);
		// or...
		AMDataTree modifySubtree = *readOnlySubtree;
		\endcode
		*/
	const AMDataTree* more(const QString& columnName, unsigned i) const {

		if(i >= d_->count()) {
			return 0;
		}

		if(d_->yD_.contains(columnName)) {
			return d_->yD_[columnName].at(i);
		}

		return 0;
	}



	// Non-constant (modifier) member functions:
	////////////////////////////////////////

	/// \todo Add columns
	/// \todo set data
	/// \todo add subtrees (how, where in memory?)



signals:

public slots:

protected:
	/// Implicit shared pointer to actual member data:
	QSharedDataPointer<AMDataTreeProtectedData> d_;
	
};

#endif // AMDATATREE_H

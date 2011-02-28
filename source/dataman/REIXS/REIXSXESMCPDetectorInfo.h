#ifndef REIXSXESMCPDETECTORINFO_H
#define REIXSXESMCPDETECTORINFO_H

#include "dataman/AMDetectorInfo.h"

/// This class contains the run-time configuration parameters for the MCP detector in the REIXS XES Spectrometer
class REIXSXESMCPDetectorInfo : public AMDetectorInfo
{
	Q_OBJECT
	Q_PROPERTY(bool orientation READ orientation WRITE setOrientation)
	Q_PROPERTY(double biasVoltage READ biasVoltage WRITE setBiasVoltage)

	Q_CLASSINFO("AMDbObject_Attributes", "description=XES Imaging Detector")

public:
	/// Default constructor
	Q_INVOKABLE REIXSXESMCPDetectorInfo(const QString& name = "xesImage", const QString& description = "XES Imaging Detector", QObject *parent = 0);

	// Dimensionality and size:
	////////////////////////////////////

	/// Returns the number of dimensions in the output of this detector. A single point has rank 0. A spectrum output would have rank 1. An image output would have rank 2.
	virtual int rank() const { return 2;}
	/// Returns the size (ie: number of elements) along each dimension of the detector.  For a single-point detector, returns an empty AMnDIndex(). For a spectrum output, this would contain one number (the number of pixels or points along the axis).  For an image output, this would contain the width and height.
	virtual AMnDIndex size() const { return AMnDIndex(pixelsX_, pixelsY_); }
	/// Returns a list of AMAxisInfo describing the size and nature of each detector axis, in order.
	virtual QList<AMAxisInfo>  axes() const {
		QList<AMAxisInfo> rv;
		rv << AMAxisInfo("x", pixelsX_, "x - energy axis", "pixels");
		rv << AMAxisInfo("y", pixelsY_, "y - vertical axis", "pixels");
		return rv;
	}

	// Properties
	///////////////////////////
	/// Number of pixels across the detector image
	int pixelsX() const { return pixelsX_; }
	/// Number of pixels across the detector image (vertical, non-energy-sensing direction)
	int pixelsY() const { return pixelsY_; }
	/// Bias voltage (HV), in volts, applied across the detector
	double biasVoltage() const { return biasVoltage_; }
	/// The orientation of the detector: 0 for horizontal (wide window, low resolution), 1 for vertical (narrow window, high resolution)
	bool orientation() const { return orientation_; }

	/// Set the number of pixels across the detector image
	bool setSize(const AMnDIndex& newSize) { if(newSize.rank() != 2) return false; pixelsX_ = newSize.i(); pixelsY_ = newSize.j(); setModified(true); return true; }
	/// Bias voltage (HV), in volts, applied across the detector
	void setBiasVoltage(double biasVoltage) { biasVoltage_ = biasVoltage; setModified(true); }
	/// The orientation of the detector: 0 for horizontal (wide window, low resolution), 1 for vertical (narrow window, high resolution)
	void setOrientation(bool orientation) { orientation_ = orientation; setModified(true); }



signals:

public slots:

protected:
	/// Number of pixels across the detector image
	int pixelsX_, pixelsY_;
	/// Bias voltage (HV), in volts, applied across the detector
	double biasVoltage_;
	/// The orientation of the detector: 0 for horizontal (wide window, low resolution), 1 for vertical (narrow window, high resolution)
	bool orientation_;

};

#endif // REIXSXESMCPDETECTORINFO_H
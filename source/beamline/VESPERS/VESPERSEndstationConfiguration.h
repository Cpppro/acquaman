#ifndef VESPERSENDSTATIONCONFIGURATION_H
#define VESPERSENDSTATIONCONFIGURATION_H

#include <QObject>

#include <QPair>

/// This class handles the configuration of the beamline.  Based on it's state, various options for the beamline will become available.
/*!
	The basic concept is the user will choose a geometry for that will be used (single 45 degrees, double 45 degrees, etc.) and,
	based on the setup for the beamline remaining fairly static, offer possibilities for techniques and detectors.

	There are 5 geometries, 4 detectors, and 3 techniques that are offered and customizable.
  */
class VESPERSEndstationConfiguration : public QObject
{
	Q_OBJECT
public:
	/// Enum for choosing the geometry.
	/*!
		The following are the available choices for the geometry.  Note that the Big beam option is technically available, but most of the
		hardware for that part of the endstation is not available currently.  What will be used there is still the same though.

		- Invalid:	Used as the null or uninitialised value.
		- StraightOn:  This has the sample sitting perpendicular to the beam.  When using the sample stage, it uses the x and z motors and
					only has XAS and XRF mapping available.  The current setup has ion chambers and the single element vortex detector.
		- Single45Vertical:	This has the sample sitting at 45 degrees vertically such that reflection based diffraction patterns can be
					measured as well as any XRF or XAS measurements.  When using the sample stage, it uses the pseudo-motors H and V and
					has all of the techniques available to it (XAS, XRF, XRD).  The current setup has ion chambers, single element vortex
					detector, and the Roper CCD.
		- Single45Horizontal:  This has the sample sitting at 45 degrees horizontally.  When using the sample stage, it uses the x and z
					motors and only has XAS and XRF mapping available.  The current setup has ion chambers and the four element vortex
					detector.
		- Double45:	This has the sample sitting at 45 degrees both vertically and horizontally such that reflection based diffraction
					patterns can be measured as well as XRF or XAS measurements.  When using the sample stage, it uses the pseudo-motors
					H and V and has all of the techniques available to it (XAS, XRF, XRD).  The current setup has ion chambers, four element
					vortex detector, and the Roper CCD.
		- BigBeam:	This has the sample sitting upstream of the Pre-KB ion chamber.  This setup has a completely different sample stage
					and the only techniques available to it are XAS and XRF mapping (macro-probe mapping).  The current setup has ion chambers
					and the four element vortex detector.
	  */
	enum Geometry { Invalid = 0, StraightOn, Single45Vertical, Single45Horizontal, Double45, BigBeam };

	/// Constructor.
	explicit VESPERSEndstationConfiguration(QObject *parent = 0);

	/// Returns the current geometry.
	Geometry geometry() const { return current_; }

	// These getters return whether a particular piece of the endstation can be used or not.  These values change depending on the geometry.
	/// Returns whether the x and z sample stage can be used.
	bool canUseXAndZ() const { return xAndZ_.first; }
	/// Returns whether the h and v sample stage can be used.
	bool canUseHAndV() const { return hAndV_.first; }
	/// Returns whether the big beam sample stage can be used.
	bool canUseBigBeam() const { return bigBeam_.first; }
	/// Returns whether XAS can be performed.
	bool canUseXAS() const { return xas_.first; }
	/// Returns whether XRF can be performed.
	bool canUseXRF() const { return xrf_.first; }
	/// Returns whether XRD can be performed.
	bool canUseXRD() const { return xrd_.first; }
	/// Returns whether the ion chambers can be used.
	bool canUseIonChambers() const { return ionChambers_.first; }
	/// Returns whether the single element vortex detector can be used.
	bool canUseSingleElementVortex() const { return vortex1E_.first; }
	/// Returns whether the four element vortex detector can be used.
	bool canUseFourElementVortex() const { return vortex4E_.first; }
	/// Returns whether the Roper CCD can be used.
	bool canUseRoperCCD() const { return roperCCD_.first; }

	// These getters return whether the particular piece is actually in use.  Returns false if canUse() is false.
	/// Returns whether the x and z sample stage is in use.  Returns false if canUse is false.
	bool usingXAndZ() const { return xAndZ_.first && xAndZ_.second; }
	/// Returns whether the h and v sample stage is in use.  Returns false if canUse is false.
	bool usingHAndV() const { return hAndV_.first && hAndV_.second; }
	/// Return whether the big beam sample stage is in use.  Returns false if canUse is false.
	bool usingBigBeam() const { return bigBeam_.first && bigBeam_.second; }
	/// Returns whether XAS is the technique being performed.  Returns false if canUse is false.
	bool usingXAS() const { return xas_.first && xas_.second; }
	/// Returns whether XRF is the technique being performed.  Returns false if canUse is false.
	bool usingXRF() const { return xrf_.first && xrf_.second; }
	/// Returns whether XRD is the technique being performed.  Returns false if canUse is false.
	bool usingXRD() const { return xrd_.first && xrd_.second; }
	/// Returns whether the ion chambers are in use.  Returns false if canUse is false.
	bool usingIonChambers() const { return ionChambers_.first && ionChambers_.second; }
	/// Returns whether the single element vortex is in use.  Returns false if canUse is false.
	bool usingSingleElementVortex() const { return vortex1E_.first && vortex1E_.second; }
	/// Returns whether the four element vortex is in use.  Returns false if canUse is false.
	bool usingFourElementVortex() const { return vortex4E_.first && vortex4E_.second; }
	/// Returns whether the CCD is in use.  Returns false if canUse is false.
	bool usingRoperCCD() const { return roperCCD_.first && roperCCD_.second; }

signals:
	/// Notifier that the geometry changed when using setGeometry.  The selections on detector choice and technique choice are reset.
	void geometryChanged();
	/// Notifier that the configuration has changed.
	void configurationChanged();

	/// Notifier for whether the x and z sample stage is now in use.
	void usingXAndZChanged(bool);
	/// Notifier for whether the h and v sample stage is now in use.
	void usingHAndVChanged(bool);
	/// Notifier for whether the big beam sample stage is now in use.
	void usingBigBeamChanged(bool);
	/// Notifier about whether XAS will be used now.
	void usingXASChanged(bool);
	/// Notifier about whether XRF will be used now.
	void usingXRFChanged(bool);
	/// Notifier about whether XRD will be used now.
	void usingXRDChanged(bool);
	/// Notifier about whether the ion chambers are now in use.
	void usingIonChambersChanged(bool);
	/// Notifier about whether the single element vortex is now in use.
	void usingSingleElementVortexChanged(bool);
	/// Notifier about whether the four element vortex is now in use.
	void usingFourElementVortexChanged(bool);
	/// Notifier about whether the Roper CCD is now in use.
	void usingRoperCCDChanged(bool);

public slots:
	/// Sets the geometry.  Assigns the appropriate values on whether a piece can be used or not.
	void setGeometry(Geometry newGeometry);

	/// Sets whether the x and z sample stage will be used.  Sets it to false automatically if canUse is false.
	void setUsingXAndZ(bool use);
	/// Sets whether the h and v sample stage will be used.  Sets it to false automatically if canUse is false.
	void setUsingHAndV(bool use);
	/// Sets whether the big beam sample stage will be used.  Sets it to false automatically if canUse is false.
	void setUsingBigBeam(bool use);
	/// Sets whether the XAS technique will be used.  Sets it to false automatically if canUse is false.
	void setUsingXAS(bool use);
	/// Sets whether the XRF technique will be used.  Sets it to false automatically if canUse is false.
	void setUsingXRF(bool use);
	/// Sets whether the XRD technique will be used.  Sets it to false automatically if canUse is false.
	void setUsingXRD(bool use);
	/// Sets whether the ion chambers will be used.  Sets it to false automatically if canUse is false.
	void setUsingIonChambers(bool use);
	/// Sets whether the single element vortex detector will be used.  Sets it to false automatically if canUse is false.
	void setUsingSingleElementVortex(bool use);
	/// Sets whether the four element vortex detector will be used.  Sets it to false automatically if canUse is false.
	void setUsingFourElementVortex(bool use);
	/// Sets whether the Roper CCD will be used.  Sets it to false automatically if canUse is false.
	void setUsingRoperCCD(bool use);

protected:
	/// This holds what the current desired geometry is.
	Geometry current_;

	/// The following are pairs of booleans where the first one is whether or not the value can be changed, and the second is whether or not the value is valid or not.
	/// Pair that handles whether the X and Z sample stage can be used.
	QPair<bool, bool> xAndZ_;
	/// Pair that handles whether the H and V sample stage can be used.
	QPair<bool, bool> hAndV_;
	/// Pair that handles whether the big beam sample stage can be used.
	QPair<bool, bool> bigBeam_;
	/// Pair that handles whether XAS can be used.
	QPair<bool, bool> xas_;
	/// Pair that handles whether XRF mapping can be done.
	QPair<bool, bool> xrf_;
	/// Pair that handles whether XRD can be done.
	QPair<bool, bool> xrd_;
	/// Pair that handles whether the ion chambers can be used.
	QPair<bool, bool> ionChambers_;
	/// Pair that handles whether the single element vortex can be used.
	QPair<bool, bool> vortex1E_;
	/// Pair that handles whether the four element vortex can be used.
	QPair<bool, bool> vortex4E_;
	/// Pair that handles whether the Roper CCD can be used.
	QPair<bool, bool> roperCCD_;
};

#endif // VESPERSENDSTATIONCONFIGURATION_H
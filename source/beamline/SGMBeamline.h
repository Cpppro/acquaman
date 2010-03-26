#ifndef SGMBEAMLINE_H
#define SGMBEAMLINE_H

#include "Control.h"
#include "PVNames.h"

class SGMBeamline : public Control
{
    Q_OBJECT

public:
    enum sgmGrating {low=0, medium=1, high=2};

    static SGMBeamline* sgm();		// singleton-class accessor
    static void releaseSGM();	// releases memory for Beamline

    virtual ~SGMBeamline();

    // What does this Beamline have? (These objects will be useful in the scripting world too!)
    ///////////////////////////////////

    ReadOnlyPVControl* ringCurrent() const { return ringCurrent_; }
    PVControl* energy() const { return energy_;}
    PVControl* exitSlitGap() const { return exitSlitGap_;}
    ReadOnlyPVControl* m4() const { return m4_;}
    PVControl* grating() const { return grating_;}

protected:
    // Singleton implementation:
    SGMBeamline();					// protected constructor... only access through Beamline::bl()
    static SGMBeamline* instance_;

    // Parts of this beamline:
    ///////////////////////////////

    ReadOnlyPVControl* ringCurrent_;
    PVControl *energy_;
    PVControl *exitSlitGap_;
    ReadOnlyPVControl* m4_;
    PVControl *grating_;
};

#endif // SGMBEAMLINE_H

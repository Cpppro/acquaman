#ifndef VESPERSIONCHAMBERVIEW_H
#define VESPERSIONCHAMBERVIEW_H

#include <QWidget>
#include <QComboBox>

#include "beamline/VESPERS/VESPERSIonChamber.h"

/// This class is a generic view that is very similar to the current view in the old user interface.  It has combo boxes for determining the sensitivity and a feedback value for the count rate.
class VESPERSIonChamberView : public QWidget
{
	Q_OBJECT

public:
	/// Constructor.  Takes in a VESPERSIonChamber and builds a view around it.
	explicit VESPERSIonChamberView(VESPERSIonChamber *ionChamber, QWidget *parent = 0);

signals:

public slots:

protected slots:
	/// Handles passing changes in the value combo box to the ion chamber.
	void onValueComboBoxChanged(int index) { ionChamber_->setSensitivityValue(index);}
	/// Handles passing changes in the units combo box to the ion chamber.
	void onUnitsComboBoxChanged(int index) { ionChamber_->setSensitivityUnits(units_->itemText(index)); }

	/// Handles setting the value combo box when the ion chamber is changed from elsewhere.
	void onValueChanged(int value);
	/// Handles setting the units combo box when the ion chamber is changed from elsewhere.
	void onUnitsChanged(QString units);

protected:
	/// Pointer to the ion chamber being viewed.
	VESPERSIonChamber *ionChamber_;

	/// Combo box holding the value for the sensitivity.
	QComboBox *value_;
	/// Combo box holding the units for the sensitivity.
	QComboBox *units_;

};

#endif // VESPERSIONCHAMBERVIEW_H
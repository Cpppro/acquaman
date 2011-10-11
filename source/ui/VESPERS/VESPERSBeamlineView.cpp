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


#include "VESPERSBeamlineView.h"

#include "beamline/VESPERS/VESPERSBeamline.h"
#include "ui/VESPERS/VESPERSIonChamberCalibrationView.h"
#include "ui/VESPERS/VESPERSIonChamberView.h"
#include "ui/CLS/CLSSynchronizedDwellTimeView.h"
#include "ui/AMTopFrame.h"

#include "beamline/CLS/CLSIonChamber.h"
#include "ui/beamline/AMIonChamberView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

VESPERSBeamlineView::VESPERSBeamlineView(QWidget *parent) :
	QWidget(parent)
{
	AMTopFrame *top = new AMTopFrame("Beamline Components");
	top->setIcon(QIcon(":/system-software-update.png"));

	QVBoxLayout *basicLayout = new QVBoxLayout;
	basicLayout->addWidget(new VESPERSSplitIonChamberBasicView(VESPERSBeamline::vespers()->ionChamberCalibration()->splitIonChamber(), 2, 4.5));

	for (int i = 0; i < VESPERSBeamline::vespers()->ionChamberCalibration()->ionChamberCount(); i++)
		basicLayout->addWidget(new VESPERSIonChamberBasicView(VESPERSBeamline::vespers()->ionChamberCalibration()->ionChamberAt(i), 2, 4.5));

	VESPERSIonChamberCalibrationView *ionCalibrationView = new VESPERSIonChamberCalibrationView(VESPERSBeamline::vespers()->ionChamberCalibration());
	CLSSynchronizedDwellTimeView *dwellTimeView = new CLSSynchronizedDwellTimeView(VESPERSBeamline::vespers()->synchronizedDwellTime());




	QVBoxLayout *current = new QVBoxLayout;
	current->addLayout(basicLayout);
	current->addWidget(ionCalibrationView, 0, Qt::AlignCenter);
	AMIonChamber *amic = new CLSIonChamber("Pre-KB", "Pre-KB", "BL1607-B2-1:mcs07:fbk", "BL1607-B2-1:mcs07:userRate", "AMP1607-204:sens_num.VAL", "AMP1607-204:sens_unit.VAL", this);
	amic->setVoltagRange(2, 4.5);
	current->addWidget(new AMIonChamberView(amic));

	QHBoxLayout *next = new QHBoxLayout;
	next->addStretch();
	next->addLayout(current);
	next->addWidget(dwellTimeView);
	next->addStretch();

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(top);
	mainLayout->addStretch();
	mainLayout->addLayout(next);
	mainLayout->addStretch();

	setLayout(mainLayout);
}

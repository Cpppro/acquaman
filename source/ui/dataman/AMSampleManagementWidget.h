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


#ifndef AMSAMPLEPOSITIONVIEW_H
#define AMSAMPLEPOSITIONVIEW_H

#include <QWidget>

class QGridLayout;
class QUrl;
class QGroupBox;

class AMTopFrame;

#include "AMSamplePlateView.h"
#include "AMSampleManipulatorView.h"
// #include "ui/AMBeamlineCameraWidget.h"


/// This widget provides a complete full-screen view for users to view, move, align, and tag samples in the machine.  You must provide it with a pointer to a manipulator widget (for moving the samples), a URL for the video stream from the sample camera, and a pointer to the sample plate object that we'll tag samples on.
class AMSampleManagementWidget : public QWidget
{
Q_OBJECT
public:
	explicit AMSampleManagementWidget(AMSampleManipulatorView *manipulatorView, const QUrl& sampleCameraUrl, AMSamplePlate* samplePlate, QWidget *parent = 0);

signals:
	void newSamplePlateSelected(AMSamplePlate *selectedPlate);

public slots:
	void onNewSamplePlateSelected();

protected:
// 	AMBeamlineCameraWidget *cam_;
	AMSamplePlateView *plateView_;
	AMSampleManipulatorView *manipulatorView_;

	AMTopFrame *topFrame_;

	QGridLayout *gl_;
};



#endif // AMSAMPLEPOSITIONVIEW_H
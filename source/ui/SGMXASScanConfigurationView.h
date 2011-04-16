/*
Copyright 2010, 2011 Mark Boots, David Chevrier.

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


#ifndef ACQMAN_SGMXASScanConfigurationViewer_H
#define ACQMAN_SGMXASScanConfigurationViewer_H

#include <QtGui>
#include "ui_SGMXASScanConfigurationViewer.h"
#include "AMControlSetView.h"
#include "AMDetectorSetView.h"
#include "AMXASRegionsView.h"
#include "AMRegionsLineView.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include "acquaman/SGM/SGMXASScanConfiguration.h"
#include "acquaman/SGM/SGMXASDacqScanController.h"
#include "AMScanConfigurationView.h"
#include "ui/AMControlOptimizationView.h"

#include "AMDetectorView.h"

class SGMFluxResolutionPickerView;

class SGMXASScanConfigurationView : public AMScanConfigurationView{
Q_OBJECT
public:
		SGMXASScanConfigurationView(SGMXASScanConfiguration *sxsc, QWidget *parent = 0);
		~SGMXASScanConfigurationView();

		const AMScanConfiguration* configuration() const;

protected slots:
	void onRegionsChanged(){
		/* NTBA March 14, 2011 David Chevrier
		if(cfg_ && fluxResolutionView_){
			fluxResolutionView_->onRegionsUpdate( ((SGMXASScanConfiguration*)cfg_)->regions() );
		}
		*/
	}

	void onDetectorConfigurationsChanged();

	void onSGMBeamlineCriticalControlsConnectedChanged();

protected:
	SGMXASScanConfiguration *cfg_;

	AMXASRegionsView *regionsView_;
	AMRegionsLineView *regionsLineView_;
	//AMCompactControlOptimizationSetView *fluxResolutionView_;
	SGMFluxResolutionPickerView *fluxResolutionView_;
	AMControlSetView *trackingView_;
	AMDetectorSetView *xasDetectorsView_;

	QLabel *warningsLabel_;

	QVBoxLayout *mainVL_;
	QGridLayout *bottomGL_;
};

class SGMFluxResolutionPickerView : public QGroupBox{
Q_OBJECT
public:
	SGMFluxResolutionPickerView(AMXASRegionsList *regions, QWidget *parent = 0);

public slots:
	void setFromInfoList(const AMControlInfoList &infoList);

protected slots:
	void onRegionsChanged();
	void onSetpointsChanged();

	void onBestFluxButtonClicked();
	void onBestResolutionButtonClicked();

signals:
	void configValuesChanged(AMControlInfoList);

protected:
	AMXASRegionsList *regions_;
	double minEnergy_;
	double maxEnergy_;

	QPushButton *bestFluxButton_;
	QPushButton *bestResolutionButton_;
	AMControlEditor *exitSlitGapCE_;
	AMControlEditor *gratingCE_;
	AMControlEditor *harmonicCE_;
	QLabel *warningsLabel_;

	QVBoxLayout *buttonsVL_;
	QVBoxLayout *ceVL_;
	QHBoxLayout *settingsHL_;
	QVBoxLayout *mainVL_;

};

#endif // SGMXASSCANCONFIGURATIONVIEWER_H

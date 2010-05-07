#ifndef ACQMAN_SGMXASScanConfigurationViewer_H
#define ACQMAN_SGMXASScanConfigurationViewer_H

#include <QtGui>
#include "ui_SGMXASScanConfigurationViewer.h"
#include "AMControlSetView.h"
#include "AMXASRegionsView.h"
#include <QPushButton>
#include <QVBoxLayout>
#include "acquaman/SGM/SGMXASScanConfiguration.h"
#include "acquaman/SGM/SGMXASDacqScanController.h"

/*
#include "../MPlot/src/MPlot/MPlotWidget.h"
#include "../MPlot/src/MPlot/MPlotSeriesData.h"
#include "../MPlot/src/MPlot/MPlotSeries.h"
#include "../MPlot/src/MPlot/MPlotImageData.h"
#include "../MPlot/src/MPlot/MPlotImage.h"
#include "../MPlot/src/MPlot/MPlotTools.h"

#include <QTableView>
#include <QPen>
#include <QBrush>
*/

class SGMXASScanConfigurationViewer : public QWidget, private Ui::SGMXASScanConfigurationViewer {
Q_OBJECT
public:
		SGMXASScanConfigurationViewer(QWidget *parent = 0) : QWidget(parent){
			setupUi(this);
			connect(doLayoutButton, SIGNAL(clicked()), this, SLOT(onDoLayout()));
		}

signals:
	void scanProgress(double elapsed, double total);
	void scanControllerReady(AMScanController *xasCtrl);

public slots:
	void setScanConfiguration(AMScanConfiguration *cfg){
		cfg_ = cfg;
		SGMXASScanConfiguration *sxsc = (SGMXASScanConfiguration*)cfg_;
		sxsc->addRegion(0, 270, 1, 280);
		sxsc->addRegion(1, 280.25, 0.25, 290);
		sxsc->addRegion(2, 290, 1, 320);
/*		sxsc->addRegion(0, 620, 1, 630);
		sxsc->addRegion(1, 630.5, 0.5, 640);
		sxsc->addRegion(2, 640, 3, 700);
		*/

//		setFluxResolutionSet(sxsc->fluxResolutionSet());
		regionsView_ = new AMXASRegionsView(sxsc->regionsPtr(), this);
		fluxResolutionView_ = new AMControlOptimizationSetView((AMControlOptimizationSet*)(sxsc->fluxResolutionSet()), this);
		QList<AMXASRegion*> oldRegions = sxsc->regions();
		QList<AMRegion*> newRegions;
		AMRegion* tmpRegion;
		for(int x = 0; x < oldRegions.count(); x++){
			tmpRegion = oldRegions.at(x);
			newRegions << tmpRegion;
		}
		fluxResolutionView_->onRegionsUpdate(newRegions);
		trackingView_ = new AMControlSetView(sxsc->trackingSet(), this);
		startScanButton_ = new QPushButton();
		startScanButton_->setText("Start Scan");
		startScanButton_->setMaximumWidth(200);
		connect(startScanButton_, SIGNAL(clicked()), this, SLOT(onStartScanClicked()));
		disconnect(doLayoutButton, SIGNAL(clicked()), this, SLOT(onDoLayout()));
		delete doLayoutButton;
		delete layout();
		vl_.addWidget(regionsView_);
		vl_.addWidget(fluxResolutionView_);
		vl_.addWidget(trackingView_);
		vl_.addWidget(startScanButton_);
		this->setLayout(&vl_);
	}

//	void setFluxResolutionSet(AMControlSet *viewSet){
//		fluxResolutionView_ = new AMControlSetView(viewSet, this);
//		delete doLayoutButton;
//		delete layout();
//		hl_.addWidget(fluxResolutionView_);
//		this->setLayout(&hl_);
//	}

protected slots:
	void onDoLayout(){
		if(!SGMBeamline::sgm()->isConnected())
			return;
		AMScanConfiguration *sxsc = new SGMXASScanConfiguration(this);
		setScanConfiguration(sxsc);
	}

	void onStartScanClicked(){
		SGMXASDacqScanController *xasCtrl = new SGMXASDacqScanController((SGMXASScanConfiguration*)cfg_, SGMBeamline::sgm());
		qDebug() << "I want to say scan controller is ready";
		emit scanControllerReady((AMScanController*)xasCtrl);

//		ctrl = new SGMXASDacqScanController(((SGMXASScanConfiguration*)cfg), SGMBeamline::sgm());
		connect(xasCtrl, SIGNAL(progress(double,double)), this, SIGNAL(scanProgress(double,double)));
//		return;

		xasCtrl->initialize();
		xasCtrl->start();
	}

protected:
	AMScanConfiguration *cfg_;
	AMXASRegionsView *regionsView_;
	AMControlOptimizationSetView *fluxResolutionView_;
	AMControlSetView *trackingView_;
	QPushButton *startScanButton_;
	QVBoxLayout vl_;
};

#endif // SGMXASSCANCONFIGURATIONVIEWER_H

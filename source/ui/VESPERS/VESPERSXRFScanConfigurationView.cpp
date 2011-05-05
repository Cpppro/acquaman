#include "VESPERSXRFScanConfigurationView.h"
#include "beamline/VESPERS/VESPERSBeamline.h"
#include "acquaman/VESPERS/VESPERSXRFScanController.h"
#include "ui/AMTopFrame.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>

VESPERSXRFScanConfigurationView::VESPERSXRFScanConfigurationView(VESPERSXRFScanConfiguration *scanConfig, QWidget *parent)
	: AMScanConfigurationView(parent)
{
	configuration_ = scanConfig;

	AMTopFrame *topFrame;

	if (configuration_->detectorChoice() == VESPERSBeamline::SingleElement){

		detector_ = VESPERSBeamline::vespers()->vortexXRF1E();
		topFrame = new AMTopFrame("XRF Configuration - Single Element Vortex");
	}
	else{

		detector_ = VESPERSBeamline::vespers()->vortexXRF4E();
		topFrame = new AMTopFrame("XRF Configuration - Four Element Vortex");
	}

	topFrame->setIcon(QIcon(":/utilities-system-monitor.png"));

	view_ = new XRFDetailedDetectorView(detector_);
	connect(detector_, SIGNAL(detectorConnected(bool)), this, SLOT(setEnabled(bool)));
	connect(detector_, SIGNAL(roisHaveValues(bool)), this, SLOT(onRoisHaveValues(bool)));

	// Using Potassium Ka as the lower energy limit and the maximum energy of the detector for the upper energy limit.
	selectionView_ = new XRFSelectionView(AMPeriodicTable::table()->elementBySymbol("K")->Kalpha().second.toDouble(), detector_->maximumEnergy()*1000);
	connect(selectionView_, SIGNAL(elementSelected(AMElement*)), view_, SLOT(showEmissionLines(AMElement*)));
	connect(selectionView_, SIGNAL(elementSelected(AMElement*)), view_, SLOT(highlightMarkers(AMElement*)));
	connect(selectionView_, SIGNAL(addRegionOfInterest(AMElement*,QPair<QString,QString>)), view_, SLOT(onAdditionOfRegionOfInterest(AMElement*,QPair<QString,QString>)));
	connect(selectionView_, SIGNAL(removeRegionOfInterest(AMElement*,QPair<QString,QString>)), view_, SLOT(onRemovalOfRegionOfInterest(AMElement*,QPair<QString,QString>)));
	connect(this, SIGNAL(roiExistsAlready(AMElement*,QPair<QString,QString>)), selectionView_, SLOT(preExistingRegionOfInterest(AMElement*,QPair<QString,QString>)));

	connect(selectionView_, SIGNAL(clearAllRegionsOfInterest()), view_, SLOT(removeAllRegionsOfInterest()));

	QToolButton *start = new QToolButton;
	start->setIcon(QIcon(":/play_button_green.png"));
	connect(start, SIGNAL(clicked()), this, SLOT(onStartClicked()));

	QToolButton *stop = new QToolButton;
	stop->setIcon(QIcon(":/red-stop-button.png"));
	connect(stop, SIGNAL(clicked()), this, SLOT(onStopClicked()));

	integrationTime_ = new QDoubleSpinBox;
	integrationTime_->setSuffix(" s");
	integrationTime_->setSingleStep(0.1);
	integrationTime_->setMaximum(1000.00);
	integrationTime_->setAlignment(Qt::AlignCenter);
	connect(integrationTime_, SIGNAL(editingFinished()), this, SLOT(onIntegrationTimeUpdate()));
	connect(detector_->integrationTimeControl(), SIGNAL(valueChanged(double)), integrationTime_, SLOT(setValue(double)));

	maxEnergy_ = new QDoubleSpinBox;
	maxEnergy_->setSuffix(" keV");
	maxEnergy_->setSingleStep(0.01);
	maxEnergy_->setMaximum(30.00);
	maxEnergy_->setAlignment(Qt::AlignCenter);
	connect(maxEnergy_, SIGNAL(editingFinished()), this, SLOT(onMaximumEnergyUpdate()));
	connect(detector_->maximumEnergyControl(), SIGNAL(valueChanged(double)), maxEnergy_, SLOT(setValue(double)));
	connect(detector_->maximumEnergyControl(), SIGNAL(valueChanged(double)), this, SLOT(onMaximumEnergyControlUpdate(double)));

	peakingTime_ = new QDoubleSpinBox;
	peakingTime_->setSuffix(QString::fromUtf8(" μs"));
	peakingTime_->setSingleStep(0.01);
	peakingTime_->setMaximum(100);
	peakingTime_->setAlignment(Qt::AlignCenter);
	connect(peakingTime_, SIGNAL(editingFinished()), this, SLOT(onPeakingTimeUpdate()));
	connect(detector_->peakingTimeControl(), SIGNAL(valueChanged(double)), peakingTime_, SLOT(setValue(double)));

	QFont font(this->font());
	font.setBold(true);

	QLabel *startLabel = new QLabel("Start & Stop");
	startLabel->setFont(font);
	QLabel *timeLabel = new QLabel("Real Time");
	timeLabel->setFont(font);
	QLabel *maxEnergyLabel = new QLabel("Max. Energy");
	maxEnergyLabel->setFont(font);
	QLabel *peakingTimeLabel = new QLabel("Peaking Time");
	peakingTimeLabel->setFont(font);

	QHBoxLayout *startAndStopLayout = new QHBoxLayout;
	startAndStopLayout->addWidget(start);
	startAndStopLayout->addWidget(stop);

	QVBoxLayout *controlLayout = new QVBoxLayout;
	controlLayout->addSpacing(20);
	controlLayout->addWidget(startLabel);
	controlLayout->addLayout(startAndStopLayout);
	controlLayout->addWidget(timeLabel);
	controlLayout->addWidget(integrationTime_);
	controlLayout->addWidget(maxEnergyLabel);
	controlLayout->addWidget(maxEnergy_);
	controlLayout->addWidget(peakingTimeLabel);
	controlLayout->addWidget(peakingTime_);
	controlLayout->addStretch();

	QVBoxLayout *viewAndSelectionLayout = new QVBoxLayout;
	viewAndSelectionLayout->addWidget(view_);
	viewAndSelectionLayout->addWidget(selectionView_);

	QHBoxLayout *plotControlLayout = new QHBoxLayout;
	plotControlLayout->addLayout(viewAndSelectionLayout);
	plotControlLayout->addLayout(controlLayout);

	QVBoxLayout *masterLayout = new QVBoxLayout;
	masterLayout->addWidget(topFrame);
	masterLayout->addLayout(plotControlLayout);

	setLayout(masterLayout);
}

void VESPERSXRFScanConfigurationView::onIntegrationTimeUpdate()
{
	detector_->setTime(integrationTime_->value());
}

void VESPERSXRFScanConfigurationView::onMaximumEnergyUpdate()
{
	detector_->setMaximumEnergyControl(maxEnergy_->value());
	selectionView_->setMaximumEnergy(maxEnergy_->value()*1000);
}

void VESPERSXRFScanConfigurationView::onMaximumEnergyControlUpdate(double val)
{
	selectionView_->setMaximumEnergy(val*1000);
}

void VESPERSXRFScanConfigurationView::onPeakingTimeUpdate()
{
	detector_->setPeakingTimeControl(peakingTime_->value());
}

void VESPERSXRFScanConfigurationView::onStopClicked()
{
	VESPERSXRFScanController *current = qobject_cast<VESPERSXRFScanController *>(AMScanControllerSupervisor::scanControllerSupervisor()->currentScanController());

	if (current)
		current->finish();
}

void VESPERSXRFScanConfigurationView::onRoisHaveValues(bool hasValues)
{
	if (hasValues){

		// Go through all the regions of interest PVs and if there are any regions set already, pass them on to the rest of the program.
		QString name;
		double low;
		double high;
		AMElement *el;

		for (int i = 0; i < detector_->roiList().count(); i++){

			name = detector_->roiList().at(i)->name();

			// If the name is empty then we've reached the end of the road for preset regions of interest.
			if (name.isEmpty()){

				el = AMPeriodicTable::table()->elementBySymbol("Fe");
				view_->showEmissionLines(el);
				view_->highlightMarkers(el);
				view_->resizeRoiMarkers();
				selectionView_->setElementView(el);
				return;
			}

			name = name.left(name.indexOf(" "));
			el = AMPeriodicTable::table()->elementBySymbol(name);

			if (el){

				low = detector_->roiList().at(i)->low();
				high = detector_->roiList().at(i)->high();

				for (int j = 0; j < el->emissionLines().count(); j++){

					if (el->emissionLines().at(j).first.contains("1")
							&& fabs((low+high)/2 - el->emissionLines().at(j).second.toDouble()/detector_->scale()) < 1)
						emit roiExistsAlready(el, el->emissionLines().at(j));
				}
			}
		}
	}
}

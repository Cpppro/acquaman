#include "VESPERSEXAFSScanConfigurationView.h"
#include "ui/AMTopFrame.h"
#include "ui/util/AMPeriodicTableDialog.h"
#include "util/AMPeriodicTable.h"
#include "beamline/VESPERS/VESPERSBeamline.h"
#include "util/VESPERS/GeneralUtilities.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>

VESPERSEXAFSScanConfigurationView::VESPERSEXAFSScanConfigurationView(VESPERSEXAFSScanConfiguration *config, QWidget *parent)
	: AMScanConfigurationView(parent)
{
	config_ = config;
	AMTopFrame *frame = new AMTopFrame("VESPERS EXAFS Configuration");

	// Regions setup
	regionsView_ = new AMRegionsView(config_->regions());
	regionsLineView_ = new AMEXAFSLineView(config_->exafsRegions());

	// The fluorescence detector setup
	QButtonGroup *fluorescenceButtonGroup = new QButtonGroup;
	QRadioButton *tempButton;
	QVBoxLayout *fluorescenceDetectorLayout = new QVBoxLayout;

	tempButton = new QRadioButton("None");
	fluorescenceButtonGroup->addButton(tempButton, 0);
	fluorescenceDetectorLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Single Element Vortex");
	fluorescenceButtonGroup->addButton(tempButton, 1);
	fluorescenceDetectorLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Four Element Vortex");
	fluorescenceButtonGroup->addButton(tempButton, 2);
	fluorescenceDetectorLayout->addWidget(tempButton);
	connect(fluorescenceButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onFluorescenceChoiceChanged(int)));

	fluorescenceButtonGroup->button((int)config_->fluorescenceDetectorChoice())->setChecked(true);

	QGroupBox *fluorescenceDetectorGroupBox = new QGroupBox("Fluorescence Detector");
	fluorescenceDetectorGroupBox->setLayout(fluorescenceDetectorLayout);

	// Ion chamber selection
	QVBoxLayout *ItGroupLayout = new QVBoxLayout;

	ItGroup_ = new QButtonGroup;
	tempButton = new QRadioButton("Isplit");
	tempButton->setEnabled(false);
	tempButton->hide();
	ItGroup_->addButton(tempButton, 0);
	ItGroupLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Iprekb");
	tempButton->hide();
	ItGroup_->addButton(tempButton, 1);
	ItGroupLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Imini");
	tempButton->hide();
	ItGroup_->addButton(tempButton, 2);
	ItGroupLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Ipost");
	tempButton->hide();
	ItGroup_->addButton(tempButton, 3);
	ItGroupLayout->addWidget(tempButton);
	connect(ItGroup_, SIGNAL(buttonClicked(int)), this, SLOT(onItClicked(int)));

	QVBoxLayout *I0GroupLayout = new QVBoxLayout;

	I0Group_ = new QButtonGroup;
	tempButton = new QRadioButton("Isplit");
	I0Group_->addButton(tempButton, 0);
	I0GroupLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Iprekb");
	I0Group_->addButton(tempButton, 1);
	I0GroupLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Imini");
	tempButton->setChecked(true);
	I0Group_->addButton(tempButton, 2);
	I0GroupLayout->addWidget(tempButton);
	tempButton = new QRadioButton("Ipost");
	I0Group_->addButton(tempButton, 3);
	I0GroupLayout->addWidget(tempButton);
	connect(I0Group_, SIGNAL(buttonClicked(int)), this, SLOT(onI0Clicked(int)));

	QHBoxLayout *ItI0Layout = new QHBoxLayout;

	QButtonGroup *ItI0ButtonGroup = new QButtonGroup;
	QToolButton *ItI0temp = new QToolButton;
	ItI0temp->setText("I0");
	ItI0temp->setCheckable(true);
	ItI0ButtonGroup->addButton(ItI0temp, 0);
	ItI0Layout->addWidget(ItI0temp);
	ItI0temp->setChecked(true);
	ItI0temp = new QToolButton;
	ItI0temp->setText("It");
	ItI0temp->setCheckable(true);
	ItI0ButtonGroup->addButton(ItI0temp, 1);
	ItI0Layout->addWidget(ItI0temp);
	ItI0Layout->setSpacing(0);
	ItI0Layout->addStretch();
	connect(ItI0ButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onItI0Toggled(int)));

	I0Group_->button((int)config_->incomingChoice())->click();
	ItGroup_->button((int)config_->transmissionChoice())->click();

	QHBoxLayout *ionChambersLayout = new QHBoxLayout;
	ionChambersLayout->addLayout(I0GroupLayout);
	ionChambersLayout->addLayout(ItGroupLayout);
	QVBoxLayout *ionChambersGroupBoxLayout = new QVBoxLayout;
	ionChambersGroupBoxLayout->addLayout(ItI0Layout);
	ionChambersGroupBoxLayout->addLayout(ionChambersLayout);

	QGroupBox *ionChambersGroupBox = new QGroupBox("Ion Chambers");
	ionChambersGroupBox->setLayout(ionChambersGroupBoxLayout);

	// Scan name selection
	scanName_ = new QLineEdit;
	scanName_->setText(config_->name());
	scanName_->setAlignment(Qt::AlignCenter);
	connect(scanName_, SIGNAL(editingFinished()), this, SLOT(onScanNameEdited()));
	onScanNameEdited();

	QFormLayout *scanNameLayout = new QFormLayout;
	scanNameLayout->addRow("Scan Name:", scanName_);

	// Energy (Eo) selection
	energy_ = new QDoubleSpinBox;
	energy_->setSuffix(" eV");
	energy_->setMinimum(0);
	energy_->setMaximum(30000);
	connect(energy_, SIGNAL(editingFinished()), this, SLOT(setEnergy()));

	elementChoice_ = new QToolButton;
	connect(elementChoice_, SIGNAL(clicked()), this, SLOT(onElementChoiceClicked()));

	lineChoice_ = new QComboBox;
	connect(lineChoice_, SIGNAL(currentIndexChanged(int)), this, SLOT(onLinesComboBoxIndexChanged(int)));

	if (config_->edge().isEmpty()){

		elementChoice_->setText("Cu");
		fillLinesComboBox(AMPeriodicTable::table()->elementBySymbol("Cu"));
		onLinesComboBoxIndexChanged(0);
	}
	// Resets the view for the view to what it should be.  Using the saved for the energy in case it is different from the original line energy.
	else {

		double energy = config_->energy();
		QString edgeName(config_->edge());

		elementChoice_->setText(config_->edge().split(" ").first());
		fillLinesComboBox(AMPeriodicTable::table()->elementBySymbol(elementChoice_->text()));
		config_->setEdge(edgeName);
		lineChoice_->setCurrentIndex(lineChoice_->findText(config_->edge().split(" ").last(), Qt::MatchStartsWith | Qt::MatchCaseSensitive));

		if (energy_->value() != energy)
			energy_->setValue(energy);
	}

	QCheckBox *useFixedTime = new QCheckBox("Use fixed time (EXAFS)");
	useFixedTime->setEnabled(config_->useFixedTime());
	connect(config_->exafsRegions(), SIGNAL(regionsHaveKSpaceChanged(bool)), useFixedTime, SLOT(setEnabled(bool)));
	connect(useFixedTime, SIGNAL(toggled(bool)), config_, SLOT(setUseFixedTime(bool)));

	QSpinBox *numberOfScans = new QSpinBox;
	numberOfScans->setMinimum(1);
	numberOfScans->setValue(config_->numberOfScans());
	numberOfScans->setAlignment(Qt::AlignCenter);
	connect(numberOfScans, SIGNAL(valueChanged(int)), config_, SLOT(setNumberOfScans(int)));
	connect(config_, SIGNAL(numberOfScansChanged(int)), this, SLOT(onEstimatedTimeChanged()));

	QFormLayout *numberOfScansLayout = new QFormLayout;
	numberOfScansLayout->addRow("Number of Scans:", numberOfScans);

	QFormLayout *energySetpointLayout = new QFormLayout;
	energySetpointLayout->addRow("Energy:", energy_);

	QHBoxLayout *energyLayout = new QHBoxLayout;
	energyLayout->addLayout(energySetpointLayout);
	energyLayout->addWidget(elementChoice_);
	energyLayout->addWidget(lineChoice_);

	// Setting the scan position.
	QCheckBox *goToPosition = new QCheckBox("Choose Position");
	goToPosition->setChecked(config_->goToPosition());

	QPushButton *setCurrentPosition = new QPushButton(QIcon(":/save.png"), "");
	setCurrentPosition->setEnabled(goToPosition->isChecked());
	connect(setCurrentPosition, SIGNAL(clicked()), this, SLOT(setScanPosition()));

	savedXPosition_ = new QLabel(QString::number(config_->goToPosition() ? config_->x() : 0, 'g', 3) + " mm");
	savedXPosition_->setEnabled(goToPosition->isChecked());
	savedYPosition_ = new QLabel(QString::number(config_->goToPosition() ? config_->y() : 0, 'g', 3) + " mm");
	savedYPosition_->setEnabled(goToPosition->isChecked());

	positionsSaved_ = new QLabel;
	QPalette palette(this->palette());
	palette.setColor(QPalette::Disabled, QPalette::WindowText, Qt::darkGray);

	// Although not entirely valid, the chances that both x and y being identically 0 for a saved position is incredibly unlikely.
	if (!config_->goToPosition() || (config_->x() == 0.0 && config_->y() == 0.0)){

		positionsSaved_->setText("Unsaved");
		palette.setColor(QPalette::Active, QPalette::WindowText, Qt::red);
	}
	else{

		positionsSaved_->setText("Saved");
		palette.setColor(QPalette::Active, QPalette::WindowText, Qt::darkGreen);
	}

	positionsSaved_->setPalette(palette);

	QFont font(this->font());
	font.setBold(true);
	positionsSaved_->setFont(font);
	positionsSaved_->setEnabled(goToPosition->isChecked());

	QHBoxLayout *saveLayout = new QHBoxLayout;
	saveLayout->addWidget(setCurrentPosition);
	saveLayout->addWidget(positionsSaved_);

	xPosition_ = new QDoubleSpinBox;
	xPosition_->setEnabled(goToPosition->isChecked());
	xPosition_->setDecimals(3);
	xPosition_->setRange(-100, 100);
	xPosition_->setValue(config_->goToPosition() ? config_->x() : 0);
	xPosition_->setSuffix(" mm");
	connect(VESPERSBeamline::vespers()->pseudoSampleStage(), SIGNAL(horizontalSetpointChanged(double)), xPosition_, SLOT(setValue(double)));
	connect(xPosition_, SIGNAL(valueChanged(double)), this, SLOT(onXorYPositionChanged()));

	QHBoxLayout *xLayout = new QHBoxLayout;
	xLayout->addWidget(xPosition_);
	xLayout->addWidget(savedXPosition_);

	yPosition_ = new QDoubleSpinBox;
	yPosition_->setEnabled(goToPosition->isChecked());
	yPosition_->setDecimals(3);
	yPosition_->setRange(-100, 100);
	yPosition_->setValue(config_->goToPosition() ? config_->y() : 0);
	yPosition_->setSuffix(" mm");
	connect(VESPERSBeamline::vespers()->pseudoSampleStage(), SIGNAL(verticalSetpointChanged(double)), yPosition_, SLOT(setValue(double)));
	connect(yPosition_, SIGNAL(valueChanged(double)), this, SLOT(onXorYPositionChanged()));

	QHBoxLayout *yLayout = new QHBoxLayout;
	yLayout->addWidget(yPosition_);
	yLayout->addWidget(savedYPosition_);

	connect(goToPosition, SIGNAL(toggled(bool)), config_, SLOT(setGoToPosition(bool)));
	connect(goToPosition, SIGNAL(toggled(bool)), setCurrentPosition, SLOT(setEnabled(bool)));
	connect(goToPosition, SIGNAL(toggled(bool)), xPosition_, SLOT(setEnabled(bool)));
	connect(goToPosition, SIGNAL(toggled(bool)), yPosition_, SLOT(setEnabled(bool)));
	connect(goToPosition, SIGNAL(toggled(bool)), savedXPosition_, SLOT(setEnabled(bool)));
	connect(goToPosition, SIGNAL(toggled(bool)), savedYPosition_, SLOT(setEnabled(bool)));
	connect(goToPosition, SIGNAL(toggled(bool)), positionsSaved_, SLOT(setEnabled(bool)));

	QFormLayout *positionLayout = new QFormLayout;
	positionLayout->addRow(goToPosition);
	positionLayout->addRow(saveLayout);
	positionLayout->addRow("x:", xLayout);
	positionLayout->addRow("y:", yLayout);

	estimatedTime_ = new QLabel;
	estimatedSetTime_ = new QLabel;
	connect(config_, SIGNAL(totalTimeChanged(double)), this, SLOT(onEstimatedTimeChanged()));
	onEstimatedTimeChanged();

	// The roi text edit.
	roiText_ = new QTextEdit;
	roiText_->setReadOnly(true);

	if (config_->fluorescenceDetectorChoice() == VESPERSEXAFSScanConfiguration::None)
		roiText_->hide();

	else{

		roiText_->insertPlainText("Name\tLow (eV)\tHigh (eV)\n");

		for (int i = 0; i < config_->roiList().count(); i++)
			roiText_->insertPlainText(GeneralUtilities::addGreek(config_->roiList().at(i).name())+"\t" + QString::number(config_->roiList().at(i).low()) + "\t" + QString::number(config_->roiList().at(i).high()) +"\n");
	}

	// Label showing where the data will be saved.
	QLabel *exportPath = new QLabel(QString("Data exported to: %1exportData").arg(AMUserSettings::userDataFolder));

	// Setting up the layout.
	QGridLayout *contentsLayout = new QGridLayout;
	contentsLayout->addWidget(regionsView_, 1, 0, 2, 2);
	contentsLayout->addWidget(fluorescenceDetectorGroupBox, 1, 2);
	contentsLayout->addLayout(scanNameLayout, 4, 0);
	contentsLayout->addLayout(energyLayout, 0, 0, 1, 3);
	contentsLayout->addLayout(positionLayout, 4, 2, 4, 1);
	contentsLayout->addWidget(ionChambersGroupBox, 2, 2, 2, 1);
	contentsLayout->addWidget(roiText_, 1, 3, 2, 2);
	contentsLayout->addWidget(useFixedTime, 3, 0);
	contentsLayout->addWidget(estimatedTime_, 6, 0, 1, 2);
	contentsLayout->addWidget(estimatedSetTime_, 7, 0, 1, 2);
	contentsLayout->addLayout(numberOfScansLayout, 5, 0);
	contentsLayout->addWidget(exportPath, 8, 0, 1, 3);

	QHBoxLayout *squeezeContents = new QHBoxLayout;
	squeezeContents->addStretch();
	squeezeContents->addLayout(contentsLayout);
	squeezeContents->addStretch();

	QVBoxLayout *configViewLayout = new QVBoxLayout;
	configViewLayout->addWidget(frame);
	configViewLayout->addStretch();
	configViewLayout->addWidget(regionsLineView_, 0, Qt::AlignCenter);
	configViewLayout->addLayout(squeezeContents);
	configViewLayout->addStretch();

	setLayout(configViewLayout);
}

void VESPERSEXAFSScanConfigurationView::onFluorescenceChoiceChanged(int id)
{
	config_->setFluorescenceDetectorChoice(id);
	roiText_->clear();

	switch(id){

	case 0:
		config_->setRoiInfoList(AMROIInfoList());
		roiText_->hide();
		break;

	case 1:
		config_->setRoiInfoList(*VESPERSBeamline::vespers()->vortexXRF1E()->roiInfoList());
		roiText_->show();
		break;

	case 2:
		config_->setRoiInfoList(*VESPERSBeamline::vespers()->vortexXRF4E()->roiInfoList());
		roiText_->show();
		break;
	}

	roiText_->insertPlainText("Name\tLow (eV)\tHigh (eV)\n");

	for (int i = 0; i < config_->roiList().count(); i++)
		roiText_->insertPlainText(GeneralUtilities::addGreek(config_->roiList().at(i).name())+"\t" + QString::number(config_->roiList().at(i).low()) + "\t" + QString::number(config_->roiList().at(i).high()) +"\n");
}

void VESPERSEXAFSScanConfigurationView::onElementChoiceClicked()
{
	AMElement *el = AMPeriodicTableDialog::getElement(this);

	if (el){

		elementChoice_->setText(el->symbol());
		fillLinesComboBox(el);
		onLinesComboBoxIndexChanged(0);
	}
}

void VESPERSEXAFSScanConfigurationView::fillLinesComboBox(AMElement *el)
{
	QPair<QString, QString> edge;
	lineChoice_->clear();

	for (int i = 0; i < el->edges().size(); i++){

		edge = el->edges().at(i);

		if (edge.second.toDouble() <= 30000 && edge.second.toDouble() >= 6700)
			lineChoice_->addItem(edge.first+": "+edge.second+" eV", edge.second.toDouble());
	}
}

void VESPERSEXAFSScanConfigurationView::onLinesComboBoxIndexChanged(int index)
{
	if (lineChoice_->count() == 0 || index == -1)
		return;

	energy_->setValue(lineChoice_->itemData(index).toDouble());
	setEnergy();
	config_->setEdge(elementChoice_->text()+" "+lineChoice_->itemText(index).split(":").first());
}

void VESPERSEXAFSScanConfigurationView::onItI0Toggled(int id)
{
	if (id == 0){

		ItGroup_->button(0)->hide();
		ItGroup_->button(1)->hide();
		ItGroup_->button(2)->hide();
		ItGroup_->button(3)->hide();

		I0Group_->button(0)->show();
		I0Group_->button(1)->show();
		I0Group_->button(2)->show();
		I0Group_->button(3)->show();
	}
	else{

		I0Group_->button(0)->hide();
		I0Group_->button(1)->hide();
		I0Group_->button(2)->hide();
		I0Group_->button(3)->hide();

		ItGroup_->button(0)->show();
		ItGroup_->button(1)->show();
		ItGroup_->button(2)->show();
		ItGroup_->button(3)->show();
	}
}

void VESPERSEXAFSScanConfigurationView::onItClicked(int id)
{
	// If the new It is at or upstream of I0, move I0.  Using id-1 is safe because Isplit can't be chosen for It.
	if (id <= I0Group_->checkedId())
		I0Group_->button(id-1)->click();

	for (int i = 0; i < id; i++)
		I0Group_->button(i)->setEnabled(true);

	for (int i = id; i < 4; i++)
		I0Group_->button(i)->setEnabled(false);

	config_->setTransmissionChoice(id);
}

void VESPERSEXAFSScanConfigurationView::onEstimatedTimeChanged()
{
	estimatedTime_->setText("Estimated time per scan:\t" + convertTimeToString(config_->totalTime()));
	estimatedSetTime_->setText("Estimated time for set:\t" + convertTimeToString(config_->totalTime()*config_->numberOfScans()));
}

QString VESPERSEXAFSScanConfigurationView::convertTimeToString(double time)
{
	QString timeString;

	int days = time/3600/24;

	if (days > 0){

		time -= time/3600/24;
		timeString += QString::number(days) + "d:";
	}

	int hours = time/3600;

	if (hours > 0){

		time -= hours*3600;
		timeString += QString::number(hours) + "h:";
	}

	int minutes = time/60;

	if (minutes > 0){

		time -= minutes*60;
		timeString += QString::number(minutes) + "m:";
	}

	int seconds = ((int)time)%60;
	timeString += QString::number(seconds) + "s";

	return timeString;
}

/*
Copyright 2010-2012 Mark Boots, David Chevrier, and Darren Hunter.

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


#include "VESPERSRoperCCDDetectorView.h"

#include "ui/AMTopFrame.h"

#include <QToolButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

VESPERSRoperCCDDetectorView::VESPERSRoperCCDDetectorView(VESPERSRoperCCDDetector *detector, bool configureOnly, QWidget *parent)
	: AMDetailedDetectorView(configureOnly, parent)
{
	detector_ = 0;
	setDetector(detector, configureOnly);
}

bool VESPERSRoperCCDDetectorView::setDetector(AMDetector *detector, bool configureOnly)
{
	//I don't have a configure only view for these.  It doesn't make quite as much sense for the stand alone spectra to have configure only views.
	Q_UNUSED(configureOnly)

	// If there is no valid detector pointer, then return false.
	if (!detector)
		return false;

	AMTopFrame *topFrame = new AMTopFrame(QString("X-Ray Diffraction - Roper CCD"));
	topFrame->setIcon(QIcon(":/utilities-system-monitor.png"));

	detector_ = static_cast<VESPERSRoperCCDDetector *>(detector);
	connect(detector_, SIGNAL(connected(bool)), this, SLOT(setEnabled(bool)));

	QPushButton *loadCCDButton = new QPushButton("Load test");
	connect(loadCCDButton, SIGNAL(clicked()), this, SLOT(loadCCDFileTest()));

	image_ = new QLabel;
	QPixmap pixmap = QPixmap(600, 600);
	pixmap.fill(Qt::blue);
	image_->setPixmap(pixmap);

	isAcquiring_ = new QLabel;
	isAcquiring_->setPixmap(QIcon(":/OFF.png").pixmap(25));
	connect(detector_, SIGNAL(isAcquiringChanged(bool)), this, SLOT(onIsAcquiringChanged(bool)));

	state_ = new QLabel;
	connect(detector_, SIGNAL(stateChanged(VESPERSRoperCCDDetector::State)), this, SLOT(onStateChanged(VESPERSRoperCCDDetector::State)));

	acquireTime_ = new QDoubleSpinBox;
	acquireTime_->setSuffix(" s");
	acquireTime_->setDecimals(2);
	acquireTime_->setRange(0, 10000);
	acquireTime_->setAlignment(Qt::AlignCenter);
	connect(detector_, SIGNAL(acquireTimeChanged(double)), acquireTime_, SLOT(setValue(double)));
	connect(acquireTime_, SIGNAL(editingFinished()), this, SLOT(setAcquireTime()));

	QToolButton *startButton = new QToolButton;
	startButton->setIcon(QIcon(":/play_button_green.png"));
	connect(startButton, SIGNAL(clicked()), detector_, SLOT(start()));

	QToolButton *stopButton = new QToolButton;
	stopButton->setIcon(QIcon(":/red-stop-button.png"));
	connect(stopButton, SIGNAL(clicked()), detector_, SLOT(stop()));

	triggerMode_ = new QComboBox;
	triggerMode_->addItem("Free Run");
	triggerMode_->addItem("External Sync");
	connect(detector_, SIGNAL(triggerModeChanged(VESPERSRoperCCDDetector::TriggerMode)), this, SLOT(onTriggerModeChanged(VESPERSRoperCCDDetector::TriggerMode)));
	connect(triggerMode_, SIGNAL(currentIndexChanged(int)), this, SLOT(setTriggerMode(int)));

	imageMode_ = new QComboBox;
	imageMode_->addItem("Normal");
	imageMode_->addItem("Focus");
	connect(detector_, SIGNAL(imageModeChanged(VESPERSRoperCCDDetector::ImageMode)), this, SLOT(onImageModeChanged(VESPERSRoperCCDDetector::ImageMode)));
	connect(imageMode_, SIGNAL(currentIndexChanged(int)), this, SLOT(setImageMode(int)));

	QToolButton *saveButton = new QToolButton;
	saveButton->setIcon(QIcon(":/save.png"));
	connect(saveButton, SIGNAL(clicked()), detector_, SLOT(saveFile()));

	autoSaveComboBox_ = new QComboBox;
	autoSaveComboBox_->addItem("No");
	autoSaveComboBox_->addItem("Yes");
	connect(detector_, SIGNAL(autoSaveEnabledChanged(bool)), this, SLOT(onAutoSaveChanged(bool)));
	connect(autoSaveComboBox_, SIGNAL(currentIndexChanged(int)), this, SLOT(setAutoSave(int)));

	// Setup the CCD file path signals and layout.
	filePathEdit_ = new QLineEdit;
	connect(filePathEdit_, SIGNAL(editingFinished()), this, SLOT(ccdPathEdited()));
	connect(detector_, SIGNAL(ccdPathChanged(QString)), filePathEdit_, SLOT(setText(QString)));

	fileNameEdit_ = new QLineEdit;
	connect(fileNameEdit_, SIGNAL(editingFinished()), this, SLOT(ccdFileEdited()));
	connect(detector_, SIGNAL(ccdNameChanged(QString)), fileNameEdit_, SLOT(setText(QString)));

	fileNumberEdit_ = new QLineEdit;
	connect(fileNumberEdit_, SIGNAL(editingFinished()), this, SLOT(ccdNumberEdited()));
	connect(detector_, SIGNAL(ccdNumberChanged(int)), this, SLOT(ccdNumberUpdate(int)));

	QGroupBox *acquisitionBox = new QGroupBox("Acquisition");

	QHBoxLayout *statusLayout = new QHBoxLayout;
	statusLayout->addWidget(isAcquiring_);
	statusLayout->addWidget(state_);
	statusLayout->addWidget(startButton);
	statusLayout->addWidget(stopButton);

	QHBoxLayout *modeLayout = new QHBoxLayout;
	modeLayout->addWidget(acquireTime_);
	modeLayout->addWidget(triggerMode_);
	modeLayout->addWidget(imageMode_);

	QVBoxLayout *acquisitionLayout = new QVBoxLayout;
	acquisitionLayout->addLayout(statusLayout);
	acquisitionLayout->addLayout(modeLayout);

	acquisitionBox->setLayout(acquisitionLayout);

	QHBoxLayout *ccdGBTopLayout = new QHBoxLayout;
	ccdGBTopLayout->addWidget(new QLabel("Autosave Image:"));
	ccdGBTopLayout->addWidget(autoSaveComboBox_);
	ccdGBTopLayout->addWidget(new QLabel("Save Current Image:"));
	ccdGBTopLayout->addWidget(saveButton);

	QGroupBox *ccdGB = new QGroupBox(tr("Image File Options"));
	QFormLayout *ccdGBLayout = new QFormLayout;
	ccdGBLayout->addRow(ccdGBTopLayout);
	ccdGBLayout->addRow("Path:", filePathEdit_);
	ccdGBLayout->addRow("Name:", fileNameEdit_);
	ccdGBLayout->addRow("Number:", fileNumberEdit_);
	ccdGBLayout->setLabelAlignment(Qt::AlignRight);
	ccdGB->setLayout(ccdGBLayout);

	QVBoxLayout *acquisitionAndTemperatureLayout = new QVBoxLayout;
	acquisitionAndTemperatureLayout->addWidget(acquisitionBox);
	acquisitionAndTemperatureLayout->addStretch();

	QHBoxLayout *detectorLayout = new QHBoxLayout;
	detectorLayout->addStretch();
	detectorLayout->addLayout(acquisitionAndTemperatureLayout);
	detectorLayout->addWidget(ccdGB);
	detectorLayout->addStretch();

	QHBoxLayout *horizontalSquishLayout = new QHBoxLayout;
	horizontalSquishLayout->addStretch();
	horizontalSquishLayout->addLayout(detectorLayout);
	horizontalSquishLayout->addStretch();

	QVBoxLayout *masterLayout = new QVBoxLayout;
	masterLayout->addWidget(topFrame);
	masterLayout->addStretch();
	masterLayout->addWidget(image_, 0, Qt::AlignCenter);
	masterLayout->addWidget(loadCCDButton);
	masterLayout->addLayout(horizontalSquishLayout);
	masterLayout->addStretch();

	setLayout(masterLayout);

	return true;
}

void VESPERSRoperCCDDetectorView::onTriggerModeChanged(VESPERSRoperCCDDetector::TriggerMode mode)
{
	triggerMode_->setCurrentIndex((int)mode);
}

void VESPERSRoperCCDDetectorView::setTriggerMode(int newMode)
{
	if (newMode == (int)detector_->triggerMode())
		return;

	switch(newMode){

	case 0:
		detector_->setTriggerMode(VESPERSRoperCCDDetector::FreeRun);
		break;

	case 1:
		detector_->setTriggerMode(VESPERSRoperCCDDetector::ExtSync);
		break;
	}
}

void VESPERSRoperCCDDetectorView::onImageModeChanged(VESPERSRoperCCDDetector::ImageMode mode)
{
	if (mode == VESPERSRoperCCDDetector::Focus)
		imageMode_->setCurrentIndex(1);
	else
		imageMode_->setCurrentIndex(0);
}

void VESPERSRoperCCDDetectorView::setImageMode(int newMode)
{
	switch(newMode){

	case 0:
		if (detector_->imageMode() != VESPERSRoperCCDDetector::Normal)
			detector_->setImageMode(VESPERSRoperCCDDetector::Normal);

		break;

	case 1:
		if (detector_->imageMode() != VESPERSRoperCCDDetector::Focus )
			detector_->setImageMode(VESPERSRoperCCDDetector::Focus);

		break;
	}
}

void VESPERSRoperCCDDetectorView::onStateChanged(VESPERSRoperCCDDetector::State newState)
{
	switch(newState){

	case VESPERSRoperCCDDetector::Idle:
		state_->setText("Idle");
		break;

	case VESPERSRoperCCDDetector::Acquire:
		state_->setText("Acquire");
		break;

	case VESPERSRoperCCDDetector::Readout:
		state_->setText("Readout");
		break;

	case VESPERSRoperCCDDetector::Correct:
		state_->setText("Correct");
		break;

	case VESPERSRoperCCDDetector::Saving:
		state_->setText("Saving");
		break;

	case VESPERSRoperCCDDetector::Aborting:
		state_->setText("Aborting");
		break;

	case VESPERSRoperCCDDetector::Error:
		state_->setText("Error");
		break;

	case VESPERSRoperCCDDetector::Waiting:
		state_->setText("Waiting");
		break;
	}
}

void VESPERSRoperCCDDetectorView::onAutoSaveChanged(bool autoSave)
{
	if (autoSave)
		autoSaveComboBox_->setCurrentIndex(1);
	else
		autoSaveComboBox_->setCurrentIndex(0);
}

void VESPERSRoperCCDDetectorView::setAutoSave(int autoSave)
{
	switch(autoSave){

	case 0:
		if (detector_->autoSaveEnabled())
			detector_->setAutoSaveEnabled(false);

		break;

	case 1:
		if (!detector_->autoSaveEnabled())
			detector_->setAutoSaveEnabled(true);

		break;
	}
}

#include "MPlot/MPlotColorMap.h"
#include <QTime>
#include <QRgb>

void VESPERSRoperCCDDetectorView::loadCCDFileTest()
{
	QTime time1;
	time1.start();
	QTime time;
	time.start();

	detector_->loadImageFromFile();

	qDebug() << "Time for loading the image from the file" << time.restart() << "ms";
	QVector<int> data = detector_->imageData();
	QImage image = QImage(2084, 2084, QImage::Format_ARGB32);

	int maximum = data.at(0);
	int minimum = data.at(0);

	for (int i = 0, size = data.size(); i < size; i++){

		if (data.at(i) < minimum)
			minimum = data.at(i);

		if (data.at(i) > maximum)
			maximum = data.at(i);
	}
	qDebug() << "Finding min and max:" << time.restart() << "ms";
	MPlotColorMap map = MPlotColorMap(MPlotColorMap::Jet);
	MPlotInterval range = MPlotInterval(minimum, int(maximum*0.10));

	int xSize = detector_->size().i();
	int ySize = detector_->size().j();

//		uchar *rgbData = new uchar[4*xSize*ySize];

	for (int i = 0; i < xSize; i++){

		int offset = i*ySize;

		for (int j = 0; j < ySize; j++){
			image.setPixel(i, j, map.rgbAt(data.at(j + offset), range));
//				QRgb rgb = map.rgbAt(data.at(j + offset), range);
//				rgbData[4*(offset+j)] = qRed(rgb);
//				rgbData[4*(offset+j)+1] = qGreen(rgb);
//				rgbData[4*(offset+j)+2] = qBlue(rgb);
		}
	}
//		QImage image = QImage(rgbData, 2084, 2084, QImage::Format_RGB32);
	qDebug() << "Time to set data in the pixels" << time.restart() << "ms";

	QPixmap newImage;
	newImage.convertFromImage(image);
	qDebug() << "Converting QImage to QPixmap" << time.restart() << "ms";
	newImage = newImage.scaled(600, 600, Qt::KeepAspectRatio);
	qDebug() << "Scaling QPixmap" << time.restart() << "ms";
	image_->setPixmap(newImage);
//		delete rgbData;
	qDebug() << "Time to put image in pixmap" << time.restart() << "ms";

	qDebug() << "Time to fully load and display image is" << time1.elapsed() << "ms";
}

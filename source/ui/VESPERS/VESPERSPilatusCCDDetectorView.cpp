#include "VESPERSPilatusCCDDetectorView.h"

VESPERSPilatusCCDDetectorView::VESPERSPilatusCCDDetectorView(VESPERSPilatusCCDDetector *detector, QWidget *parent)
	: VESPERSCCDDetectorView(detector, false, parent)
{
	QGroupBox *diskUsageBox = new QGroupBox("Aurora File System");
	diskUsageBar_ = new QProgressBar;
	diskUsageBar_->setRange(0, 1);
	diskUsageBar_->setValue(0);
	status_ = new QLabel;
	status_->setPixmap(QIcon(":/Yellow.png").pixmap(20));

	QHBoxLayout *barAndLabelLayout = new QHBoxLayout;
	barAndLabelLayout->addWidget(new QLabel("Disk Usage:"));
	barAndLabelLayout->addWidget(diskUsageBar_);
	barAndLabelLayout->addWidget(status_);
	diskUsageBox->setLayout(barAndLabelLayout);

	connect(detector, SIGNAL(fileSystemInfoUpdated()), this, SLOT(onFileSystemInfoUpdate()));

	detectorLayout_->insertWidget(1, diskUsageBox);
}

void VESPERSPilatusCCDDetectorView::onFileSystemInfoUpdate()
{
	VESPERSPilatusCCDDetector *detector = (VESPERSPilatusCCDDetector *)detector_;
	diskUsageBar_->setRange(0, detector->totalAuroraSize());
	diskUsageBar_->setValue(detector->occupiedAuroraSize());

	double diskUsage = detector->occupiedAuroraSize()/detector->totalAuroraSize();

	if (diskUsage < 0.6)
		status_->setPixmap(QIcon(":/ON.png").pixmap(20));

	else if (diskUsage >= 0.6 && diskUsage < 0.8)
		status_->setPixmap(QIcon(":/Yellow.png").pixmap(20));

	else
		status_->setPixmap(QIcon(":/RED.png").pixmap(20));
}

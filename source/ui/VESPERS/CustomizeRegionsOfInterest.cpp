#include "CustomizeRegionsOfInterest.h"
#include "util/AMElement.h"
#include "util/AMPeriodicTable.h"

#include <QHBoxLayout>
#include <cmath>

RegionOfInterestView::RegionOfInterestView(AMROI *roi, QWidget *parent)
	: QWidget(parent)
{
	roi_ = roi;

	connect(roi_, SIGNAL(roiHasValues(bool)), this, SLOT(onRoiInialized(bool)));

	name_ = new QLabel;
	connect(roi, SIGNAL(nameUpdate(QString)), this, SLOT(nameUpdate(QString)));

	low_ = new QDoubleSpinBox;
	low_->setMinimum(0);
	low_->setMaximum(30000);
	low_->setDecimals(0);
	low_->setSingleStep(roi->scale());
	low_->setSuffix(" eV");
	connect(low_, SIGNAL(editingFinished()), this, SLOT(setRoiLow()));
	connect(roi, SIGNAL(lowUpdate(int)), this, SLOT(onLowUpdate(int)));

	high_ = new QDoubleSpinBox;
	high_->setMinimum(0);
	high_->setMaximum(30000);
	high_->setDecimals(0);
	high_->setSingleStep(roi->scale());
	high_->setSuffix(" eV");
	connect(high_, SIGNAL(editingFinished()), this, SLOT(setRoiHigh()));
	connect(roi, SIGNAL(highUpdate(int)), this, SLOT(onHighUpdate(int)));

	QLabel *value = new QLabel;
	connect(roi, SIGNAL(valueUpdate(double)), value, SLOT(setNum(double)));

	QHBoxLayout *roiLayout = new QHBoxLayout;
	roiLayout->addWidget(name_, 0, Qt::AlignCenter);
	roiLayout->addWidget(new QLabel("Low: "), 0, Qt::AlignRight);
	roiLayout->addWidget(low_);
	roiLayout->addWidget(new QLabel("High: "), 0, Qt::AlignRight);
	roiLayout->addWidget(high_);
	roiLayout->addWidget(value, 0, Qt::AlignCenter);

	setLayout(roiLayout);
	setMinimumWidth(420);
}

void RegionOfInterestView::nameUpdate(QString name)
{
	if (name.isEmpty()){

		hide();
		return;
	}

	name = name.left(name.indexOf(" "));
	AMElement *el = AMPeriodicTable::table()->elementBySymbol(name);

	if (el){

		int low = roi_->low();
		int high = roi_->high();

		for (int j = 0; j < el->emissionLines().count(); j++){

			if (el->emissionLines().at(j).first.contains("1")
					&& fabs((low+high)/2 - el->emissionLines().at(j).second.toDouble()/roi_->scale()) < 3)
				name_->setText(el->symbol()+" "+el->emissionLines().at(j).first);
		}
	}

	show();
}

CustomizeRegionsOfInterest::CustomizeRegionsOfInterest(QList<AMROI *> rois, QWidget *parent)
	: QWidget(parent)
{
	QVBoxLayout *listLayout = new QVBoxLayout;

	for (int i = 0; i < rois.size(); i++)
		listLayout->addWidget(new RegionOfInterestView(rois.at(i)));

	listLayout->addStretch();

	setLayout(listLayout);
	setMinimumSize(420, 500);
}
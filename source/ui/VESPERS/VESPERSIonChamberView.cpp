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


#include "VESPERSIonChamberView.h"

#include <QHBoxLayout>
#include <QStringList>
#include <QPushButton>

VESPERSIonChamberView::VESPERSIonChamberView(VESPERSIonChamber *ionChamber, QWidget *parent)
	: QWidget(parent)
{
	ionChamber_ = ionChamber;

	QLabel *name = new QLabel(ionChamber_->name());

	value_ = new QComboBox;
	value_->addItems(QStringList() << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200" << "500");
	connect(value_, SIGNAL(currentIndexChanged(int)), this, SLOT(onValueComboBoxChanged(int)));
	connect(ionChamber_, SIGNAL(sensitivityValueChanged(int)), this, SLOT(onValueChanged(int)));

	units_ = new QComboBox;
	units_->addItems(QStringList() << "pA/V" << "nA/V" << "uA/V" << "mA/V");
	connect(units_, SIGNAL(currentIndexChanged(int)), this, SLOT(onUnitsComboBoxChanged(int)));
	connect(ionChamber_, SIGNAL(sensitivityUnitsChanged(QString)), this, SLOT(onUnitsChanged(QString)));

	QLabel *voltage = new QLabel(QString::number(ionChamber_->voltage()));
	connect(ionChamber_, SIGNAL(voltageChanged(double)), voltage, SLOT(setNum(double)));

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(name, 0, Qt::AlignLeft);
	layout->addWidget(value_, 0, Qt::AlignCenter);
	layout->addWidget(units_, 0, Qt::AlignCenter);
	layout->addWidget(voltage, 0, Qt::AlignCenter);

	setLayout(layout);
}

void VESPERSIonChamberView::onValueChanged(int value)
{
	ionChamber_->blockSignals(true);

	switch(value){

	case 1:
		value_->setCurrentIndex(0);
		break;
	case 2:
		value_->setCurrentIndex(1);
		break;
	case 5:
		value_->setCurrentIndex(2);
		break;
	case 10:
		value_->setCurrentIndex(3);
		break;
	case 20:
		value_->setCurrentIndex(4);
		break;
	case 50:
		value_->setCurrentIndex(5);
		break;
	case 100:
		value_->setCurrentIndex(6);
		break;
	case 200:
		value_->setCurrentIndex(7);
		break;
	case 500:
		value_->setCurrentIndex(8);
		break;
	}

	ionChamber_->blockSignals(false);
}

void VESPERSIonChamberView::onUnitsChanged(QString units)
{
	ionChamber_->blockSignals(true);

	if (units == "pA/V")
		units_->setCurrentIndex(0);
	else if (units == "nA/V")
		units_->setCurrentIndex(1);
	else if (units == "uA/V")
		units_->setCurrentIndex(2);
	else if (units == "mA/V")
		units_->setCurrentIndex(3);

	ionChamber_->blockSignals(false);
}

VESPERSIonChamberBasicView::VESPERSIonChamberBasicView(VESPERSIonChamber *ionChamber, double min, double max, QWidget *parent)
	: QWidget(parent)
{
	ionChamber_ = ionChamber;
	minimumVoltage_ = min;
	maximumVoltage_ = max;

	QLabel *name = new QLabel(ionChamber_->name());

	QPushButton *minus = new QPushButton(QIcon(":/22x22/list-remove.png"), "");
	minus->setMaximumSize(40, 40);
	connect(minus, SIGNAL(clicked()), ionChamber_, SLOT(increaseSensitivity()));

	QPushButton *plus = new QPushButton(QIcon(":/22x22/list-add.png"), "");
	plus->setMaximumSize(40, 40);
	connect(plus, SIGNAL(clicked()), ionChamber_, SLOT(decreaseSensitivity()));

	feedback_ = new QLabel;
	connect(ionChamber_, SIGNAL(voltageChanged(double)), this, SLOT(onIonChamberChanged()));

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(name);
	layout->addWidget(minus);
	layout->addWidget(plus);
	layout->addWidget(feedback_);

	setLayout(layout);
}

void VESPERSIonChamberBasicView::onIonChamberChanged()
{
	double val = ionChamber_->voltage();

	if (val > minimumVoltage_ && val < maximumVoltage_)
		feedback_->setText("Good");
	else if (val <= minimumVoltage_)
		feedback_->setText("Too Low");
	else if (val >= maximumVoltage_)
		feedback_->setText("Too High");
	else
		feedback_->setText("--");
}

VESPERSSplitIonChamberView::VESPERSSplitIonChamberView(VESPERSSplitIonChamber *ionChamber, QWidget *parent)
	: QWidget(parent)
{
	ionChamber_ = ionChamber;

	QLabel *name = new QLabel(ionChamber_->name());

	value_ = new QComboBox;
	value_->addItems(QStringList() << "1" << "2" << "5" << "10" << "20" << "50" << "100" << "200" << "500");
	connect(value_, SIGNAL(currentIndexChanged(int)), this, SLOT(onValueComboBoxChanged(int)));
	connect(ionChamber_, SIGNAL(sensitivityValueChanged(int)), this, SLOT(onValueChanged(int)));

	units_ = new QComboBox;
	units_->addItems(QStringList() << "pA/V" << "nA/V" << "uA/V" << "mA/V");
	connect(units_, SIGNAL(currentIndexChanged(int)), this, SLOT(onUnitsComboBoxChanged(int)));
	connect(ionChamber_, SIGNAL(sensitivityUnitsChanged(QString)), this, SLOT(onUnitsChanged(QString)));

	QLabel *voltageA = new QLabel(QString::number(ionChamber_->voltageA()));
	connect(ionChamber_, SIGNAL(voltageAChanged(double)), voltageA, SLOT(setNum(double)));
	QLabel *voltageB = new QLabel(QString::number(ionChamber_->voltageB()));
	connect(ionChamber_, SIGNAL(voltageBChanged(double)), voltageB, SLOT(setNum(double)));

	QVBoxLayout *voltageLayout = new QVBoxLayout;
	voltageLayout->addWidget(voltageA, 0, Qt::AlignCenter);
	voltageLayout->addWidget(voltageB, 0, Qt::AlignCenter);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(name, 0, Qt::AlignLeft);
	layout->addWidget(value_, 0, Qt::AlignCenter);
	layout->addWidget(units_, 0, Qt::AlignCenter);
	layout->addLayout(voltageLayout);

	setLayout(layout);
}

void VESPERSSplitIonChamberView::onValueChanged(int value)
{
	ionChamber_->blockSignals(true);

	switch(value){

	case 1:
		value_->setCurrentIndex(0);
		break;
	case 2:
		value_->setCurrentIndex(1);
		break;
	case 5:
		value_->setCurrentIndex(2);
		break;
	case 10:
		value_->setCurrentIndex(3);
		break;
	case 20:
		value_->setCurrentIndex(4);
		break;
	case 50:
		value_->setCurrentIndex(5);
		break;
	case 100:
		value_->setCurrentIndex(6);
		break;
	case 200:
		value_->setCurrentIndex(7);
		break;
	case 500:
		value_->setCurrentIndex(8);
		break;
	}

	ionChamber_->blockSignals(false);
}

void VESPERSSplitIonChamberView::onUnitsChanged(QString units)
{
	ionChamber_->blockSignals(true);

	if (units == "pA/V")
		units_->setCurrentIndex(0);
	else if (units == "nA/V")
		units_->setCurrentIndex(1);
	else if (units == "uA/V")
		units_->setCurrentIndex(2);
	else if (units == "mA/V")
		units_->setCurrentIndex(3);

	ionChamber_->blockSignals(false);
}

VESPERSSplitIonChamberBasicView::VESPERSSplitIonChamberBasicView(VESPERSSplitIonChamber *ionChamber, double min, double max, QWidget *parent)
	: QWidget(parent)
{
	ionChamber_ = ionChamber;
	minimumVoltage_ = min;
	maximumVoltage_ = max;

	QLabel *name = new QLabel(ionChamber_->name());

	QPushButton *minus = new QPushButton(QIcon(":/22x22/list-remove.png"), "");
	minus->setMaximumSize(40, 40);
	connect(minus, SIGNAL(clicked()), ionChamber_, SLOT(increaseSensitivity()));

	QPushButton *plus = new QPushButton(QIcon(":/22x22/list-add.png"), "");
	plus->setMaximumSize(40, 40);
	connect(plus, SIGNAL(clicked()), ionChamber_, SLOT(decreaseSensitivity()));

	feedback_ = new QLabel;
	connect(ionChamber_, SIGNAL(voltageAChanged(double)), this, SLOT(onIonChamberChanged()));
	connect(ionChamber_, SIGNAL(voltageBChanged(double)), this, SLOT(onIonChamberChanged()));

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(name);
	layout->addWidget(minus);
	layout->addWidget(plus);
	layout->addWidget(feedback_);

	setLayout(layout);
}

void VESPERSSplitIonChamberBasicView::onIonChamberChanged()
{
	double valA = ionChamber_->voltageA();
	double valB = ionChamber_->voltageB();

	if ((valA > minimumVoltage_ && valA < maximumVoltage_) && (valB > minimumVoltage_ && valB < maximumVoltage_))
		feedback_->setText("Good");
	else if (valA <= minimumVoltage_ || valB <= minimumVoltage_)
		feedback_->setText("Too Low");
	else if (valA >= maximumVoltage_ || valB >= maximumVoltage_)
		feedback_->setText("Too High");
	else
		feedback_->setText("--");
}

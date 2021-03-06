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


#include "VESPERSDeadTimeButton.h"
#include <QPainter>
#include <QStyleOption>

VESPERSDeadTimeButton::VESPERSDeadTimeButton(double good, double bad, QWidget *parent)
	: QToolButton(parent)
{
	good_ = good;
	bad_ = bad;
	current_ = 0;
}

void VESPERSDeadTimeButton::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e)

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// Main button box.  Uses the standand push button style for the basic aspects of the box.
	QStyleOptionButton option;
	option.initFrom(this);
	option.state = QStyle::State_Off;

	if (!isEnabled())
		option.palette = QPalette(Qt::black, QColor(170, 170, 170, 100), Qt::gray, Qt::gray, QColor(170, 170, 170), Qt::gray, Qt::gray, Qt::gray, QColor(170, 170, 170));
	else if (!isChecked()){

		if (current() < good())
			option.palette = QPalette(Qt::black, QColor(20, 220, 20), Qt::gray, Qt::darkGray, QColor(170, 170, 170), Qt::black, Qt::red, Qt::green, QColor(0, 200, 0));
		else if (current() >= good() && current() < bad())
			option.palette = QPalette(Qt::black, QColor(220, 220, 20), Qt::gray, Qt::darkGray, QColor(170, 170, 170), Qt::black, Qt::red, Qt::yellow, QColor(200, 200, 0));
		else
			option.palette = QPalette(Qt::black, QColor(220, 20, 20), Qt::gray, Qt::darkGray, QColor(170, 170, 170), Qt::black, Qt::red, Qt::red, QColor(200, 0, 0));
	}
	else
		option.palette = QPalette(Qt::black, QColor(225, 225, 225, 100), Qt::gray, Qt::gray, QColor(225, 225, 225), Qt::gray, Qt::gray, Qt::gray, QColor(225, 225, 225));

	style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);
}

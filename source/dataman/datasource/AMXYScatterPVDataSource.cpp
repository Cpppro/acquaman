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


#include "AMXYScatterPVDataSource.h"

AMXYScatterPVDataSource::AMXYScatterPVDataSource(const AMProcessVariable *x, const AMProcessVariable *y, const QString &name, QObject *parent)
	: QObject(parent), AMDataSource(name)
{
	x_ = x;
	y_ = y;

	connect(x_, SIGNAL(valueChanged()), this, SLOT(onDataChanged()));
	connect(x_, SIGNAL(hasValuesChanged(bool)), this, SLOT(onStateChanged()));
	connect(y_, SIGNAL(valueChanged()), this, SLOT(onDataChanged()));
	connect(y_, SIGNAL(hasValuesChanged(bool)), this, SLOT(onStateChanged()));

	AMAxisInfo ai("xValue", 0, "X-value");
	ai.isUniform = true;
	ai.size = 1;
	axes_ << ai;
}

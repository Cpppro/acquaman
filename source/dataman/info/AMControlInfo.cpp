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

#include "AMControlInfo.h"

AMControlInfo::AMControlInfo(const QString& name, double value, double minimum, double maximum, const QString& units, double tolerance, const QString &description, const QString &contextKnownDescription, const QString& enumString, QObject* parent)
	: AMDbObject(parent)
{
	setName(name);
	value_ = value;
	minimum_ = minimum;
	maximum_ = maximum;
	units_ = units;
	tolerance_ = tolerance;
	description_ = description;
	contextKnownDescription_ = contextKnownDescription;
	enumString_ = enumString;
}
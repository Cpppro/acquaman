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

#ifndef AMANALYSISBLOCKINTERFACE_H
#define AMANALYSISBLOCKINTERFACE_H

#include <QtPlugin>

class AMAnalysisBlockInterface
{
public:
	virtual ~AMAnalysisBlockInterface() {}
	virtual bool accepts(const QString &format) = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(AMAnalysisBlockInterface,
		    "AMAnalysisBlockInterface/1.0");
QT_END_NAMESPACE

#endif // AMFILELOADERINTERFACE_H

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


#include "AMExporterOptionGeneralAscii.h"
#include <QPushButton>
#include "ui/dataman/AMExporterOptionGeneralAsciiView.h"

AMExporterOptionGeneralAscii::AMExporterOptionGeneralAscii(QObject *parent) :
	AMExporterOptionGeneral(parent)
{
	columnDelimiter_ = "\t";
	newlineDelimiter_ = "\r\n";

	fileName_ = "$name_$number_$dateTime[yyyyMMdd_hhmmss].txt";
	separateSectionFileName_ = "$name_$number_$dataSetName_$dateTime[yyyyMMdd_hhmmss].txt";

	setModified(false);
}

const QMetaObject* AMExporterOptionGeneralAscii::getMetaObject(){
	return metaObject();
}

QWidget * AMExporterOptionGeneralAscii::createEditorWidget()
{
	return new AMExporterOptionGeneralAsciiView(this, availableDataSourcesModel_);
}

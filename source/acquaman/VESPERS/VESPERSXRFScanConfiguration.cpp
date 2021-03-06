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


#include "VESPERSXRFScanConfiguration.h"
#include "acquaman/VESPERS/VESPERSXRFScanController.h"

VESPERSXRFScanConfiguration::VESPERSXRFScanConfiguration(XRFDetectorInfo detectorInfo, QObject *parent)
	: AMScanConfiguration(parent)
{
	xrfDetectorInfo_ = detectorInfo;
	setAutoExportEnabled(false);
}

VESPERSXRFScanConfiguration::VESPERSXRFScanConfiguration(QObject *parent)
	: AMScanConfiguration(parent)
{
	setAutoExportEnabled(false);
}

VESPERSXRFScanConfiguration::~VESPERSXRFScanConfiguration()
{

}

AMScanConfiguration *VESPERSXRFScanConfiguration::createCopy() const
{
	return new VESPERSXRFScanConfiguration(*this);
}

AMScanController *VESPERSXRFScanConfiguration::createController()
{
	return new VESPERSXRFScanController(this);
}

QString VESPERSXRFScanConfiguration::detailedDescription() const
{
	if (!xrfDetectorInfo_.name().isEmpty())
		return QString("XRF Free Run Scan\nDetector: %1\nReal time: %2 s").arg(xrfDetectorInfo_.name()).arg(xrfDetectorInfo_.integrationTime());

	return QString();
}

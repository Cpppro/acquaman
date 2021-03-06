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

#include "AMScanConfigurationViewHolder2.h"

#include <QRadioButton>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QButtonGroup>

#include "acquaman/AMScanConfiguration.h"
#include "ui/acquaman/AMScanConfigurationView.h"
#include "actions2/actions/AMScanControllerAction.h"

AMScanConfigurationViewHolder2::AMScanConfigurationViewHolder2(AMScanConfigurationView* view, QWidget *parent) :
	AMActionRunnerAddActionBar("Scan", parent)
{
	view_ = view;

	if(view_)
		addWidget(view_);
}

void AMScanConfigurationViewHolder2::setView(AMScanConfigurationView *view) {
	// delete old view, if it exists
	if(view_)
		delete view_;

	view_ = view;
	if(view_) {
		addWidget(view_);
	}
}


AMAction * AMScanConfigurationViewHolder2::createAction()
{
	if(view_)
		return new AMScanControllerAction(new AMScanControllerActionInfo(view_->configuration()->createCopy()));

	return 0;
}

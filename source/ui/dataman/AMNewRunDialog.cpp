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



#include "AMNewRunDialog.h"
#include "util/AMErrorMonitor.h"
#include "dataman/database/AMDbObjectSupport.h"

//Constructor:

AMNewRunDialog:: AMNewRunDialog(AMDatabase* db, QWidget *parent)
	: QDialog(parent)
{
	database_ = db;
	//setting up the layout of the entire window

	QLabel *facilitiesLabel = new QLabel(tr("Facilities"));
	QLabel *runNameLineLabel = new QLabel(tr("New Run Name:"));
	QLabel* informationLabel = new QLabel(tr("<b>Create a new run</b><br><br>These runs will be used to organize your data for each visit to the facility.<br>"));
	QLabel* dateWarningLabel = new QLabel(tr("The run date will be automatically added to this name."));
	QPushButton *okButton = new QPushButton("Ok");
	QPushButton *cancelButton = new QPushButton("Cancel");
	facilitySelectCb = new QComboBox();
	runNameLineEdit = new QLineEdit();



	addRunsAndFacilitiesLayout = new QGridLayout;

	addRunsAndFacilitiesLayout->addWidget(informationLabel, 0,0,1,2,Qt::AlignLeft);
	addRunsAndFacilitiesLayout->addWidget(facilitiesLabel,1,0);
	addRunsAndFacilitiesLayout ->addWidget(facilitySelectCb, 1, 1);
	addRunsAndFacilitiesLayout ->addWidget(runNameLineLabel,2,0);
	addRunsAndFacilitiesLayout ->addWidget(runNameLineEdit, 2, 1, 1, 2);
	addRunsAndFacilitiesLayout->addWidget(dateWarningLabel, 3, 1, 1, 2);
	addRunsAndFacilitiesLayout ->addWidget(okButton, 4, 2);
	addRunsAndFacilitiesLayout ->addWidget(cancelButton, 4, 3);
	setLayout(addRunsAndFacilitiesLayout);

	addFacility(); // added all facilities in database to the combo box
	facilitySelectCbChanged(0);
	connect(okButton,SIGNAL(clicked()), this, SLOT(okButtonPressed()));

//when user selects a different facility in the combobox, must append facility name to line edit
	connect(facilitySelectCb, SIGNAL(activated(int)),this, SLOT(facilitySelectCbChanged(int)));

	connect(cancelButton,SIGNAL(clicked()),this, SLOT(cancelButtonPressed()));
}


AMNewRunDialog::~AMNewRunDialog()
{
}

/// This function scans the database for facilities and their thumbnail and adds the facility description and it's corresponding thumbnail to the facilitySelectCb combobox. Other metadata are added as user role or tooltip role.
void AMNewRunDialog::addFacility(){

	//Before adding anything into the combobox, make sure nothing is in it
	facilitySelectCb->clear();

	// searching database for the required components
	QSqlQuery q = database_->query();
	/* NTBA - September 1st, 2011 (David Chevrier)
	"Hard-coded database table names. Down to only AMDbObjectThumbnails_table."
	*/
	q.prepare(QString("SELECT %1.description,%1.name,AMDbObjectThumbnails_table.thumbnail,AMDbObjectThumbnails_table.type,%1.id "
			  "FROM %1,AMDbObjectThumbnails_table WHERE %1.thumbnailFirstId = AMDbObjectThumbnails_table.id "
			  "ORDER BY %1.id DESC").arg(AMDbObjectSupport::s()->tableNameForClass<AMFacility>()));
	int i = 0;
	if (q.exec()) {
		while (q.next()) {
			facilitySelectCb->addItem(QString(q.value(0).toString()));  //Adding facilities description

			if(q.value(3).toString() == "PNG") {      // Checking if thumbnail type is PNG
				QPixmap p;
				if(p.loadFromData(q.value(2).toByteArray(), "PNG"))  // Converting thumbnail to byte array and storing it as decoration role
					facilitySelectCb->setItemData(i, p.scaledToHeight(22, Qt::SmoothTransformation), Qt::DecorationRole);
			}
			facilitySelectCb->setItemData(i,q.value(0).toString(), Qt::ToolTipRole);  //Setting description as tool tip
			facilitySelectCb->setItemData(i,q.value(4).toInt(), AM::IdRole);		// Setting facility's ID in the User Role
			facilitySelectCb->setItemData(i, q.value(1).toString(), AM::DescriptionRole);
			i++;
		}
	}
	else {
		q.finish();
		AMErrorMon::report(AMErrorReport(this, AMErrorReport::Debug, 0, "Error retrieving information from the database."));
	}


}


/// This function takes the text in the combobox and puts it in the runNameLineEdit.
void AMNewRunDialog::facilitySelectCbChanged(int index) {


	runNameLineEdit->setText( facilitySelectCb->itemData(index, AM::DescriptionRole).toString() );
}

/// This function is activated when the okay button is pressed, and will store the contents of the line edit as the name of the new run and the current facility's id as the new run's facility id
void AMNewRunDialog::okButtonPressed(){
	QString runName = runNameLineEdit->text();
	//run AMRun constructor to create new run, but first, we need facility Id
	int facilityId = facilitySelectCb->itemData(facilitySelectCb->currentIndex(),AM::IdRole).toInt();

	AMRun newRun(runName, facilityId);
	bool success = newRun.storeToDb(AMDatabase::database("user"));


	if(success) {
		emit dialogBoxClosed(newRun.id());
	}
	else {
		emit dialogBoxClosed(-1);
	}

	accept();
}

/// This function will hide the dialog box if the cancel button is pressed. Also, the dialogBoxClosed signal will be emitted
void AMNewRunDialog::cancelButtonPressed(){
	emit dialogBoxClosed(-1);
	reject();
}

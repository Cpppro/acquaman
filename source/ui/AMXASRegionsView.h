/*
Copyright 2010, 2011 Mark Boots, David Chevrier.

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


#ifndef AMXASREGIONSVIEW_H
#define AMXASREGIONSVIEW_H

#include <QWidget>
#include <QtGui>

class QTableView;
class QPushButton;
class QHBoxLayout;
class QMenu;

#include "acquaman/AMRegionsList.h"

/// An AMXASRegionsView is a means of generating a default view for a group of AMXASRegion
/*!
  This default view utilizes a QTableView to interface with a group of AMXASRegion.
  By calling the constructor while passing a QList of AMXASRegion, the AMXASRegionsView:
  - creates add region and delete region buttons; and, \todo Hook these buttons up to something
  - uses the QList of AMXASRegion to make an AMXASRegionModel (the programmer need not do this nor understand how it works); and,
  - creates a table view on the newly created model.
  \todo This should most likely be an AMRegionsView ... there's no real need to have it at the AMXASRegion level. I'll fix this, I just needed to figure out if it would work in the first place.
  */
class AMXASRegionsView : public QWidget
{
Q_OBJECT
public:
	/// Constructor
	/*!
	  \param regions Pointer to a QList of AMXASRegions. Can be easily retreived using regionsPtr() function on an AMXASScanConfiguration or child.
	  \param parent Pointer to QWidget to act as parent.
	  */
	explicit AMXASRegionsView(AMXASRegionsList *regions, QWidget *parent = 0);
	~AMXASRegionsView();

signals:
	void addRegionClicked();

public slots:
	void setBeamlineEnergy(AMControl *beamlineEnergy){regions_->setEnergyControl(beamlineEnergy);}
	bool addRegion();
	bool deleteRegion();
	void setInsertIndex();
	void setRemoveIndex();

	bool insertRegion(){ return regions_->addRegionSqueeze(insertIndex_); }
	bool removeRegion(){ return regions_->deleteRegionSqueeze(removeIndex_); }

	void setDisabled(bool disabled);

protected:
	void resizeEvent(QResizeEvent *event);

protected:
	/// The table view used to interact with the group of AMXASRegion.
	QTableView *tv_;

	/// Button to add a region. \todo Figure out interface and hook up to something.
	QPushButton *addButton_;
	/// Button to delete a region. \todo Figure out interface and hook up to something.
	QPushButton *deleteButton_;
	/// The model created from the AMXASRegion group to be used with the table view.
	AMXASRegionsList *regions_;
	QMenu *addRegionMenu_;
	QMenu *deleteRegionMenu_;
	int insertIndex_;
	int removeIndex_;
};

#endif // AMXASREGIONSVIEW_H

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


#include "AMControlInfoList.h"

AMControlInfo::AMControlInfo(const QString& name, double value, double minimum, double maximum, const QString& units, QObject* parent)
	: AMDbObject(parent)
{
	setName(name);
	value_ = value;
	minimum_ = minimum;
	maximum_ = maximum;
	units_ = units;
}

AMControlInfo::AMControlInfo(AMDatabase *db, int id)
	: AMDbObject() {

	setName("Invalid Control Info");
	value_ = 0;
	minimum_ = 0;
	maximum_ = 0;

	loadFromDb(db, id);
}

AMControlInfoList::AMControlInfoList(QObject *parent)
	: AMDbObject(parent), AMOrderedList<AMControlInfo>()
{
	connect(signalSource(), SIGNAL(itemAdded(int)), this, SLOT(onControlAdded(int)));
	connect(signalSource(), SIGNAL(itemRemoved(int)), this, SLOT(onControlRemoved(int)));
	connect(signalSource(), SIGNAL(itemChanged(int)), this, SLOT(onControlValuesChanged(int)));
}

AMControlInfoList::AMControlInfoList(const AMControlInfoList& other)
	: AMDbObject(other), AMOrderedList<AMControlInfo>(other)
{
	connect(signalSource(), SIGNAL(itemAdded(int)), this, SLOT(onControlAdded(int)));
	connect(signalSource(), SIGNAL(itemRemoved(int)), this, SLOT(onControlRemoved(int)));
	connect(signalSource(), SIGNAL(itemChanged(int)), this, SLOT(onControlValuesChanged(int)));
}

AMControlInfoList::AMControlInfoList(AMDatabase* db, int id)
	: AMDbObject(), AMOrderedList<AMControlInfo>() {

	connect(signalSource(), SIGNAL(itemAdded(int)), this, SLOT(onControlAdded(int)));
	connect(signalSource(), SIGNAL(itemRemoved(int)), this, SLOT(onControlRemoved(int)));
	connect(signalSource(), SIGNAL(itemChanged(int)), this, SLOT(onControlValuesChanged(int)));

	loadFromDb(db, id);
}

AMControlInfoList& AMControlInfoList::operator=(const AMControlInfoList& other) {
	// always: check for self-assignment
	if(this != &other) {
		AMOrderedList<AMControlInfo>::operator=(other);
		AMDbObject::operator=(other);

		// if the items in other have been modified, but the signal hasn't been emitted yet, other.modified_ will not be set to true yet. We know that things have changed, so ours should be true.
		if(other.signalSource()->delayedItemChangedScheduled())
			setModified(true);
	}

	return *this;
}

// Returns a list of pointers to the AMControlInfo objects we store, for use by the database system in storeToDb() / loadFromDb().
AMDbObjectList AMControlInfoList::dbReadControlInfos() {
	AMDbObjectList rv;
	for(int i=0; i<count(); i++)
		rv << &((*this)[i]);
	return rv;
}

// Called by the database system on loadFromDb() to give us our new set of AMControlInfo objects. We copy these ones into our internal list and then delete them.
void AMControlInfoList::dbLoadControlInfos(const AMDbObjectList& newControlInfos) {
	clear();	// get rid of our existing

	for(int i=0; i<newControlInfos.count(); i++) {
		AMControlInfo* newControlInfo = qobject_cast<AMControlInfo*>(newControlInfos.at(i));
		if(newControlInfo) {
			append(*newControlInfo);	// note: makes a copy of object pointed to by newControlInfo, and stores in our internal list.
		}

		if(newControlInfos.at(i))
			delete newControlInfos.at(i);	// we're copying these; don't need to keep these ones around. Our responsibility to delete.
	}
}


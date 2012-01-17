/*
Copyright 2010, 2011 Mark Boots, David Chevrier, and Darren Hunter.

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


#include "AMDbObject.h"
#include "dataman/database/AMDbObjectSupport.h"
#include "acquaman.h"

#include <QMetaType>
#include "dataman/AMnDIndex.h"
#include <QVector3D>
#include <QtConcurrentRun>
#include <QStringBuilder>

// Default constructor
AMDbThumbnail::AMDbThumbnail(const QString& Title, const QString& Subtitle, ThumbnailType Type, const QByteArray& ThumbnailData)
	: title(Title), subtitle(Subtitle), type(Type), thumbnail(ThumbnailData) {
}

// This constructor takes an image of any size and saves it as a PNG type. (It will be saved at the current size of the image, so if you want to save at a reduced size, pass in image.scaledToWidth(240) or similar.)
AMDbThumbnail::AMDbThumbnail(const QString& Title, const QString& Subtitle, const QImage& image)
	: title(Title), subtitle(Subtitle) {

	if(image.isNull()) {
		type = InvalidType;
		thumbnail = QByteArray();
	}
	else {
		QBuffer bout;
		bout.open(QIODevice::WriteOnly);
		if(image.save(&bout, "PNG")) {
			type = PNGType;
			thumbnail = bout.buffer();
		}
		else {
			type = InvalidType;
			thumbnail = QByteArray();
		}
	}
}



QString AMDbThumbnail::typeString() const {
	switch(type) {
	case PNGType:
		return "PNG";
		break;
	case InvalidType:
	default:
		return "Invalid";
		break;
	}
}

AMDbObject::AMDbObject(QObject *parent) : QObject(parent) {
	isReloading_ = false;
	id_ = 0;
	database_ = 0;
	modified_ = true;

	name_ = "Unnamed Object";

}

AMDbObject::AMDbObject(const AMDbObject &original) : QObject() {
	isReloading_ = false;
	id_ = original.id_;
	database_ = original.database_;
	modified_ = original.modified_;
	name_ = original.name_;
}

AMDbObject& AMDbObject::operator=(const AMDbObject& other) {
	if(this != &other) {
		id_ = other.id_;
		database_ = other.database_;
		name_ = other.name_;
		setModified(other.modified_);
	}

	return *this;
}

#include <QMetaClassInfo>
QString AMDbObject::dbObjectAttribute(const QString& key) const {
	return AMDbObjectSupport::dbObjectAttribute(this->metaObject(), key);
}


QString AMDbObject::dbPropertyAttribute(const QString& propertyName, const QString& key) const {
	return AMDbObjectSupport::dbPropertyAttribute(this->metaObject(), propertyName, key);
}


bool AMDbObject::isReloading() const{
	return isReloading_;
}

// returns the name of the database table where objects like this should be/are stored
QString AMDbObject::dbTableName() const {
	return AMDbObjectSupport::s()->tableNameForClass(this->metaObject());
}

// If this class has already been registered in the AMDbObject system, returns a pointer to the AMDbObjectInfo describing this class's persistent properties.  If the class hasn't been registered, returns 0;
const AMDbObjectInfo* AMDbObject::dbObjectInfo() const {
	return AMDbObjectSupport::s()->objectInfoForClass( type() );
}


// This member function updates a scan in the database (if it exists already in that database), otherwise it adds it to the database.
bool AMDbObject::storeToDb(AMDatabase* db, bool generateThumbnails) {

	if(!db)
		return false;

	const AMDbObjectInfo* myInfo = dbObjectInfo();
	if(!myInfo)
		return false;	// class has not been registered yet in the database system.

	QTime saveTime;
	qDebug() << "Starting storeToDb() of" << myInfo->className;
	saveTime.start();

	// For performance when storing many child objects, we can speed things up (especially with SQLite, which needs to do a flush and reload of the db file on every write) by doing all the updates in one big transaction. This also ensure consistency.  Because storeToDb() calls could be nested, we don't want to start a transaction if one has already been started.
	bool openedTransaction = false;
	if(db->supportsTransactions() && !db->transactionInProgress()) {
		if(db->startTransaction()) {
			openedTransaction = true;
			qDebug() << "Opening transaction for save of " << myInfo->tableName << id();
		}
		else {
			AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -496, "Could not start a transaction to save the object '" % myInfo->tableName % ":" % QString::number(id()) % "' in the database. Please report this problem to the Acquaman developers."));
			return false;
		}
	}
	// If this object has never been stored to this database before, we could optimize some things.
	bool neverSavedHere = (id()<1 || db !=database());

	QVariantList values;	// list of values to store
	QStringList keys;	// list of keys (column names) to store

	// determine and append the type. (Necessary to know for later, when storing objects of different types in the same table)
	keys << "AMDbObjectType";
	values << type();

	// store all the columns:
	//////////////////////////////////////////////////
	for(int i=0; i<myInfo->columnCount; i++) {

		int columnType = myInfo->columnTypes.at(i);
		QByteArray columnNameBA = myInfo->columns.at(i).toAscii();
		const char* columnName = columnNameBA.constData();

		// add column name to key list... UNLESS the type is an AMDbObjectList. In that case, it gets its own table instead of a column.
		if(columnType != qMetaTypeId<AMDbObjectList>())
			keys << myInfo->columns.at(i);

		// add value to values list. First, some special processing is needed for StringList, IntList, and DoubleList types, to join their values into a single string. Other property types simply get written out in their native QVariant form. EXCEPTION: AMDbObjectList doesn't get written here; it gets its own table later.

		if(columnType == qMetaTypeId<AMnDIndex>()) {

			AMnDIndex output = property(columnName).value<AMnDIndex>();
			QStringList resultString;
			for(int i=0; i<output.size(); i++)
				resultString << QString("%1").arg(output[i]);
			values << resultString.join(AMDbObjectSupport::listSeparator());

		}
		else if(columnType == qMetaTypeId<AMIntList>()) {
			AMIntList intList = property(columnName).value<AMIntList>();
			QStringList resultString;
			foreach(int i, intList)
				resultString << QString("%1").arg(i);
			values << resultString.join(AMDbObjectSupport::listSeparator());
		}

		else if(columnType == qMetaTypeId<AMDoubleList>()) {
			AMDoubleList doubleList = property(columnName).value<AMDoubleList>();
			QStringList resultString;
			foreach(double d, doubleList)
				resultString << QString("%1").arg(d);
			values << resultString.join(AMDbObjectSupport::listSeparator());
		}

		else if(columnType == qMetaTypeId<QVector3D>()) {
			QVector3D val = property(columnName).value<QVector3D>();
			QStringList resultString;
			resultString << QString::number(val.x()) << QString::number(val.y()) << QString::number(val.z());
			values << resultString.join(AMDbObjectSupport::listSeparator());
		}

		else if(columnType == QVariant::StringList || columnType == QVariant::List) {	// string lists, or lists of QVariants that can (hopefully) be converted to strings.
			values << property(columnName).toStringList().join(AMDbObjectSupport::stringListSeparator());
		}

		// special case: pointers to AMDbObjects: we actually store the object in the database, and then store a string "tableName;id"... which will let us re-load it later.
		else if(columnType == qMetaTypeId<AMDbObject*>()) {
			AMDbObject* obj = property(columnName).value<AMDbObject*>();
			if(obj && obj!=this) {	// if its a valid object, and not ourself (avoid recursion)
				if(!obj->modified() && obj->database()==db && obj->id() >=1)	// if it's not modified, and already part of this database... don't need to store it. Just remember where it is...
					values << QString("%1%2%3").arg(obj->dbTableName()).arg(AMDbObjectSupport::listSeparator()).arg(obj->id());
				else {
					if(obj->storeToDb(db))
						values << QString("%1%2%3").arg(obj->dbTableName()).arg(AMDbObjectSupport::listSeparator()).arg(obj->id());
					else
						values << QString();// storing empty string: indicates failure to save object here.
				}
			}
			else
				values << QString();// storing empty string: indicates invalid object to save here.
		}

		// special case: lists of AMDbObject pointers. Interpreted as a one-to-many (or maybe many-to-many) relationship.
		else if(columnType == qMetaTypeId<AMDbObjectList>()) {
			// don't do anything here. Instead, we'll save all the objects, and add their location entries, once we know our id
			// most importantly, DON'T add anything to values, since we didn't add a matching key.
		}

		// everything else
		else
			values << property(columnName);
	}
	////////////////////////////////////////



	// Add thumbnail info (just the count for now: 0) We will update later once we store thumbnails (possibly in another thread).
	keys << "thumbnailCount";
	values << 0;

	// store type, thumbnailCount, and all metadata into the table.
	int retVal;
	// If saving into same database, can use existing id():
	if(database() == db)
		retVal = db->insertOrUpdate(id(), myInfo->tableName, keys, values);
	// otherwise, use id of 0 to insert new.
	else
		retVal = db->insertOrUpdate(0, myInfo->tableName, keys, values);


	// Did the update succeed?
	if(retVal == 0) {
		if(openedTransaction)
			db->rollbackTransaction();	// this is good for consistency. Even all the child object changes will be reverted if this save failed.
		return false;
	}

	// Success! We have our new / old id:
	id_ = retVal;
	database_ = db;


	// AMDbObjectList associated objects save
	////////////////////////////////////////////
	for(int i=0; i<myInfo->columnCount; i++) {
		if(myInfo->columnTypes.at(i) == qMetaTypeId<AMDbObjectList>()) {	// only do this for AMDbObjectList types...

			QByteArray columnNameBA = myInfo->columns.at(i).toAscii();
			const char* columnName = columnNameBA.constData();

			AMDbObjectList objList = property(columnName).value<AMDbObjectList>();
			QString auxTableName = myInfo->tableName + "_" + myInfo->columns.at(i);
			// delete old entries for this object and property:
			db->deleteRows(auxTableName, QString("id1 = '%1'").arg(id()));

			QStringList clist;
			clist << "id1" << "table1" << "id2" << "table2";
			QVariantList vlist;
			vlist << id() << myInfo->tableName << int(0) << "tableName2";	// int(0) and "tableName2" are dummy variables for now.
			foreach(AMDbObject* obj, objList) {
				if(obj && obj!=this) {	// verify that this is a valid object, and not ourself (to avoid infinite recursion)
					if(  (!obj->modified() && obj->database() == db && obj->id() >=1) ) {	// if this object is unmodified and already stored in the database, just remember its location. (This avoids storeToDb()'ing objects that are unmodified and do not need re-saving.)
						vlist[2] = obj->id();
						vlist[3] = obj->dbTableName();
						db->insertOrUpdate(0, auxTableName, clist, vlist);
					}
					else if( obj->storeToDb(db) ) {	// otherwise, store the object and remember its location
						vlist[2] = obj->id();
						vlist[3] = obj->dbTableName();
						db->insertOrUpdate(0, auxTableName, clist, vlist);
					}
					else {	// problem storing the object...
						AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -47, QString("While storing '%1' to the database, there was an error trying to store its child object '%2'").arg(this->name()).arg(obj->name())));
						// We let this slide and don't give up on ourself and the rest of the child objects.
					}
				}
			}
		}
	}


	// Thumbnail save (if we're supposed to do it in this thread)
	///////////////////////////////////////////

	if(generateThumbnails && thumbnailCount() > 0 && !shouldGenerateThumbnailsInSeparateThread())
		updateThumbnailsInCurrentThread(neverSavedHere);

	// NOTE: currently there are a few situations where we are "leaking" thumbnails: leaving old stale thumbnails in the database. Ex: When the thumbnailCount() was non-zero on a previous save to this database, and is now 0. Or when the thumbnailCount() was non-zero on a previous save to the database, and generateThumbnails has been forced to false this time. That's not such a big deal... they're not referenced by anything, and they'll get removed next time we store valid thumbnails.

	// finalizing... Commit the transaction if we opened it.
	if(openedTransaction) {
		if(db->commitTransaction()) {
			// we were just stored to the database, so our properties must be in sync with it.
			setModified(false);
			qDebug() << "Finished save of " << myInfo->className << "in" << saveTime.elapsed() << "ms; transaction committed.";
			if(shouldGenerateThumbnailsInSeparateThread() && generateThumbnails && thumbnailCount() > 0) {
				QtConcurrent::run(&AMDbObject::updateThumbnailsInSeparateThread, db, id_, myInfo->tableName, neverSavedHere);
			}
			return true;
		}
		else {
			db->rollbackTransaction();
			AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -497, "Could not commit a transaction to save the object '" % myInfo->tableName % ":" % QString::number(id()) % "' in the database. Please report this problem to the Acquaman developers."));
			return false;
		}
	}
	// if we didn't open the transaction, no need to commit it.
	else {
		qDebug() << "Finished save of " << myInfo->className << "in" << saveTime.elapsed() << "ms;  not closing transaction.";
		// we were just stored to the database, so our properties must be in sync with it.
		setModified(false);
		if(shouldGenerateThumbnailsInSeparateThread() && generateThumbnails && thumbnailCount() > 0) {
			QtConcurrent::run(&AMDbObject::updateThumbnailsInSeparateThread, db, id_, myInfo->tableName, neverSavedHere);
		}
		return true;
	}
}



// load a AMDbObject (set its properties) by retrieving it based on id.
bool AMDbObject::loadFromDb(AMDatabase* db, int sourceId) {

	// All valid database id's start at 1. This is an optimization to omit the db query if it won't find anything.
	if(sourceId < 1)
		return false;

	const AMDbObjectInfo* myInfo = dbObjectInfo();
	if(!myInfo)
		return false;	// class hasn't been registered yet with the database system.


	// Retrieve all columns from the database.
	// optimization: not necessary to retrieve anything with the doNotLoad attribute set. Also, if the type is AMDbObjectList, there is no actual database column for this "column"... instead, its an auxiliary table.
	QStringList keys;	// keys is the set of database columns to retrieve; all the columns that are loadable, and are not of type AMDbObjectList.
	for(int i=0; i<myInfo->columnCount; i++)
		if(myInfo->columnTypes.at(i) != qMetaTypeId<AMDbObjectList>() && myInfo->isLoadable.at(i))
			keys << myInfo->columns.at(i);

	QVariantList values = db->retrieve( sourceId, myInfo->tableName, keys);

	if(values.isEmpty())
		return false;

	isReloading_ = true;
	// if we just successfully loaded out of here, then we have our new id() and database().
	id_ = sourceId;
	database_ = db;

	int ri = 0;	// the AMDbObjectInfo::columnCount will not match the number of columns we retrieved, given that some are omitted. This is the index in the retrieved columns 'values'. It will become offset from 'i' in the loop below if there are any non-loadable columns, or AMDbObjecList columns which don't have actual database columns.
	// go through all results and restore properties
	for(int i=0; i<myInfo->columnCount; i++) {

		// do not re-load this column?
		if(!myInfo->isLoadable.at(i))
			continue;

		QByteArray columnNameBA = myInfo->columns.at(i).toAscii();
		const char* columnName = columnNameBA.constData();
		int columnType = myInfo->columnTypes.at(i);

		// special action necessary to convert StringList, IntList, and DoubleList types which have been returned as strings, as well as re-load AMDbObjects or lists of AMDbObjects that are owned by this object. Determine based on column type:

		// if its an AMDbObjectList property, it doesn't have an actual column. Look in the auxiliary table instead.
		if(columnType == qMetaTypeId<AMDbObjectList>()) {
			// grab current AMDbObjectList using property() and check if existing count and types match. In that case, can call loadFromDb() on each of them.
			// otherwise, create new objects with createAndLoadObjectAt(), and then call setProperty().
			AMDbObjectList reloadedObjects;
			AMDbObjectList existingObjects = property(columnName).value<AMDbObjectList>();

			QString auxTableName = myInfo->tableName + "_" + myInfo->columns.at(i);
			QList<int> storedObjectRows = db->objectsMatching(auxTableName, "id1", id());

			bool canUseExistingObjects = (storedObjectRows.count() == existingObjects.count());	// one prereq for reloading using existing objects: count is the same.
			QStringList storedObjectTables;
			QList<int> storedObjectIds;
			QStringList clist;  clist << "id2" << "table2";
			for(int r=0; r<storedObjectRows.count(); r++) {
				QVariantList objectLocation = db->retrieve(storedObjectRows.at(r), auxTableName, clist);
				if(objectLocation.isEmpty())
					return false;
				QString objectTable = objectLocation.at(1).toString();
				int objectId = objectLocation.at(0).toInt();
				storedObjectTables << objectTable;
				storedObjectIds << objectId;

				// second prereq for re-using existing objects is that current types and stored types match.
				canUseExistingObjects = (canUseExistingObjects &&
										 existingObjects.at(r)->type() == AMDbObjectSupport::typeOfObjectAt(db, objectTable, objectId) );
			}

			if(canUseExistingObjects) {
				for(int r=0; r<existingObjects.count(); r++)
					existingObjects.at(r)->loadFromDb(db, storedObjectIds.at(r));
			}
			else {
				for(int r=0; r<storedObjectIds.count(); r++)
					reloadedObjects << AMDbObjectSupport::s()->createAndLoadObjectAt(db, storedObjectTables.at(r), storedObjectIds.at(r));
				setProperty(columnName, QVariant::fromValue(reloadedObjects));
			}
		}
		// in all other cases, we're using up actual columns in the result set, so ri should be incremented after all of these:
		else {

			if(columnType == qMetaTypeId<AMDbObject*>()) {	// stored owned AMDbObject. reload from separate location in database.
				QStringList objectLocation = values.at(ri).toString().split(AMDbObjectSupport::listSeparator());	// location was saved as string: "tableName;id"
				if(objectLocation.count() == 2) {
					QString tableName = objectLocation.at(0);
					int dbId = objectLocation.at(1).toInt();
					AMDbObject* existingObject = property(columnName).value<AMDbObject*>();
					// have a valid existing object, and its type matches the type to load? Just call loadFromDb() and keep the existing object.
					if(existingObject && existingObject->type() == AMDbObjectSupport::typeOfObjectAt(db, tableName, dbId))
						existingObject->loadFromDb(db, dbId);
					else {
						AMDbObject* reloadedObject = AMDbObjectSupport::s()->createAndLoadObjectAt(db, tableName, dbId);
						if(reloadedObject)
							setProperty(columnName, QVariant::fromValue(reloadedObject));
						else
							setProperty(columnName, QVariant::fromValue((AMDbObject*)0));	// if it wasn't reloaded successfully, you'll still get a setProperty call, but it will be with a null pointer.
					}
				}
				else
					setProperty(columnName, QVariant::fromValue((AMDbObject*)0));	// if it wasn't reloaded successfully, you'll still get a setProperty call, but it will be with a null pointer.

			}
			else if(columnType == qMetaTypeId<AMnDIndex>()) {
				AMnDIndex ndIndex;
				QStringList stringList = values.at(ri).toString().split(AMDbObjectSupport::listSeparator(), QString::SkipEmptyParts);
				foreach(QString i, stringList)
					ndIndex.append(i.toInt());
				setProperty(columnName, QVariant::fromValue(ndIndex));
			}
			else if(columnType == qMetaTypeId<AMIntList>()) {	// integer lists: must convert back from separated string.
				AMIntList intList;
				QStringList stringList = values.at(ri).toString().split(AMDbObjectSupport::listSeparator(), QString::SkipEmptyParts);
				foreach(QString i, stringList)
					intList << i.toInt();
				setProperty(columnName, QVariant::fromValue(intList));
			}
			else if(columnType == qMetaTypeId<AMDoubleList>()) {	// double lists: must convert back from separated string.
				AMDoubleList doubleList;
				QStringList stringList = values.at(ri).toString().split(AMDbObjectSupport::listSeparator(), QString::SkipEmptyParts);
				foreach(QString d, stringList)
					doubleList << d.toDouble();
				setProperty(columnName, QVariant::fromValue(doubleList));
			}
			else if(columnType == qMetaTypeId<QVector3D>()) {
				QVector3D vector;
				QStringList stringList = values.at(ri).toString().split(AMDbObjectSupport::listSeparator(), QString::SkipEmptyParts);
				if(stringList.size() == 3) {
					vector = QVector3D(stringList.at(0).toDouble(), stringList.at(1).toDouble(), stringList.at(2).toDouble());
				}
				else
					AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -57, "Couldn't find 3 numbers when attempting to load a 3D geometry point from the database."));
				setProperty(columnName, QVariant::fromValue(vector));
			}
			else if(columnType == QVariant::StringList || columnType == QVariant::List) {	// string list, and anything-else-lists saved as string lists: must convert back from separated string.
				setProperty(columnName, values.at(ri).toString().split(AMDbObjectSupport::stringListSeparator(), QString::SkipEmptyParts));
			}
			else {	// the simple case.
				setProperty(columnName, values.at(ri));
			}

			ri++;// we just used up this result column, so move on to the next.
		}
	}

	// we were just loaded out of the database, so we must be in-sync.
	setModified(false);

	isReloading_ = false;
	emit loadedFromDb();
	return true;
}



// This global function enables using the insertion operator to add objects to the database
///		ex: *Database::db() << myScan
// Because AMDbObject::storeToDb() is virtual, this version can be used properly for all sub-types of AMDbObject.
AMDatabase& operator<<(AMDatabase& db, AMDbObject& s) {
	s.storeToDb(&db);
	return db;
}

void AMDbObject::dissociateFromDb(bool shouldDissociateChildren)
{
	id_ = 0;
	database_ = 0;

	const AMDbObjectInfo* myInfo = dbObjectInfo();
	if(!myInfo)
		return;	// class has not been registered yet in the database system. Nothing to do.

	if(shouldDissociateChildren) {

		// Dissociating children. Go through all columns...
		for(int i=0; i<myInfo->columnCount; i++) {

			QByteArray columnNameBA = myInfo->columns.at(i).toAscii();
			const char* columnName = columnNameBA.constData();

			if(myInfo->columnTypes.at(i) == qMetaTypeId<AMDbObject*>()) {	// single child objects
				AMDbObject* obj = property(columnName).value<AMDbObject*>();
				if(obj && obj!=this)
					obj->dissociateFromDb(true);
			}
			if(myInfo->columnTypes.at(i) == qMetaTypeId<AMDbObjectList>()) {	// Lists of child objects
				AMDbObjectList objList = property(columnName).value<AMDbObjectList>();
				foreach(AMDbObject* obj, objList) {
					if(obj && obj!=this)
						obj->dissociateFromDb(true);
				}
			}
		}
	}
}

void AMDbObject::updateThumbnailsInSeparateThread(AMDatabase *db, int id, const QString& dbTableName, bool neverSavedHereBefore) {

	AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -313, "Storing AMScan thumbnail in separate thread..."));

	// Step 1: try to load the object.
	AMDbObject* object = AMDbObjectSupport::s()->createAndLoadObjectAt(db, dbTableName, id);
	if(!object) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -313, QString("AMDbObject: error trying to load object with ID %1 out of '%2' to create thumbnails. Please report this bug to the Acquaman developers.").arg(id).arg(dbTableName)));
		return;
	}

	// Find out how many thumbnails we're supposed to have:
	int thumbsCount = object->thumbnailCount();
	if(thumbsCount == 0) {
		return;	// nothing else to do...
	}

	QTime renderTime;
	renderTime.start();
	// Generating the thumbnails could take some time, especially for things that do complicated drawing (ie: scans with lots of points or multi-dim data). Let's do it all before hitting the database
	QList<AMDbThumbnail> thumbnails;
	for(int i=0; i<thumbsCount; i++)
		thumbnails << object->thumbnail(i);

	qDebug() << object->type() << "took" << renderTime.elapsed() << "ms to create thumbnails for saving.";
	renderTime.restart();

	// The remainder of this should happen in one database transaction. This ensures consistency, and it also increases performance because a database commit (and time consuming flush-to-disk, in the case of SQLite) doesn't have to happen for each thumbnail insert -- just once at the end.
	// Note that there might be a transaction started already...
	bool openedTransaction = false;
	if(db->supportsTransactions() && !db->transactionInProgress()) {
		if(db->startTransaction()) {
			openedTransaction = true;
			qDebug() << "Opened transaction for thumbnail save at" << dbTableName << id;
		}
		else {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -495, "Could not start a transaction to save the thumbnails for object '" % dbTableName % ":" % QString::number(id) % "' in the database. Please report this problem to the Acquaman developers."));
			return;
		}
	}

	bool reuseThumbnailIds = false;
	QList<int> existingThumbnailIds;

	if(!neverSavedHereBefore) {
		// Find out if there are any thumbnails for this object already in the DB:
		existingThumbnailIds = db->objectsWhere(AMDbObjectSupport::thumbnailTableName(), QString("objectId = %1 AND objectTableName = '%2'").arg(id).arg(dbTableName));
		// as long as this function works properly, these will always be in a sequential block.

		reuseThumbnailIds = (existingThumbnailIds.count() == thumbsCount);
		// need to check that the existingThumbnailIds are consecutive as well... Otherwise can't reuse them. They should always be in a consecutive block, but just in case the database has gotten messed up, it doesn't hurt to check.
		if(reuseThumbnailIds) {
			for(int i=1; i<existingThumbnailIds.count(); i++) {
				if(existingThumbnailIds.at(i) != existingThumbnailIds.at(i-1)+1) {
					reuseThumbnailIds = false;
					break;
				}
			}
		}

		if(!reuseThumbnailIds) {
			// Don't reuse existing rows in the thumbnail table. Instead, delete before appending new ones.
			db->deleteRows(AMDbObjectSupport::thumbnailTableName(), QString("objectId = %1 AND objectTableName = '%2'").arg(id).arg(dbTableName));
		}
	}

	QVariantList values;	// list of values to store
	QStringList keys;	// list of keys (column names) to store
	int firstThumbnailId;

	// Save each thumbnail:
	for(int i=0; i<thumbsCount; i++) {
		const AMDbThumbnail& t = thumbnails.at(i);

		keys.clear();
		values.clear();

		keys << "objectId";
		values << object->id();
		keys << "objectTableName";
		values << dbTableName;
		keys << "number";
		values << i;
		keys << "type";
		values << t.typeString();
		keys << "title";
		values << t.title;
		keys << "subtitle";
		values << t.subtitle;
		keys << "thumbnail";
		values << t.thumbnail;

		int retVal;
		if(reuseThumbnailIds) {
			// qDebug() << "Thumbnail save: reusing row" << i+existingThumbnailIds.at(0) << "in other thread";
			retVal = db->insertOrUpdate(i+existingThumbnailIds.at(0), AMDbObjectSupport::thumbnailTableName(), keys, values);
		}
		else {
			// qDebug() << "THumnail save: inserting new row in other thread";
			retVal = db->insertOrUpdate(0, AMDbObjectSupport::thumbnailTableName(), keys, values);
		}
		if(retVal == 0) {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -314, QString("AMDbObject: error trying to save thumbnails for object with ID %1 in table '%2'. Please report this bug to the Acquaman developers.").arg(id).arg(dbTableName)));
			if(openedTransaction) {
				db->rollbackTransaction();
				return;
			}
		}

		if(i == 0)	// when inserting the first one... remember the id of this first thumbnail.
			firstThumbnailId = retVal;
	}

	// now that we know where the thumbnails are, update this in the object table
	if(!db->update(id, dbTableName,
				   QStringList() << "thumbnailCount" << "thumbnailFirstId",
				   QVariantList() << thumbsCount << firstThumbnailId)) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -315, QString("AMDbObject: error trying to store the updated thumbnail count and firstThumbnailId for database object %1 in table '%2'. Please report this bug to the Acquaman developers.").arg(id).arg(dbTableName)));
		if(openedTransaction) {
			db->rollbackTransaction();
			return;
		}
	}

	// only commit the transaction if we opened it.
	if(openedTransaction && !db->commitTransaction()) {
		db->rollbackTransaction();
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -495, "AMDbObject: Could not commit a transaction to save the thumbnails for object '" % dbTableName % ":" % QString::number(id) % "' in the database. Please report this problem to the Acquaman developers."));
	}

	qDebug() << object->type() << "took" << renderTime.elapsed() << "ms to store thumbnails in the database. Used own transaction = " << openedTransaction;

	// And now we're done with the object...
	delete object;
}

void AMDbObject::updateThumbnailsInCurrentThread(bool neverSavedHereBefore)
{
	QString databaseTableName = dbTableName();

	// Find out how many thumbnails we're supposed to have:
	int thumbsCount = thumbnailCount();
	if(thumbsCount == 0) {
		return;	// nothing else to do...
	}
	// Generating the thumbnails could take some time, especially for things that do complicated drawing (ie: scans with lots of points or multi-dim data). Let's do it all before hitting the database
	QList<AMDbThumbnail> thumbnails;
	for(int i=0; i<thumbsCount; i++)
		thumbnails << thumbnail(i);

	// The remainder of this should happen in one database transaction. This ensures consistency, and it also increases performance because a database commit (and time consuming flush-to-disk, in the case of SQLite) doesn't have to happen for each thumbnail insert -- just once at the end.
	// Note that there might be a transaction open already and we cannot nest transactions. Therefore, only start a transaction if not open already.
	bool openedTransaction = false;
	if(database()->supportsTransactions() && !database()->transactionInProgress()) {
		if(database()->startTransaction()) {
			openedTransaction = true;
		}
		else {
			AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -495, "Could not start a transaction to save the thumbnails for object '" % databaseTableName % ":" % QString::number(id()) % "' in the database. Please report this problem to the Acquaman developers."));
			return;
		}
	}

	// Do we need to delete or reuse some existing thumbnails?
	bool reuseThumbnailIds = false;
	QList<int> existingThumbnailIds;

	if(!neverSavedHereBefore) {	// optimization: only need to do this if we've been saved here before.

		// Find out if there are any thumbnails for this object already in the DB:
		existingThumbnailIds = database()->objectsWhere(AMDbObjectSupport::thumbnailTableName(), QString("objectId = %1 AND objectTableName = '%2'").arg(id()).arg(databaseTableName));

		reuseThumbnailIds = (existingThumbnailIds.count() == thumbsCount);	// can reuse if the number of old and new ones matches.
		// need to check that the existingThumbnailIds are consecutive as well... ie: in a sequential block. Otherwise can't reuse them.
		// normally this is the case, unless we've forgotten / orphaned some. Doesn't hurt to check.
		if(reuseThumbnailIds) {
			for(int i=1; i<existingThumbnailIds.count(); i++) {
				if(existingThumbnailIds.at(i) != existingThumbnailIds.at(i-1)+1) {
					reuseThumbnailIds = false;
					break;
				}
			}
		}

		// If we shouldn't reuse existing rows in the thumbnail table: delete old before appending new ones.
		if(!reuseThumbnailIds) {
			database()->deleteRows(AMDbObjectSupport::thumbnailTableName(), QString("objectId = %1 AND objectTableName = '%2'").arg(id()).arg(databaseTableName));
		}
	}

	QVariantList values;	// list of values to store
	QStringList keys;	// list of keys (column names) to store
	int firstThumbnailId;

	// Save each thumbnail:
	for(int i=0; i<thumbsCount; i++) {
		const AMDbThumbnail& t = thumbnails.at(i);

		keys.clear();
		values.clear();

		keys << "objectId";
		values << id();
		keys << "objectTableName";
		values << databaseTableName;
		keys << "number";
		values << i;
		keys << "type";
		values << t.typeString();
		keys << "title";
		values << t.title;
		keys << "subtitle";
		values << t.subtitle;
		keys << "thumbnail";
		values << t.thumbnail;

		int retVal;
		if(reuseThumbnailIds) {
			// qDebug() << "Thumbnail save: reusing row" << i+existingThumbnailIds.at(0) << "in main thread";
			retVal = database()->insertOrUpdate(i+existingThumbnailIds.at(0), AMDbObjectSupport::thumbnailTableName(), keys, values);
		}
		else {
			// qDebug() << "Thumbnail save: adding new row in main thread";
			retVal = database()->insertOrUpdate(0, AMDbObjectSupport::thumbnailTableName(), keys, values);
		}
		if(retVal == 0) {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -314, QString("AMDbObject: error trying to save thumbnails for object with ID %1 in table '%2'. Please report this bug to the Acquaman developers.").arg(id()).arg(databaseTableName)));
			if(openedTransaction) {
				database()->rollbackTransaction();
				return;
			}
		}

		if(i == 0)	// when inserting the first one... remember the id of this first thumbnail.
			firstThumbnailId = retVal;
	}

	// now that we know where the thumbnails are, update this in the object table
	if(!database()->update(id(),
						   databaseTableName,
						   QStringList() << "thumbnailCount" << "thumbnailFirstId",
						   QVariantList() << thumbsCount << firstThumbnailId)) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -315, QString("AMDbObject: error trying to store the updated thumbnail count and firstThumbnailId for database object %1 in table '%2'. Please report this bug to the Acquaman developers.").arg(id()).arg(databaseTableName)));
		if(openedTransaction) {
			database()->rollbackTransaction();
			return;
		}
	}

	// only commit the transaction if we started it.
	if(openedTransaction && !database()->commitTransaction()) {
		database()->rollbackTransaction();
		AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -495, "Could not commit a transaction to save the thumbnails for object '" % databaseTableName % ":" % QString::number(id()) % "' in the database. Please report this problem to the Acquaman developers."));
	}
}



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


#include "AMDbObjectSupport.h"

#include "dataman/database/AMDbObject.h"

#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaClassInfo>
#include "util/AMErrorMonitor.h"

#include <QStringBuilder>
#include <QHashIterator>

#include <QMutexLocker>
#include <QReadLocker>
#include <QWriteLocker>
#include <QThread>
#include <QApplication>

// fill the className, tableName, metaObject, columns, columnTypes, isVisible, isLoadable, and doNotReuseIds properties based on a prototype AMDbObject.
AMDbObjectInfo::AMDbObjectInfo(AMDbObject* prototype) {
	initWithMetaObject(prototype->metaObject());
}

// fill the className, tableName, metaObject, columns, columnTypes, isVisile, isLoadable, and doNotReuseIds properties based on a prototype object's QMetaObject. (This version doesn't require an instance. The \c classMetaObject can be retrieved statically with Class::staticMetaObject. )
AMDbObjectInfo::AMDbObjectInfo(const QMetaObject* classMetaObject) {
	initWithMetaObject(classMetaObject);
}

void AMDbObjectInfo::initWithMetaObject(const QMetaObject *classMetaObject) {
	metaObject = classMetaObject;
	className = metaObject->className();
	tableName = AMDbObjectSupport::s()->tableNameForClass(metaObject);

	doNotReuseIds = ( AMDbObjectSupport::dbObjectAttribute(metaObject, "doNotReuseIds") == QString("true") );
	classDescription = AMDbObjectSupport::dbObjectAttribute(metaObject, "description");
	sharedTable = !AMDbObjectSupport::dbObjectAttribute(metaObject, "shareTableWithClass").isEmpty();

	bool versionSet;
	int versionInt = AMDbObjectSupport::dbObjectAttribute(metaObject, "version").toInt(&versionSet);
	version = versionSet ? versionInt : 1;

	int numProperties = metaObject->propertyCount();
	columnCount = 0;

	for(int i=0; i<numProperties; i++) {
		QString propertyName = metaObject->property(i).name();

		// if we can't read this property, or if the doNotStore attribute has been set to 'true', do not store it. Ignore it completely from the AMDbObject system.
		bool doNotStore = !metaObject->property(i).isReadable() ||
				(AMDbObjectSupport::dbPropertyAttribute(metaObject, propertyName, "doNotStore") == QString("true"));
		if(doNotStore)
			continue;

		bool hidden = (AMDbObjectSupport::dbPropertyAttribute(metaObject, propertyName, "hidden") == QString("true"));
		bool doNotLoad =  !metaObject->property(i).isWritable()
				|| (AMDbObjectSupport::dbPropertyAttribute(metaObject, propertyName, "doNotLoad") == QString("true"));
		bool createIndex = (AMDbObjectSupport::dbPropertyAttribute(metaObject, propertyName, "createIndex") == QString("true"));

		columnCount++;
		columns << propertyName;
		columnTypes << metaObject->property(i).userType();	// need userType() instead of type(), because the type() for all user-defined types would be simply QVariant::UserType.  Note that for normal types, userType() == type().
		isVisible << !hidden;
		isLoadable << !doNotLoad;
		isIndexed << createIndex;
	}
}


AMDbObjectSupport* AMDbObjectSupport::instance_;
QMutex AMDbObjectSupport::instanceMutex_(QMutex::Recursive);



// Retrieve the AMDbObjectAttribute for a given \c object and \c key
QString AMDbObjectSupport::dbObjectAttribute(const QMetaObject* mo, const QString& key) {
	int i = mo->indexOfClassInfo("AMDbObject_Attributes");
	if(i < 0) {
		//qDebug() << "AMDBOBJECT" << mo->className() << ": no dbobject attributes set";
		return QString();
	}
	QString allAttributes( mo->classInfo(i).value() );
	QStringList attributeList = allAttributes.split(QChar(';'), QString::SkipEmptyParts);// split attributes by semicolon
	// search for a matching attribute:
	for(int i=0; i<attributeList.count(); i++) {
		if(attributeList.at(i).trimmed().startsWith(key+"="))
			return attributeList.at(i).section(QChar('='), 1);// return section after "key=".
	}
	//qDebug() << "AMDBOBJECT" << mo->className() << ": could not find object attribute " << key;
	return QString();
}

// Retrieve an object's property attribute for a given \c object, \c propertyName, and \c key. Returns empty string if not set.
QString AMDbObjectSupport::dbPropertyAttribute(const QMetaObject* mo, const QString& propertyName, const QString& key) {
	int i = mo->indexOfClassInfo(propertyName.toAscii().constData());
	if(i < 0) {
		//qDebug() << "AMDBOBJECT" << mo->className() << ": no property attributes set for " << propertyName;
		return QString();	// property attributes not found for this property
	}
	QString allAttributes( mo->classInfo(i).value() );
	QStringList attributeList = allAttributes.split(QChar(';'), QString::SkipEmptyParts);// split attributes by semicolon
	// search for matching attribute:
	for(int i=0; i<attributeList.count(); i++) {
		if(attributeList.at(i).startsWith(key+"="))
			return attributeList.at(i).section(QChar('='), 1);// return section after "key=".
	}
	//qDebug() << "AMDBOBJECT" << mo->className() << ": could not find property attribute " << propertyName << key;

	return QString();
}


// Returns the name of the table that should be used to store this class. If the class is registered already, it returns the registered name. If it's not registered, but it has the dbObjectProperty 'shareTableWithClass' set to another registered class, then it returns the table used by that class. Otherwise, it provides a default table name of 'className' + '_table'.
QString AMDbObjectSupport::tableNameForClass(const QMetaObject* classMetaObject) const {

	QString className = classMetaObject->className();

	// Threading note: this might be called from situations where we have a read lock, or a write lock. This should work for both (previously holding a read lock or a write lock, because the locks are recursive)
	/// \todo This could be simplified a lot of we didn't have shared tables in the database system. Once AMScan is merged, maybe we should go to that.
	if(!registryMutex_.tryLockForRead())
		registryMutex_.lockForWrite();

	// if we're registered already, let's just look this up in the registry.
	QHash<QString, AMDbObjectInfo>::const_iterator iInfo = registeredClasses()->find(className);
	if(iInfo != registeredClasses()->end()) {
		registryMutex_.unlock();
		return iInfo.value().tableName;
	}

	// ok, we're not registered. Instead, let's answer where we SHOULD be stored:

	// are supposed to use the table of another registered class?
	QString sharedClassName = dbObjectAttribute(classMetaObject, "shareTableWithClass");
	if( !sharedClassName.isEmpty() ) {
		iInfo = registeredClasses()->find(sharedClassName);
		if(iInfo != registeredClasses()->end()) {
			registryMutex_.unlock();
			return iInfo.value().tableName;
		}
	}
	// default is to use the class name (which is always unique) plus "_table":
	registryMutex_.unlock();
	return className + "_table";
}



QString AMDbObjectSupport::tableNameForClass(const QString& className) const {
	// this should work if our thread is previously holding a read lock or a write lock (because the locks are recursive)
	if(!registryMutex_.tryLockForRead())
		registryMutex_.lockForWrite();

	QHash<QString, AMDbObjectInfo>::const_iterator iInfo = registeredClasses()->find(className);
	if(iInfo != registeredClasses()->end()) {
		registryMutex_.unlock();
		return iInfo.value().tableName;
	}
	registryMutex_.unlock();
	return QString();
}

// register a new class with the database system. This is all you need to do enable an AMDbObect subclass. Returns false if the initialization failed; true if it was completed successfully, or if the object is already registered.
bool AMDbObjectSupport::registerClass(const QMetaObject* mo) {

	QWriteLocker wl(&registryMutex_);

	// is this a subclass of AMDbObject? (Or an AMDbObject itself?)
	if(!inheritsAMDbObject(mo))
		return false;	// can't register a non AMDbObject subclass.

	// is it already registered? return true.
	QString className(mo->className());
	if(registeredClasses_.contains(className)) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, 0, QString("Database Support: The class '%1' has already been registered in the database. Skipping duplicate registration.").arg(className)));
		return true;
	}

	AMDbObjectInfo newInfo(mo);

	bool success = true;
	QSetIterator<AMDatabase*> iDatabases(registeredDatabases_);
	while(iDatabases.hasNext()) {
		success = success && getDatabaseReadyForClass(iDatabases.next(), newInfo);
	}

	if(success) {
		registeredClasses_.insert(className, newInfo);
		registeredClassesInOrder_ << newInfo;
		return true;
	}
	else
		return false;
}


bool AMDbObjectSupport::registerDatabase(AMDatabase* db) {

	QWriteLocker wl(&registryMutex_);

	// is it already registered? return true.
	if(registeredDatabases_.contains(db)) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, 0, QString("Database Support: The database '%1' has already been registered in the system. Skipping duplicate registration.").arg(db->connectionName())));
		return true;
	}


	// ensure type-metaInfo table
	if( db->ensureTable(	typeTableName(),
							QString("AMDbObjectType,tableName,description,version").split(','),
							QString("TEXT,TEXT,TEXT,INTEGER").split(','),
							false) )
		db->createIndex(typeTableName(), "AMDbObjectType");

	// ensure supporting type tables: (These map types to column names: an entry for each type / field-name combintion)
	if( db->ensureTable(	allColumnsTableName(),
							QString("typeId,columnName").split(','),
							QString("INTEGER,TEXT").split(','),
							false) )
		db->createIndex(allColumnsTableName(), "typeId");

	if( db->ensureTable(	visibleColumnsTableName(),
							QString("typeId,columnName").split(','),
							QString("INTEGER,TEXT").split(','),
							false) )
		db->createIndex(visibleColumnsTableName(), "typeId");

	if( db->ensureTable(loadColumnsTableName(),
						QString("typeId,columnName").split(','),
						QString("INTEGER,TEXT").split(','),
						false) )
		db->createIndex(loadColumnsTableName(), "typeId");

	// This table stores thumbnails for all these object types.  It should not reuse ids, so that a set of thumbnails added will always have sequential ids.
	db->ensureTable(thumbnailTableName(), QString("objectId,objectTableName,number,type,title,subtitle,thumbnail").split(','), QString("INTEGER,TEXT,INTEGER,TEXT,TEXT,TEXT,BLOB").split(','), false);
	db->createIndex(thumbnailTableName(), "objectId,objectTableName");

	// temporary... this should all be cleaned up and moved and handled generically
	////////////////////////////
	db->ensureTable(elementTableName(), QString("AMDbObjectType,thumbnailCount,thumbnailFirstId,symbol,name,atomicNumber").split(','), QString("TEXT,INTEGER,INTEGER,TEXT,TEXT,INTEGER").split(','));


	// These tables provide links between experiments and user-data objects, and samples and elements.
	db->ensureTable(experimentEntriesTableName(), QString("objectId,experimentId").split(','), QString("INTEGER,INTEGER").split(','));
	db->createIndex(experimentEntriesTableName(), "objectId,experimentId");
	db->createIndex(experimentEntriesTableName(), "experimentId,objectId");

	db->ensureTable(sampleElementEntriesTableName(), QString("sampleId,elementId").split(','), QString("INTEGER,INTEGER").split(','));
	db->createIndex(sampleElementEntriesTableName(), "sampleId,elementId");
	db->createIndex(sampleElementEntriesTableName(), "elementId,sampleId");

	// This table provides optimized storage for the individual control fields in a control set
	db->ensureTable(controlSetEntriesTableName(), QString("csiId,name,ctrlValue,minimum,maximum,units,number").split(','), QString("INTEGER,TEXT,REAL,REAL,REAL,TEXT,INTEGER").split(','));
	db->createIndex(controlSetEntriesTableName(), "csiId");
	/////////////////////////////////

	/// \todo error checking on creating these previous tables.


	// Retro-actively add all previously registered classes.
	bool success = true;
	foreach(const AMDbObjectInfo& dbo, registeredClassesInOrder_) {
		success = success && getDatabaseReadyForClass(db, dbo);
	}

	if(success) {
		registeredDatabases_.insert(db);
		connect(db, SIGNAL(destroyed()), this, SLOT(onRegisteredDatabaseDeleted()));
		return true;
	}
	else
		return false;
}

bool AMDbObjectSupport::getDatabaseReadyForClass(AMDatabase* db, const AMDbObjectInfo& info) {

	// have the tables already been created in this database for this class? check the types table:
	QList<int> foundRows = db->objectsMatching(typeTableName(), "AMDbObjectType", info.className );

	// Possibility 1: This type has never been stored in this database... need to create new table for it, create columns and indexes in that table, add row for this class in the type information table, and add column entries in the allColumns, visibleColumns, and loadColumns tables.
	/////////////////////////
	if(foundRows.count() == 0) {
		return initializeDatabaseForClass(db, info);
	}

	// Possibility 2: this class has already been installed in this database.
	////////////////////////////////////////////
	if( foundRows.count() == 1) {
		int id = foundRows.at(0);

		if(isUpgradeRequiredForClass(db, info, id)) {
			return upgradeDatabaseForClass(db, info, id);
		}
		else {
			return true;
		}
	}

	/* REMOVED this version system
   QStringList typeInfoCols;
   typeInfoCols << "AMDbObjectType" << "tableName" << "version";
   QVariantList typeInfoValues = db->retrieve(id, typeTableName(), typeInfoCols);

   // check for matching version. If not matching, need to upgrade
   int storedVersion = typeInfoValues.at(2).toInt();
   if( storedVersion != info.version ) {
 return upgradeDatabaseForClass(db, info, storedVersion);
   }

   // check for matching table name. If not matching, the database has been corrupted somewhere.
   QString storedTableName = typeInfoValues.at(1).toString();
   if(storedTableName != info.tableName) {
 AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -6, QString("Database support: Your database stores '%1' objects in a table called '%2', but they should be stored under '%3'. This database is not compatible with the current version of Acquaman.").arg(info.className, storedTableName, info.tableName)));
 return false;
   }
 // otherwise, assuming future versions of this function adhere to creating the types table with a version #, and create the allColumns, loadColumns, and visibleColumns tables... we should be good here. Could check for matching columns, but that should be taken care of with the version system. Could check that the entries are created in the allColumns, visibleColumns, and loadColumns tables, but that would only fail if we have a bug in this function.
   */


	// Possibility 3: more than one entry found for this class, in the type info table. This should never happen.
	else {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -6, QString("Database support: Multiple versions of the '%1' object were found in your database. This likely means that your database has been corrupted.").arg(info.className)));
		return false;
	}
}



bool AMDbObjectSupport::initializeDatabaseForClass(AMDatabase* db, const AMDbObjectInfo& info) {
	db->startTransaction();	// exiting this function for any error should roll back to here.

	// create table...
	// \bug attempts to re-create table when sharing tables with other already-registered objects...
	if ( !info.sharedTable && !ensureTableForDbObjects(info.tableName, db, !info.doNotReuseIds) ) {
		db->rollbackTransaction();
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1, QString("Database support: There was an error trying to create a table in the database for class %1.").arg(info.className)));
		return false;
	}
	// go through properties and create columns for each, (with some exceptions...)
	for(int i=0; i<info.columns.count(); i++) {

		// if type of property is AMDbObjectList (ie: it 'owns' a set of other AMDbObjects), then don't create a column at all. Instead, create an auxilliary table.  Table name is our table name + "_propertyName".
		if( info.columnTypes.at(i) == qMetaTypeId<AMDbObjectList>() ) {
			QString auxTableName = info.tableName % "_" % info.columns.at(i);
			if( !db->ensureTable(auxTableName,
								 QString("id1,table1,id2,table2").split(','),
								 QString("INTEGER,TEXT,INTEGER,TEXT").split(','))
					|| !db->createIndex(auxTableName, "id1")
					|| !db->createIndex(auxTableName, "id2") ) {

				// must allow failures on shared tables... the columns might have already been created for previous classes.
				/// \todo For more reliability, could ensure that _unique columns_ in shared-table classes have actually been created.
				if(!info.sharedTable) {
					db->rollbackTransaction();
					AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -16, QString("Database support: There was an error trying to create (initialize) an auxiliary table (%1) in the database for class %2.").arg(auxTableName).arg(info.className)));
					return false;
				}
			}
		}

		// otherwise, handle as usual: create one column with the name and type of the property.
		else {
			if( !db->ensureColumn(info.tableName,
								  info.columns.at(i),
								  db->metaType2DbType((QVariant::Type)info.columnTypes.at(i))) ) {
				// must allow failures on shared tables... the columns might have already been created for previous classes.
				/// \todo For more reliability, could ensure that _unique columns_ in shared-table classes have actually been created.
				if(!info.sharedTable) {
					db->rollbackTransaction();
					AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -2, QString("Database support: There was an error trying to create a column (%1) in the database for class %2.").arg(info.columns.at(i)).arg(info.className)));
					return false;
				}
			}

			// create index on column, if required.
			if(info.isIndexed.at(i)) {
				if( !db->createIndex(info.tableName, info.columns.at(i)) ) {
					if(!info.sharedTable) {
						AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -3, QString("Database support: There was an error trying to create an index (%1) in the database for class %2.").arg(info.columns.at(i)).arg(info.className)));
						db->rollbackTransaction();
						return false;
					}
				}
			}
		}
	}	// end of loop over properties

	// add to type table:
	QStringList typeTableCols;
	typeTableCols << "AMDbObjectType" << "tableName" << "description" << "version";
	QVariantList typeTableValues;
	typeTableValues << info.className << info.tableName << info.classDescription << info.version;
	int typeId = db->insertOrUpdate(0, typeTableName(), typeTableCols, typeTableValues);

	if(typeId < 1) {
		db->rollbackTransaction();
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -4, QString("Database support: There was an error trying to register the class '%1' in the database").arg(info.className)));
		return false;
	}

	// add entry for each column in allColumns, and (potentially) visibleColumns, loadColumns
	QStringList clist;
	clist << "typeId" << "columnName";
	QVariantList vlist;
	vlist << typeId << "colName";
	for(int i=0; i<info.columns.count(); i++) {	// loop over columns

		// lists of AMDbObjects use aux. tables, not columns. So no column entry should appear for these.
		if(info.columnTypes.at(i) == qMetaTypeId<AMDbObjectList>())
			continue;

		vlist[1] = info.columns.at(i);// takes on the name of this column

		bool success = db->insertOrUpdate(0, allColumnsTableName(), clist, vlist); // always add to the 'allColumns' table.

		if(info.isVisible.at(i) && info.columnTypes.at(i) != qMetaTypeId<AMDbObject*>())	// no matter what, AMDbObject* reference columns aren't user-visible. There's nothing user-meaningful about a 'tableName;id' string.
			success = success && db->insertOrUpdate(0, visibleColumnsTableName(), clist, vlist);

		if(info.isLoadable.at(i)) // if loadable, add to 'loadColumns' table.
			success = success && db->insertOrUpdate(0, loadColumnsTableName(), clist, vlist);

		if(!success) {
			db->rollbackTransaction();
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -5, QString("Database support: There was an error trying to register the class '%1' in the database").arg(info.className)));
			return false;
		}
	}

	db->commitTransaction();
	return true;
}

// Helper function: checks if a class can be stored in the database as-is, or if the DB needs to be upgraded.
/* Confirms that all columns and auxiliary tables exist, although not necessarily in the order specified. (Since upgrading a base class will tack on the new base class members at the end of the original subclass members.) */
bool AMDbObjectSupport::isUpgradeRequiredForClass(AMDatabase* db, const AMDbObjectInfo& info, int typeIdInDatabase) {

	// what columns does the database have?  This is kept up-to-date in the table allColumnsTableName().
	QSet<QString> existingColumns;

	QSqlQuery q = db->query();
	q.prepare("SELECT columnName FROM " % allColumnsTableName() % " WHERE typeId = '" % QString::number(typeIdInDatabase) % "';");
	if(!AMDatabase::execQuery(q)) {
		q.finish();
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -205, QString("Database support: There was an error while trying to check if the class '%1' in the database needs an upgrade.").arg(info.className)));
		return false;
	}
	while(q.next()) {
		existingColumns << q.value(0).toString();
	}
	// existingColumns now has a list of all the columns that are currently in the DB for this type.



	// Determine which columns we need to have:
	for(int i=0; i<info.columns.count(); i++) {

		// if type of property is AMDbObjectList (ie: it 'owns' a set of other AMDbObjects), then there shouldn't be a column. Instead, should be an auxilliary table.  Table name is our table name + "_propertyName".
		if( info.columnTypes.at(i) == qMetaTypeId<AMDbObjectList>() ) {
			QString auxTableName = info.tableName % "_" % info.columns.at(i);
			// Does SQLite not support the SQL-92 standard INFORMATION_SCHEMA?
			//				q.prepare("SELECT COUNT(1) FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = " % auxTableName % ";");
			//				AMDatabase::execQuery(q);
			//				if(!q.first() || q.value(0).toInt() != 1) {
			//					return true;
			//				}
			// Ok, let's try this way.  This will simply fail if the auxiliary table doesn't exist...
			q.prepare("SELECT COUNT(1) FROM " % auxTableName % " WHERE 1=0;");	// as high-performance of a query as we can make on that table;
			if(!AMDatabase::execQuery(q)) {
				return true;
			}
		}
		// normal tables / columns.
		else {
			if(!existingColumns.contains(info.columns.at(i))) {
				return true;
			}
		}
	}

	return false;
}

// upgrade an existing database from supporting an old version of a class to supporting a new version.
bool AMDbObjectSupport::upgradeDatabaseForClass(AMDatabase* db, const AMDbObjectInfo& info, int typeIdInDatabase) {


	// 1) what columns does the database have?  This is kept up-to-date in the table allColumnsTableName().
	QSet<QString> existingColumns;
	QSqlQuery q = db->query();
	q.prepare("SELECT columnName FROM " % allColumnsTableName() % " WHERE typeId = '" % QString::number(typeIdInDatabase) % "';");
	if(!AMDatabase::execQuery(q)) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -105, QString("Database support: There was an error while trying to check if the class '%1' in the database needs an upgrade.").arg(info.className)));
		return false;
	}
	while(q.next()) {
		existingColumns << q.value(0).toString();
	}
	// existingColumns now has a list of all the columns that are currently in the DB for this type.


	// 2) Just set this up for later (used when adding entry for each column in allColumns, and (potentially) visibleColumns, loadColumns)
	QStringList clist;
	clist << "typeId" << "columnName";
	QVariantList vlist;
	vlist << typeIdInDatabase << "colName";


	db->startTransaction();	// exiting this function for any error should roll back to here.

	AMErrorMon::report(AMErrorReport(0, AMErrorReport::Information, 300, QString("AMDbObjectSupport: Database Upgrade starting for class '%1'.").arg(info.className)));

	// 3) For all the columns we have:
	for(int i=0; i<info.columns.count(); i++) {

		// 3a) if type of property is AMDbObjectList (ie: it 'owns' a set of other AMDbObjects), then there shouldn't be a column. Instead, should be an auxilliary table.  Table name is our table name + "_propertyName".
		////////////////////////////////////////////
		if( info.columnTypes.at(i) == qMetaTypeId<AMDbObjectList>() ) {
			QString auxTableName = info.tableName % "_" % info.columns.at(i);
			// Does the table exist?
			q.prepare("SELECT COUNT(1) FROM " % auxTableName % " WHERE 1=0;");	// as high-performance of a query as we can make on that table;
			if(!AMDatabase::execQuery(q)) {	// fails if table doesn't exist.
				q.finish();
				// therefore, we need to create the table:
				if( !db->ensureTable(auxTableName,
									 QString("id1,table1,id2,table2").split(','),
									 QString("INTEGER,TEXT,INTEGER,TEXT").split(','))
						|| !db->createIndex(auxTableName, "id1")
						|| !db->createIndex(auxTableName, "id2") ) {

					db->rollbackTransaction();
					AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -106, QString("Database support: There was an error trying to create (upgrade) an auxiliary table (%1) in the database while upgrading class %2.").arg(auxTableName).arg(info.className)));
					return false;
				}
			}
		}

		// 3b) normal tables / columns.
		////////////////////////////////
		else {
			if(!existingColumns.contains(info.columns.at(i))) {
				// 3b-1) need to create the column:
				if( !db->ensureColumn(info.tableName,
									  info.columns.at(i),
									  db->metaType2DbType((QVariant::Type)info.columnTypes.at(i))) ) {
					// must allow failures on shared tables... the columns might have already been created for previous classes.
					/// \todo For more reliability, could ensure that _unique columns_ in shared-table classes have actually been created.
					if(!info.sharedTable) {
						db->rollbackTransaction();
						AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -102, QString("Database support: There was an error trying to create a column (%1) in the database while upgrading class %2.").arg(info.columns.at(i)).arg(info.className)));
						return false;
					}
				}

				// 3b-2) create index on column, if required.
				if(info.isIndexed.at(i)) {
					if( !db->createIndex(info.tableName, info.columns.at(i)) ) {
						if(!info.sharedTable) {
							db->rollbackTransaction();
							AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -103, QString("Database support: There was an error trying to create an index (%1) in the database while upgrading class %2.").arg(info.columns.at(i)).arg(info.className)));
							return false;
						}
					}
				}

				// 3b-3) Fill existing rows with default value, if an upgrade default is provided.
				QString defaultValue = dbPropertyAttribute(info.metaObject, info.columns.at(i), "upgradeDefault");
				if(!defaultValue.isEmpty()) {
					QSqlQuery q = db->query();
					q.prepare(QString("UPDATE %1 SET %2 = ?;").arg(info.tableName).arg(info.columns.at(i)));
					q.bindValue(0, QVariant(defaultValue));

					if(!AMDatabase::execQuery(q)) {
						q.finish();
						AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -403, QString("AMDbObjectSupport: Could not insert default value '%1' for column '%2'").arg(defaultValue).arg(info.columns.at(i))));
					}
				}

				// 3b-3) add to the all-columns, visible-columns, and load-columns tables
				vlist[1] = info.columns.at(i);// takes on the name of this column

				bool success = db->insertOrUpdate(0, allColumnsTableName(), clist, vlist); // always add to the 'allColumns' table.

				if(info.isVisible.at(i) && info.columnTypes.at(i) != qMetaTypeId<AMDbObject*>())	// no matter what, AMDbObject* reference columns aren't user-visible. There's nothing user-meaningful about a 'tableName;id' string.
					success = success && db->insertOrUpdate(0, visibleColumnsTableName(), clist, vlist);

				if(info.isLoadable.at(i)) // if loadable, add to 'loadColumns' table.
					success = success && db->insertOrUpdate(0, loadColumnsTableName(), clist, vlist);

				if(!success) {
					db->rollbackTransaction();
					AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -105, QString("Database support: There was an error trying to update the schema tables while upgrading the class '%1' in the database").arg(info.className)));
					return false;
				}
			}
		}
	}


	db->commitTransaction();
	AMErrorMon::report(AMErrorReport(0, AMErrorReport::Information, 301, QString("AMDbObjectSupport: Database Upgrade completed for class '%1'.").arg(info.className)));
	return true;
}


// Support table names: (AMDbObject___ prefix ensures they don't collide with potential class table names)
QString AMDbObjectSupport::thumbnailTableName() { return "AMDbObjectThumbnails_table"; }
// name of the table that stores meta-type information for all the registered classes.
QString AMDbObjectSupport::typeTableName() { return "AMDbObjectTypes_table"; }
QString AMDbObjectSupport::allColumnsTableName() { return "AMDbObjectTypes_allColumns"; }
QString AMDbObjectSupport::visibleColumnsTableName() { return "AMDbObjectTypes_visibleColumns"; }
QString AMDbObjectSupport::loadColumnsTableName() { return "AMDbObjectTypes_loadColumns"; }


bool AMDbObjectSupport::ensureTableForDbObjects(const QString& tableName, AMDatabase* db, bool reuseDeletedIds) {

	return db->ensureTable(tableName, QString("AMDbObjectType,thumbnailCount,thumbnailFirstId").split(','), QString("TEXT,INTEGER,INTEGER").split(','), reuseDeletedIds);
}



// Separator used between strings when exporting a StringList to the database
QString AMDbObjectSupport::stringListSeparator() { return "|@^@|"; }
// Separator used between items when exporting all other lists to the database (changed from comma to support french localizations which use une virgule for the decimal point. maybe this needs to be fully localized.)
QString AMDbObjectSupport::listSeparator() { return ";"; }


// Temporary tables (to be generalized)
///////////////////////
QString AMDbObjectSupport::elementTableName() { return "Elements"; }

QString AMDbObjectSupport::experimentEntriesTableName() { return "ObjectExperimentEntries"; }
QString AMDbObjectSupport::sampleElementEntriesTableName() { return "SampleElementEntries"; }
QString AMDbObjectSupport::controlSetEntriesTableName() { return "ControlSetEntries"; }
///////////////////////////



// Useful for database introspection, this returns the type() (ie: class name) of the object stored in database \c db, under table \c tableName, at row \c id.
QString AMDbObjectSupport::typeOfObjectAt(AMDatabase* db, const QString& tableName, int id) {

	if(!db || id<1)
		return QString();

	QVariant typeString = db->retrieve(id, tableName, "AMDbObjectType");
	if(typeString.isValid())
		return typeString.toString();
	else
		return QString();
}

// returns a pointer to the object info for a given class with \c className, or 0 if the class has not been registered in the database system.
const AMDbObjectInfo* AMDbObjectSupport::objectInfoForClass(const QString& className) const {
	QReadLocker rl(&registryMutex_);

	QHash<QString, AMDbObjectInfo>::const_iterator iInfo = registeredClasses()->find(className);
	if(iInfo != registeredClasses()->end()) {
		return &(iInfo.value());
	}
	return 0;
}

// Useful for database introspection, this creates and dynamically loads an object stored in database \c db, under table \c tableName, at row \c id. You can use qobject_cast<>() or type() to find out the detailed type of the new object.  Returns 0 if no object found.
/* Ownership of the newly-created object becomes the responsibility of the caller. */
AMDbObject* AMDbObjectSupport::createAndLoadObjectAt(AMDatabase* db, const QString& tableName, int id) const {

	QString className = typeOfObjectAt(db, tableName, id);
	if(className.isEmpty()) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -90, QString("[AMDbObjectSupport] Could not load the object with ID %1 from the table '%2', because we couldn't determine the type of the object.").arg(id).arg(tableName)));
		return 0;
	}

	const AMDbObjectInfo* objInfo = objectInfoForClass(className);

	if(!objInfo) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -91, QString("[AMDbObjectSupport] Could not load the object with ID %1 from the table '%2', because the class '%3' hasn't yet been registered in the database system.").arg(id).arg(tableName).arg(className)));
		return 0;
	}

	// this should never happen, so we won't bother to check. But if an object was somehow stored in the wrong table, AMDbObject::loadFromDb() will fail.
	// if(tableName != objInfo->tableName)
	// return 0;

	const QMetaObject* mo = objInfo->metaObject;

	// first try a constructor that takes a database and an id. We assume this one calls loadFromDb() automatically.
	AMDbObject* newObject;

	newObject = qobject_cast<AMDbObject*>( mo->newInstance(	Q_ARG(AMDatabase*, db),
															Q_ARG(int, id)) );
	if(newObject)
		return newObject;

	// now try a default constructor taking no arguments:
	newObject = qobject_cast<AMDbObject*>( mo->newInstance() );
	if(newObject) {
		if(newObject->loadFromDb(db, id))
			return newObject;
		else {
			delete newObject;	// loading failed, and we're not going to return anything. Make sure not to leak the newly-created object.
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -93, QString("[AMDbObjectSupport] Could not load the object with ID %1 from the table '%2': loadFromDb() failed. Please report this bug to the Acquaman developers").arg(id).arg(tableName)));
		}
	}
	else {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Debug, -93, QString("[AMDbObjectSupport] Could not load the object with ID %1 from the table '%2', because there are no suitable constructors. Please report this bug to the Acquaman developers").arg(id).arg(tableName)));
	}
	return 0;
}


// Helper function to check if a class inherits AMDbObject.  \c mo is the Qt QMetaObject representing the class.
bool AMDbObjectSupport::inheritsAMDbObject(const QMetaObject* mo) {
	const QMetaObject* superClass = mo;
	bool inheritsDbObject;
	do {
		inheritsDbObject = (superClass->className() == QString("AMDbObject"));
	}
	while( (superClass=superClass->superClass()) && inheritsDbObject == false );

	return inheritsDbObject;
}


QSqlQuery AMDbObjectSupport::select(AMDatabase* db, const QString& className, const QString& columnNames, const QString& whereClause) const {
	QReadLocker rl(&registryMutex_);

	QString tableName = tableNameForClass(className);
	rl.unlock();

	return db->select(tableName, columnNames, whereClause);
}

void AMDbObjectSupport::onRegisteredDatabaseDeleted()
{
	QReadLocker rl(&registryMutex_);

	AMDatabase* db = static_cast<AMDatabase*>(sender());
	if(registeredDatabases_.contains(db)) {
		rl.unlock();
		registryMutex_.lockForWrite();
		registeredDatabases_.remove(db);
		registryMutex_.unlock();
	}
}

AMDbObjectSupport * AMDbObjectSupport::s() {
	QMutexLocker ml(&instanceMutex_);

	if(!instance_) {
		instance_ = new AMDbObjectSupport();
		// ensure we're in the main thread...
		if(QThread::currentThread() != QApplication::instance()->thread()) {
			instance_->moveToThread(QApplication::instance()->thread());
		}
	}
	return instance_;
}

#include <QDebug>
bool AMDbObjectSupport::event(QEvent *e)
{
	if(e->type() != (QEvent::Type)AM::ThumbnailsGeneratedEvent)
		return false;

	AMDbThumbnailsGeneratedEvent* te = static_cast<AMDbThumbnailsGeneratedEvent*>(e);

	AMDatabase* db = te->db;
	QString dbTableName = te->dbTablename;
	int id = te->dbObjectId;
	bool neverSavedHereBefore = te->neverSavedHereBefore;
	int thumbsCount = te->thumbnails.count();

	// check that the database hasn't been deleted and is still registered properly.
	registryMutex_.lockForRead();
	if(!registeredDatabases_.contains(db)) {
		registryMutex_.unlock();
		AMErrorMon::report(AMErrorReport(this, AMErrorReport::Debug, -967, "Received a thumbnail update event for an unregistered database. It's possible that the database was deleted since this event was posted."));
		return true;	// still want the event, but can't do anything.
	}
	registryMutex_.unlock();

	// for debugging database performance only:
	QTime saveTime;
	saveTime.start();

	// The remainder of this should happen in one database transaction. This ensures consistency, and it also increases performance because a database commit (and time consuming flush-to-disk, in the case of SQLite) doesn't have to happen for each thumbnail insert -- just once at the end.
	// Note that there might be a transaction started already...
	bool openedTransaction = false;
	if(db->supportsTransactions() && !db->transactionInProgress()) {
		if(db->startTransaction()) {
			openedTransaction = true;
			qDebug() << "Opened transaction for thumbnail save of object at [" << dbTableName << id << "].";
		}
		else {
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -495, "Could not start a transaction to save the thumbnails for object '" % dbTableName % ":" % QString::number(id) % "' in the database. Please report this problem to the Acquaman developers."));
			return true;
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
		const AMDbThumbnail& t = te->thumbnails.at(i);

		keys.clear();
		values.clear();

		keys << "objectId";
		values << id;
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
				return true;
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
			return true;
		}
	}

	// only commit the transaction if we opened it.
	if(openedTransaction && !db->commitTransaction()) {
		db->rollbackTransaction();
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -495, "AMDbObject: Could not commit a transaction to save the thumbnails for object '" % dbTableName % ":" % QString::number(id) % "' in the database. Please report this problem to the Acquaman developers."));
	}

	qDebug() << "Storing thumbnails for [" << dbTableName << ":" << id << "] took" << saveTime.elapsed() << "ms to store thumbnails in the database. Used own transaction = " << openedTransaction;
	return true;
}



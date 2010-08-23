#ifndef ACQMAN_DBLOADER_H
#define ACQMAN_DBLOADER_H

#include "dataman/AMDbObject.h"

/// This class provides a way of loading an arbitrary object from the database, doing type inspection on it, and fully loading the extended object.
/*! It must know about all the types it can load, so this file must be updated when adding new scan types...
 If you know of a good way to structure to avoid this, let me know!
 */
class AMDbLoader {

public:
	/// Create a database object in memory and retrieve its parameters from the database.
	/// If no object exists at that id, a null pointer is returned.
	/// Memory management: the new object will become a child of 'parent'.
	/// If you pass in a pointer to a QString 'type', it will be filled with the type of the object that was created.
	static AMDbObject* createAndLoad(AMDatabase* db, int id, QObject* parent = 0, QString* type = 0);
};

#endif // DBLOADER_H

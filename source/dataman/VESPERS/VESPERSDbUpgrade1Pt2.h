#ifndef VESPERSDBUPGRADE1PT2_H
#define VESPERSDBUPGRADE1PT2_H

#include "dataman/database/AMDbUpgrade.h"

#define VESPERSDBUPGRADE1PT2_COULD_NOT_UPDATE_2DCONFIGURATION 389100
#define VESPERSDBUPGRADE1PT2_COULD_NOT_UPDATE_LINECONFIGURATION 389101
#define VESPERSDBUPGRADE1PT2_COULD_NOT_UPDATE_EXAFSCONFIGURATION 389102

/// This class is a very simple database upgrade where the column name for fluorescenceDetectorChoice is renamed to fluorescenceDetector.
/*!
	Although the actual change is quite minor, the process is somewhat involved because SQLite doesn't have the entire SQL language
	options.  The other reason for this upgrade is for practice changing column names since the next database upgrade
	changes the contents of the columns as well as the name of the column.
  */
class VESPERSDbUpgrade1Pt2 : public AMDbUpgrade
{
	Q_OBJECT

public:
	/// Constructor.  Need to provide the name of the database to upgrade.
	VESPERSDbUpgrade1Pt2(const QString &databaseNameToUpgrade, QObject *parent = 0);

	/// Indicates the dependcies of this upgrade.  The dependency is VESPERSUpgrade1.1 because it uses the name "fluorescenceDetectorChoice" explicitly.
	virtual QStringList upgradeFromTags() const;

	/// Returns true if the database contains any 2D scan configurations, line scan configurations, or EXAFS scan configurations.
	virtual bool upgradeNecessary() const;

	/// Renames the current table, creates a new one with the old name with the modified column name.  Then all of the data is transferred to the new table.  Lastly, the old table is deleted.
	virtual bool upgradeImplementation();

	/// Creates a new copy of this upgrade (caller is responsidble for memory).
	virtual AMDbUpgrade *createCopy() const;

	/// Upgrade tag for this upgrade is "VESPERSDbUpgrade1.2".
	virtual QString upgradeToTag() const;

	/// Returns the description of hte upgrade.
	virtual QString description() const;
};

#endif // VESPERSDBUPGRADE1PT2_H

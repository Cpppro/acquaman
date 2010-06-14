#include "AMFirstTimeController.h"
#include <QStringList>

AMFirstTimeController::AMFirstTimeController() {

	bool isFirstTime = false;

	// check for missing user settings:
	QSettings s(QSettings::IniFormat, QSettings::UserScope, "Acquaman", "Acquaman");
	if(!s.contains("userDataFolder")) {
		isFirstTime = true;
	}

	else {

		// check for existence of user data folder:
		QDir userDataDir(AMUserSettings::userDataFolder);
		if(!userDataDir.exists()) {
			isFirstTime = true;
		}

		// check for existence of database:
		QString filename = AMUserSettings::userDataFolder + AMUserSettings::userDatabaseFilename;
		QFile dbFile(filename);
		if(!dbFile.exists()) {
			isFirstTime = true;
		}
	}


	if(isFirstTime)
		onFirstTime();

	databaseUpgrade();
}

#include <QApplication>

void AMFirstTimeController::onFirstTime() {

	AMFirstTimeWizard ftw;
	if(ftw.exec() != QDialog::Accepted) {
		// figure out how to quit the main program from here.  We might not be inside the application run loop yet.
	}

	AMUserSettings::userName = ftw.field("userName").toString();
	AMUserSettings::userDataFolder = ftw.field("userDataFolder").toString();
	if(!AMUserSettings::userDataFolder.endsWith('/'))
		AMUserSettings::userDataFolder.append("/");

	AMUserSettings::save();

	// Attempt to create user's data folder, only if it doesn't exist:
	QDir userDataDir(AMUserSettings::userDataFolder);
	if(!userDataDir.exists()) {
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, 0, "Creating new user data folder: "  + AMUserSettings::userDataFolder));
		if(!userDataDir.mkpath(AMUserSettings::userDataFolder))
			AMErrorMon::report(AMErrorReport(0, AMErrorReport::Serious, 0, "Could not create user data folder " + AMUserSettings::userDataFolder));
	}

	// initialize the database
	QString filename = AMUserSettings::userDataFolder + AMUserSettings::userDatabaseFilename;
	QFile dbFile(filename);
	if(!dbFile.exists())
		databaseInitialization();
	else
		databaseUpgrade();


}



#include <dataman/AMDatabaseDefinition.h>
#include <dataman/AMXASScan.h>

/// create structures and tables for a new user database, from scratch
void AMFirstTimeController::databaseInitialization() {

	AMDatabaseDefinition::initializeDatabaseTables(AMDatabase::userdb());

	AMDbObject s1;
	AMDatabaseDefinition::registerType(&s1, AMDatabase::userdb());
	AMScan s2;
	AMDatabaseDefinition::registerType(&s2, AMDatabase::userdb());
	AMXASScan s3;
	AMDatabaseDefinition::registerType(&s3, AMDatabase::userdb());

}

/// Check whether the user database is the most recent version, and migrate if required.
void AMFirstTimeController::databaseUpgrade() {
	/// \todo
}

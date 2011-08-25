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


#ifndef ACQMAN_SETTINGS_H
#define ACQMAN_SETTINGS_H


#include <QSettings>
#include <QString>

/// This class encapsulates user settings/options that are persistent over many runs of the program.  Now that we have a database system for persistent data, only the essential information required to get to the database is stored in here. (See AMUser for the rest of the user's settings.)
/*!
   User-specific settings are stored under the AMUserSettings class.  They can be different for each user account.


	AMSettings are loaded from config. files by calling
   load(), and pushed out to the file with save().  Load() is normally called on program startup, and save() when the AMSettings view gets hit with OK or Apply (\todo... Make an AMSettings view).

   Accessing an option variable is simple and easy:
		AMUserAMSettings::userDataFolder or AMSettings::publicDatabaseFilename, etc.


   For user settings, only two basic pieces of information are stored in the config file: the folder to store the user's data and database, and the name of their database file.  Once we have this information, we can access the user's personal meta-data object, which is stored in the database itself. (see AMUser::user() for more information.)

	Storage locations: on Mac OS X, storage is at:
		- /Library/Preferences/Qt/Acquaman/Acquaman.ini for system-wide stuff
		- ~/.config/Acquaman/Acquaman.ini for user-specific

	On linux, storage is at:
		- /etc/xdg/Acquaman/Acquaman.ini (system-wide)
		- /home/user/.config/Acquaman/Acquaman.ini (user-specific)
   */

class AMUserSettings {
public:
	/// 1. Database and storage:
	// ========================================

	/// Data storage root folder:
	static QString userDataFolder;
	/// name of user database
	static QString userDatabaseFilename;

	/// Generates a default file path and file name (without an extension) within the user data storage folder. You can  trust this to be unique. It will also ensure that the complete path (folders and subfolders) exists all the way down to the destination.  \note This version provides an absolute path name, starting at the root of the filesystem.
	static QString defaultAbsolutePathForScan(const QDateTime&);

	/// Generates a default file path and file name (without an extension) within the user data storage folder. You can  trust this to be unique. It will also ensure that the complete path (folders and subfolders) exists all the way down to the destination.  \note This version provides a relative path name, relative to the user's \c userDataFolder.
	static QString defaultRelativePathForScan(const QDateTime&);

	/// Takes an absolute file path, and if it can be expressed relative to the userDataFolder, returns it as that relative path. (Example: /Users/mboots/acquamanUserData/2010/03/foo.txt becomes 2010/03/foo.txt, if my userDataFolder is /User/mboots/acquamanUserData).  If provided, \c wasInUserDataFolder is set to true if \c absolutePath could be expressed within the userDataFolder, and false if it was outside of that.
	static QString relativePathFromUserDataFolder(const QString& absolutePath, bool* wasInUserDataFolder = 0);



	/// Load settings from disk:
	static void load() ;

	/// Save settings to disk:
	static void save();

};


/// This class encapsulates application-wide settings and options that are persistent over many runs of the program.
class AMSettings {
public:


	/// 1. public database and storage:
	// ========================================

	/// This is where public (archived/reviewed) data is stored, system-wide
	static QString publicDataFolder;
	/// This is the public database filename:
	static QString publicDatabaseFilename;



	/// Load settings from disk:
	static void load();

	/// Save settings to disk:
	static void save();

};





#endif // SETTINGS_H

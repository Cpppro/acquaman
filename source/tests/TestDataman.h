
#include <QtTest/QtTest>
#include "dataman/AMDatabase.h"
#include "dataman/AMScan.h"
#include "dataman/AMDataTree.h"
#include "dataman/AMXASScan.h"
#include "dataman/SGM2004FileLoader.h"
#include "AMErrorMonitor.h"
#include "dataman/AMDbObjectSupport.h"
#include "analysis/AM1DExpressionAB.h"
#include "dataman/AMDataTreeDataStore.h"

#include "util/AMOrderedSet.h"

/// This subclass of AMDbObject is used only for test purposes.
class AMTestDbObject : public AMDbObject {

	Q_OBJECT
	Q_PROPERTY(AMDbObject* myScan READ myScan WRITE reloadMyScan)
			Q_PROPERTY(AMDbObjectList myDbObjects READ myDbObjects WRITE reloadMyDbObjects)
			Q_PROPERTY(int numScans READ numScans WRITE reloadNumScans)

public:
			Q_INVOKABLE explicit AMTestDbObject(QObject* parent = 0) : AMDbObject(parent) {
		mySingleScan_ = 0;
	}//todo

	AMDbObject* myScan() const { return mySingleScan_; }
	void reloadMyScan(AMDbObject* newObj) { if(mySingleScan_) delete mySingleScan_; mySingleScan_ = qobject_cast<AMScan*>(newObj); }

	int numScans() const { return myDbObjects_.count(); }
	void reloadNumScans(int num) { qDebug() << "AMTestDbObject: num scans:" << num; }

	AMDbObjectList myDbObjects() const { return myDbObjects_; }

	void reloadMyDbObjects(const AMDbObjectList& newScans) {
		qDebug() << "AMTestDbObject: creating/loading" << newScans.count() << "brand new scans";
		for(int i=0; i<myDbObjects_.count(); i++)
			if(myDbObjects_.at(i))
				delete myDbObjects_.at(i);
		myDbObjects_ = newScans;
	}



protected:
	AMScan* mySingleScan_;
	AMDbObjectList myDbObjects_;

};

/// This class contains all of the unit tests for the dataman module.
/*! Each private slot corresponds to one test (which can actually contain several individual unit tests.)  The initTestCase() function is run before any of the tests, and the cleanupTestCase is run after all of them finish.
  */
class TestDataman: public QObject
{
	Q_OBJECT
private slots:


	/// This runs before any of the private slots (test cases) get run. It loads the settings and prepares the database tables as required for each Scan object that gets tested.
	void initTestCase()
	{

	}

	/// This is run after all tests are complete.
	void cleanupTestCase()
	{

	}

	void testDataTreeDataStore() {
		qDebug() << "\n\n\n DAVID STARTS HERE \n\nTesting data tree data store\n\n\n";


		AMMeasurementInfo teyInfo("tey", "Total Electron Yield", "amps");
		AMMeasurementInfo tfyInfo("tfy", "Total Fluoresence Yield", "amps");
		AMMeasurementInfo i0Info("I0", "I0", "amps");

		AMAxisInfo sddEVAxisInfo("energy", 1024, "SDD Energy", "eV");
		QList<AMAxisInfo> sddAxes;
		sddAxes << sddEVAxisInfo;
		AMMeasurementInfo sddInfo("sdd", "Silicon Drift Detector", "counts", sddAxes);

		AMAxisInfo xosWavelengthAxisInfo("wavelength", 512, "XOS Wavelength", "nm");
		QList<AMAxisInfo> xosAxes;
		xosAxes << xosWavelengthAxisInfo;
		AMMeasurementInfo xosInfo("xos", "XEOL Optical Spectrometer", "counts", xosAxes);

		AMAxisInfo xessXAxisInfo("x", 1280, "x pixel", "index");
		AMAxisInfo xessYAxisInfo("y", 1024, "y pixel", "index");
		QList<AMAxisInfo> xessAxes;
		xessAxes << xessXAxisInfo << xessYAxisInfo;
		AMMeasurementInfo xessInfo("xess", "XES Spectrometer", "counts", xessAxes);

		AMAxisInfo uglyRAxisInfo("r", 100, "Radial Distance", "percent");
		AMAxisInfo uglyPhiAxisInfo("phi", 180, "Phi Angle", "degrees");
		AMAxisInfo uglyThetaAxisInfo("theta", 360, "Theta Distance", "degrees");
		QList<AMAxisInfo> uglyAxes;
		uglyAxes << uglyRAxisInfo << uglyPhiAxisInfo << uglyThetaAxisInfo;
		AMMeasurementInfo uglyInfo("ugly", "Some Ugly Detector", "events", uglyAxes);

		AMAxisInfo scanAxis("energy", 1000, "Beamline Energy", "eV");

		AMDataTreeDataStore dtds(scanAxis, this);
		QVERIFY(dtds.addMeasurement(teyInfo));
		QVERIFY(dtds.addMeasurement(tfyInfo));
		QCOMPARE(dtds.measurementCount(), 2);
		QVERIFY(dtds.addMeasurement(i0Info));
		QVERIFY(dtds.addMeasurement(sddInfo));
		QVERIFY(dtds.addMeasurement(xosInfo));
		QCOMPARE(dtds.measurementCount(), 5);
		QVERIFY(dtds.addMeasurement(xessInfo));
		QVERIFY(dtds.addMeasurement(uglyInfo));
		QCOMPARE(dtds.measurementCount(), 7);

		AMMeasurementInfo sameAsTeyInfo("tey", "something else", "counts");
		QVERIFY(!dtds.addMeasurement(teyInfo));
		QVERIFY(!dtds.addMeasurement(sameAsTeyInfo));
		QCOMPARE(dtds.measurementCount(), 7);

		QCOMPARE(dtds.idOfMeasurement("tey"), 0);
		QCOMPARE(dtds.idOfMeasurement("ugly"), 6);
		QCOMPARE(dtds.idOfMeasurement("sdd"), 3);
		QCOMPARE(dtds.idOfMeasurement("not in here"), -1);

		QCOMPARE(dtds.measurementAt(1).name, tfyInfo.name);
		QCOMPARE(dtds.measurementAt(1).description, tfyInfo.description);
		QCOMPARE(dtds.measurementAt(1).units, tfyInfo.units);

		QCOMPARE(dtds.measurementAt(5).name, xessInfo.name);
		QCOMPARE(dtds.measurementAt(5).description, xessInfo.description);
		QCOMPARE(dtds.measurementAt(5).units, xessInfo.units);
		QCOMPARE(dtds.measurementAt(5).axes.at(0).name, xessInfo.axes.at(0).name);
		QCOMPARE(dtds.measurementAt(5).axes.at(0).description, xessInfo.axes.at(0).description);
		QCOMPARE(dtds.measurementAt(5).axes.at(0).size, xessInfo.axes.at(0).size);
		QCOMPARE(dtds.measurementAt(5).axes.at(1).name, xessInfo.axes.at(1).name);
		QCOMPARE(dtds.measurementAt(5).axes.at(1).description, xessInfo.axes.at(1).description);
		QCOMPARE(dtds.measurementAt(5).axes.at(1).size, xessInfo.axes.at(1).size);

		AMDataTree *dt = dtds.dataTree();

		dataTreeColumnsPuke(dt);

		qDebug() << "\n\n\nEND TESTING\n\n\n";
	}



	/// Test inserts of DbObjects into the database, and confirm all values loaded back with DbObject::loadFromDb().
	void insertAMDbObject_retrieveStatically() {
		AMDbObject dbo;

		// generate unique name, and set properties:
		//qDebug() << "Generating DbObject with unique properties.";
		QString uniqueName = QDateTime::currentDateTime().toString("ddddMMddhh:mm:ss.zzz");
		dbo.setName("myTestDbObject" + uniqueName);


		// Insert into db:
		*AMDatabase::userdb() << dbo;

		// Was it inserted succesfully?
		//qDebug() << "Testing DbObject insert into database: id should not be 0:" << dbo.id();
		QVERIFY(dbo.id() > 0);	// check that insert succeeded.

		// Load same scan back:
		AMDbObject dbo2;
		dbo2.loadFromDb(AMDatabase::userdb(), dbo.id());
		//qDebug() << "Retrieving DbObject out of database: comparing all parameters with original:";
		QCOMPARE(dbo2.id(), dbo.id());
		QCOMPARE(dbo2.name(), dbo.name());

	}

	/// Test inserts of DbObjects into database, and confirm all values loaded back with dynamic loader:
	void insertAMDbObject_retrieveDynamically() {
		AMDbObject dbo;

		// generate unique name, and set properties:
		//qDebug() << "Generating DbObject with unique properties.";
		QString uniqueName = QDateTime::currentDateTime().toString("ddddMMddhh:mm:ss.zzz");
		dbo.setName("myTestDbObject" + uniqueName);

		// Insert into db:
		*AMDatabase::userdb() << dbo;

		// Was it inserted succesfully?
		//qDebug() << "Testing DbObject insert into database: id should not be 0:" << dbo.id();
		QVERIFY(dbo.id() > 0);	// check that insert succeeded.

		// Load same scan back:
		AMDbObject* dbo2 = 0;
		dbo2 = AMDbObjectSupport::createAndLoadObjectAt(AMDatabase::userdb(), dbo.dbTableName(), dbo.id());


		//qDebug() << "Confirming object was dynamically loaded: pointer is: " << dbo2;
		QVERIFY(dbo2 != 0);

		//qDebug() << "Type of retrieved object should be AMDbObject: " << typeString;
		QCOMPARE(dbo2->type(), QString("AMDbObject"));

		//qDebug() << "Retrieved DbObject out of database: comparing all parameters with original:";
		QCOMPARE(dbo2->id(), dbo.id());
		QCOMPARE(dbo2->name(), dbo.name());
	}

	/// Test inserts of AMScan into the database, and confirm all values loaded back with DbObject::loadFromDb().
	void insertAMScan_retrieveStatically() {
		AMScan dbo;

		// generate unique name, and set properties:
		//qDebug() << "Generating AMScan with unique properties.";
		QString uniqueName = QDateTime::currentDateTime().toString("ddddMMddhh:mm:ss.zzz");
		dbo.setName("myTestAMScan" + uniqueName);
		dbo.setDateTime(QDateTime::currentDateTime());
		dbo.setNumber(42);
		/// \todo dbo.setSampleName("mySampleName" + uniqueName);
		dbo.setNotes("my Comments\n"+uniqueName);

		// Insert into db:
		*AMDatabase::userdb() << dbo;

		// Was it inserted succesfully?
		//qDebug() << "Testing AMScan insert into database: id should not be 0:" << dbo.id();
		QVERIFY(dbo.id() > 0);	// check that insert succeeded.

		// Load same scan back:
		AMScan dbo2;
		dbo2.loadFromDb(AMDatabase::userdb(), dbo.id());
		//qDebug() << "Retrieving AMScan out of database: comparing all parameters with original:";
		QCOMPARE(dbo2.id(), dbo.id());
		QCOMPARE(dbo2.name(), dbo.name());
		QCOMPARE(dbo2.number(), dbo.number());
		QCOMPARE(dbo2.dateTime().toString("yyyy MM/dd hh:mm:ss"), dbo.dateTime().toString("yyyy MM/dd hh:mm:ss"));
		QCOMPARE(dbo2.sampleName(), dbo.sampleName());
		QCOMPARE(dbo2.notes(), dbo.notes());
	}

	/// Test inserts of AMScan into database, and confirm all values loaded back with dynamic loader:
	void insertAMScan_retrieveDynamically() {
		AMScan dbo;

		// generate unique name, and set properties:
		//qDebug() << "Generating AMScan with unique properties.";
		QString uniqueName = QDateTime::currentDateTime().toString("ddddMMddhh:mm:ss.zzz");
		dbo.setName("myTestAMScan" + uniqueName);
		dbo.setDateTime(QDateTime::currentDateTime());
		dbo.setNumber(42);
		/// \todo dbo.setSampleName("mySampleName" + uniqueName);
		dbo.setNotes("my Comments\n"+uniqueName);

		// Insert into db:
		*AMDatabase::userdb() << dbo;

		// Was it inserted succesfully?
		//qDebug() << "Testing AMScan insert into database: id should not be 0:" << dbo.id();
		QVERIFY(dbo.id() > 0);	// check that insert succeeded.

		// Load same scan back:
		AMDbObject* dbo2d = 0;
		QString typeString;
		dbo2d = AMDbObjectSupport::createAndLoadObjectAt(dbo.database(), dbo.dbTableName(), dbo.id());

		//qDebug() << "Confirming object was dynamically loaded: pointer is: " << dbo2d;
		QVERIFY(dbo2d != 0);

		//qDebug() << "Type of retrieved object should be AMScan: " << typeString;
		QCOMPARE(dbo2d->type(), QString("AMScan"));

		//qDebug() << "Confirming dynamically loaded object is an AMScan: pointer is: " << dbo2d;
		AMScan* dbo2 = qobject_cast<AMScan*>(dbo2d);
		QVERIFY(dbo2 != 0);

		// Load same scan back:
		//qDebug() << "Retrieving AMScan out of database: comparing all parameters with original:";
		QCOMPARE(dbo2->id(), dbo.id());
		QCOMPARE(dbo2->name(), dbo.name());
		QCOMPARE(dbo2->number(), dbo.number());
		QCOMPARE(dbo2->dateTime().toString("yyyy MM/dd hh:mm:ss"), dbo.dateTime().toString("yyyy MM/dd hh:mm:ss"));
		QCOMPARE(dbo2->sampleName(), dbo.sampleName());
		QCOMPARE(dbo2->notes(), dbo.notes());
	}

	/// Test inserts of AMScan into the database, and confirm all values loaded back with DbObject::loadFromDb().
	void insertAMScan_search() {
		AMScan dbo;

		// generate unique name, and set properties:
		//qDebug() << "Generating AMScan with unique properties.";
		QString uniqueName = QDateTime::currentDateTime().toString("ddddMMddhh:mm:ss.zzz");
		dbo.setName("myTestAMScan" + uniqueName);
		dbo.setDateTime(QDateTime::currentDateTime());
		dbo.setNumber(QDateTime::currentDateTime().toTime_t());
		/// \todo dbo.setSampleName("mySampleName" + uniqueName);
		dbo.setNotes("my Comments\n"+uniqueName);

		// Insert into db:
		*AMDatabase::userdb() << dbo;

		// Was it inserted succesfully?
		//qDebug() << "Testing AMScan insert into database: id should not be 0:" << dbo.id();
		QVERIFY(dbo.id() > 0);	// check that insert succeeded.

		QList<int> lr;
		// Check: all columns: scans matching should be 1:
		//qDebug() << "Checking objectsMatching finds one matching for each column.";
		//lr = AMDatabase::userdb()->objectsMatching("id", dbo.id());
		//QCOMPARE(lr.count(), 1);
		lr = AMDatabase::userdb()->objectsMatching(dbo.dbTableName(), "name", dbo.name());
		QCOMPARE(lr.count(), 1);
		lr = AMDatabase::userdb()->objectsMatching(dbo.dbTableName(), "number", dbo.number());
		QCOMPARE(lr.count(), 1);
		lr = AMDatabase::userdb()->objectsMatching(dbo.dbTableName(), "sampleId", dbo.sampleId());
		QVERIFY(lr.count() > 1);
		lr = AMDatabase::userdb()->objectsMatching(dbo.dbTableName(), "notes", dbo.notes());
		QCOMPARE(lr.count(), 1);
		lr = AMDatabase::userdb()->objectsMatching(dbo.dbTableName(), "dateTime", dbo.dateTime());
		/// \todo check for 1-minute tolerance on date-time... This fails...
		QCOMPARE(lr.count(), 1);




	}

	/// Test of insert and retrieval from AMDataTree.
	/*!
	  First example is a typical set of XAS data from SGM: energy, tey, and an SDD spectra for each datapoint
	  - create with count of 5 datapoints
	  - fill primary column with energy values: 410.1, 410.2, 410.3, 410.4, 411.0 (use setX() and setValue("eV"))
	  - test retrieval using all methods: .x(i), .value("colName", i), ["colName"][i]
	  - fill tey column with energy^2
	  - attempt to write beyond range of tey data
	  - create column of AMDataTrees for each SDD spectra
	  - retrieve all values and check for matching
	  */
	void insertAMDataTree1() {

		//qDebug() << "Testing creation and inserts of primary column values into data tree.";

		AMDataTree t1(5, "eV", true);
		t1.setX(0, 410.1);
		t1.setX(1, 410.2);
		t1.setValue("eV",2, 410.3);
		t1.setValue("eV",3, 410.4);
		t1.setX(4, 411);

		QCOMPARE(t1.xName(), QString("eV"));
		QCOMPARE(t1.count(), (unsigned)5);
		QCOMPARE(t1.x(0), 410.1);
		QCOMPARE(t1.value("eV", 0), 410.1);
		QCOMPARE(t1.column("eV")[0], 410.1);
		QCOMPARE(t1.x(1), 410.2);
		QCOMPARE(t1.x(2), 410.3);
		QCOMPARE(t1.value("eV",3), 410.4);
		QCOMPARE(t1.column("eV")[3], 410.4);
		QCOMPARE(t1.x(4), 411.0);
		QCOMPARE(t1.setX(500, 1.2345), false);	// index out of range
		QCOMPARE(t1.setValue("eV", 500, 1.2345), false); //index out of range
		QCOMPARE(t1.setValue("nonexistentColumn", 2, 1.2345), false); //non-existent column

		//qDebug() << "Testing creation of data tree with no explicit x values.";
		AMDataTree t2(5, "eV", false);
		QCOMPARE(t2.setX(0, 410.7), false);	// should fail
		QCOMPARE(t2.setValue("eV", 2, 410.7), false);	// should fail
		QCOMPARE(t2.x(0), 0.0);
		QCOMPARE(t2.x(4), 4.0);
		QCOMPARE(t2.x(28), AMDATATREE_OUTOFRANGE_VALUE);	// out of range; should return default-constructed value (-1.0 for double)

		//qDebug() << "Testing copying simple trees, count=5, not even x values.";
		AMDataTree t2Copy(t2);
		QCOMPARE(t2Copy.x(0), 0.0);
		QCOMPARE(t2Copy.x(4), 4.0);
		QCOMPARE(t2Copy.count(), (unsigned)5);
		QCOMPARE(t2Copy.x(5100), AMDATATREE_OUTOFRANGE_VALUE);// out of range; should return default value (and log error message)

		//qDebug() << "Testing copying simple trees, count = 5, with just x values.";
		AMDataTree t1Copy(t1);
		QCOMPARE(t1Copy.xName(), t1.xName());
		for(unsigned i=0; i<t1Copy.count(); i++) {
			QCOMPARE(t1Copy.x(i), t1.x(i));
			QCOMPARE(t1Copy.value("eV", i), t1.value("eV", i));
		}
		//qDebug() << "attempting changing data values and causing deep copy of x-column data.";
		t1Copy.setValue("eV", 3, 300.2);
		t1Copy.setX(4, 400.2);
		QCOMPARE(t1Copy.x(0), t1.x(0));
		QCOMPARE(t1Copy.x(1), t1.x(1));
		QCOMPARE(t1Copy.x(2), t1.x(2));
		QCOMPARE(t1Copy.x(3), 300.2);
		QCOMPARE(t1Copy.x(4), 400.2);
		t1.setX(0, -31.2);
		QCOMPARE(t1.x(0), -31.2);
		QCOMPARE(t1Copy.x(0), 410.1);	// after changing original, t1Copy should still have first original value.

		//qDebug() << "Adding a second column and checking insert/retrieves";
		t1.createColumn("tey");
		for(unsigned i=0; i<t1.count(); i++)
			QCOMPARE(t1.setValue("tey", i, t1.x(i)*t1.x(i) ), true);	// tey = eV^2
		for(unsigned i=0; i<t1.count(); i++) {
			QCOMPARE(t1.value("tey", i), t1.x(i)*t1.x(i) );
			QCOMPARE(t1.column("tey")[i], t1.x(i)*t1.x(i) );
		}

		QCOMPARE(t1.setValue("tey", 300, 1.234), false);	// insert out-of-range should fail
		QCOMPARE(t1.setValue("teyeee", 0, 1.234), false);	// invalid column

		//qDebug() << "inserting subtrees with a count of 1024, x-axis values named 'sddEV', and going from 200 to 1300";
		t1.createSubtreeColumn("sddSpectrum", new AMDataTree(1024, "sddEV", true));
		// insert the x-axis values for each subtree...
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<t1.deeper	("sddSpectrum", i)->count(); j++)
				QCOMPARE( t1.deeper("sddSpectrum", i)->setValue("sddEV", j, 200+j*(1300.0-200.0)/1024.0), true);	// check for successful insert.

		t1.deeper("sddSpectrum", 0)->createColumn("sddCount");
		for(unsigned i=0; i<t1.deeper("sddSpectrum", 0)->count(); i++)
			t1.deeper("sddSpectrum", 0)->setValue("sddCount", i, i);

		qDebug() << "David's check: " << t1.deeper("sddSpectrum", 0)->yColumnNames().count();
		qDebug() << "David's check: " << t1.deeper("sddSpectrum", 0)->yColumnNames();

		// check for correct values:
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<t1.deeper("sddSpectrum", i)->count(); j++)
				QCOMPARE( t1.deeper("sddSpectrum", i)->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.

		//qDebug() << "copying the multi-level tree to t1Copy, using copy constructor.";
		AMDataTree t1Copy2(t1);
		// check for correct values:
		for(unsigned i=0; i<t1Copy2.count(); i++)
			for(unsigned j=0; j<t1Copy2.deeper("sddSpectrum", i)->count(); j++)
				QCOMPARE( t1Copy2.deeper("sddSpectrum", i)->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.

		//qDebug() << "changing a value in t1Copy2, at top-index 3, subtree index 512 in one of the trees; causes deep copies to be made. Checking for change to occur.";

		t1Copy2.deeper("sddSpectrum", 3)->setValue("sddEV", 512, -1.0);
		// check for correct values within copy, even after modifications.
		for(unsigned i=0; i<t1Copy2.count(); i++)
			for(unsigned j=0; j<t1Copy2.deeper("sddSpectrum", i)->count(); j++) {
			if(i==3 && j==512)
				QCOMPARE( t1Copy2.deeper("sddSpectrum", i)->value("sddEV", j),  -1.0);
			else
				QCOMPARE( t1Copy2.deeper("sddSpectrum", i)->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
		}
		//qDebug() << "modified value is:" << t1Copy2.deeper("sddSpectrum", 3)->value("sddEV", 512);


		//qDebug() << "checking that original tree is unaffected by changes to the copy.";
		// check for correct values within original tree, even after modifications.
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<t1.deeper("sddSpectrum", i)->count(); j++) {
			QCOMPARE( t1.deeper("sddSpectrum", i)->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);
			QCOMPARE( t1.deeper("sddSpectrum", i)->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);
		}

		//qDebug() << "original value is:" << t1.deeper("sddSpectrum", 3)->value("sddEV", 512);

		//qDebug() << "copying the multi-level tree to t1Copy3, using assignment operator.";
		AMDataTree t1Copy3 = t1;
		// check for correct values:
		QCOMPARE(t1Copy3.count(), t1.count());
		for(unsigned i=0; i<t1Copy3.count(); i++)
			for(unsigned j=0; j<t1Copy3.deeper("sddSpectrum", i)->count(); j++) {
			QCOMPARE( t1Copy3.deeper("sddSpectrum", i)->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
			QCOMPARE( t1Copy3.deeper("sddSpectrum", i)->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
		}

		//qDebug() << "changing a value in t1Copy3, at top-index 3, subtree index 512 in one of the trees; causes deep copies to be made. Checking for change to occur.";

		t1Copy3.deeper("sddSpectrum", 3)->setValue("sddEV", 512, -1.0);
		// check for correct values within copy, even after modifications.
		for(unsigned i=0; i<t1Copy3.count(); i++)
			for(unsigned j=0; j<t1Copy3.deeper("sddSpectrum", i)->count(); j++) {
			if(i==3 && j==512) {
				QCOMPARE( t1Copy3.deeper("sddSpectrum", i)->value("sddEV", j),  -1.0);
				QCOMPARE( t1Copy3.deeper("sddSpectrum", i)->column("sddEV")[j],  -1.0);
			}
			else {
				QCOMPARE( t1Copy3.deeper("sddSpectrum", i)->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
				QCOMPARE( t1Copy3.deeper("sddSpectrum", i)->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
			}
		}
		//qDebug() << "modified value is:" << t1Copy3.deeper("sddSpectrum", 3)->value("sddEV", 512);


		//qDebug() << "checking that original tree is unaffected by changes to the copy.";
		// check for correct values within original tree, even after modifications.
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<t1.deeper("sddSpectrum", i)->count(); j++) {
			QCOMPARE( t1.deeper("sddSpectrum", i)->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);
			QCOMPARE( t1.deeper("sddSpectrum", i)->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);
		}

		//qDebug() << "original value is:" << t1.deeper("sddSpectrum", 3)->value("sddEV", 512);

	}


	/// Same test as insertAMDataTree1, but using deeperColumn("colName")[i] instead of deeper("colName", i) to access the entire subtree vector, rather than individual subtrees.
	void insertAMDataTree2() {

		//qDebug() << "Testing creation and inserts of primary column values into data tree.";

		AMDataTree t1(5, "eV", true);
		t1.setX(0, 410.1);
		t1.setX(1, 410.2);
		t1.setValue("eV",2, 410.3);
		t1.setValue("eV",3, 410.4);
		t1.setX(4, 411);

		QCOMPARE(t1.xName(), QString("eV"));
		QCOMPARE(t1.count(), (unsigned)5);
		QCOMPARE(t1.x(0), 410.1);
		QCOMPARE(t1.value("eV", 0), 410.1);
		QCOMPARE(t1.column("eV")[0], 410.1);
		QCOMPARE(t1.x(1), 410.2);
		QCOMPARE(t1.x(2), 410.3);
		QCOMPARE(t1.value("eV",3), 410.4);
		QCOMPARE(t1.column("eV")[3], 410.4);
		QCOMPARE(t1.x(4), 411.0);
		QCOMPARE(t1.setX(500, 1.2345), false);	// index out of range
		QCOMPARE(t1.setValue("eV", 500, 1.2345), false); //index out of range
		QCOMPARE(t1.setValue("nonexistentColumn", 2, 1.2345), false); //non-existent column

		//qDebug() << "Testing creation of data tree with no explicit x values.";
		AMDataTree t2(5, "eV", false);
		QCOMPARE(t2.setX(0, 410.7), false);	// should fail
		QCOMPARE(t2.setValue("eV", 2, 410.7), false);	// should fail
		QCOMPARE(t2.x(0), 0.0);
		QCOMPARE(t2.x(4), 4.0);
		QCOMPARE(t2.x(28), AMDATATREE_OUTOFRANGE_VALUE);	// out of range; should return default-constructed value (-1.0 for double)

		//qDebug() << "Testing copying simple trees, count=5, not even x values.";
		AMDataTree t2Copy(t2);
		QCOMPARE(t2Copy.x(0), 0.0);
		QCOMPARE(t2Copy.x(4), 4.0);
		QCOMPARE(t2Copy.count(), (unsigned)5);
		QCOMPARE(t2Copy.x(5100), AMDATATREE_OUTOFRANGE_VALUE);// out of range; should return default value (and log error message)

		//qDebug() << "Testing copying simple trees, count = 5, with just x values.";
		AMDataTree t1Copy(t1);
		QCOMPARE(t1Copy.xName(), t1.xName());
		for(unsigned i=0; i<t1Copy.count(); i++) {
			QCOMPARE(t1Copy.x(i), t1.x(i));
			QCOMPARE(t1Copy.value("eV", i), t1.value("eV", i));
		}
		//qDebug() << "attempting changing data values and causing deep copy of x-column data.";
		t1Copy.setValue("eV", 3, 300.2);
		t1Copy.setX(4, 400.2);
		QCOMPARE(t1Copy.x(0), t1.x(0));
		QCOMPARE(t1Copy.x(1), t1.x(1));
		QCOMPARE(t1Copy.x(2), t1.x(2));
		QCOMPARE(t1Copy.x(3), 300.2);
		QCOMPARE(t1Copy.x(4), 400.2);
		t1.setX(0, -31.2);
		QCOMPARE(t1.x(0), -31.2);
		QCOMPARE(t1Copy.x(0), 410.1);	// after changing original, t1Copy should still have first original value.

		//qDebug() << "Adding a second column and checking insert/retrieves";
		t1.createColumn("tey");
		for(unsigned i=0; i<t1.count(); i++)
			QCOMPARE(t1.setValue("tey", i, t1.x(i)*t1.x(i) ), true);	// tey = eV^2
		for(unsigned i=0; i<t1.count(); i++) {
			QCOMPARE(t1.value("tey", i), t1.x(i)*t1.x(i) );
			QCOMPARE(t1.column("tey")[i], t1.x(i)*t1.x(i) );
		}

		QCOMPARE(t1.setValue("tey", 300, 1.234), false);	// insert out-of-range should fail
		QCOMPARE(t1.setValue("teyeee", 0, 1.234), false);	// invalid column

		//qDebug() << "inserting subtrees with a count of 1024, x-axis values named 'sddEV', and going from 200 to 1300";
		t1.createSubtreeColumn("sddSpectrum", new AMDataTree(1024, "sddEV", true));
		// insert the x-axis values for each subtree...
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<(*t1.deeperColumn("sddSpectrum"))[i]->count(); j++)
				QCOMPARE( (*t1.deeperColumn("sddSpectrum"))[i]->setValue("sddEV", j, 200+j*(1300.0-200.0)/1024.0), true);	// check for successful insert.

		// check for correct values:
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<(*t1.deeperColumn("sddSpectrum"))[i]->count(); j++)
				QCOMPARE( (*t1.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.

		//qDebug() << "copying the multi-level tree to t1Copy, using copy constructor.";
		AMDataTree t1Copy2(t1);
		// check for correct values:
		for(unsigned i=0; i<t1Copy2.count(); i++)
			for(unsigned j=0; j<(*t1Copy2.deeperColumn("sddSpectrum"))[i]->count(); j++)
				QCOMPARE( (*t1Copy2.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.

		//qDebug() << "changing a value in t1Copy2, at top-index 3, subtree index 512 in one of the trees; causes deep copies to be made. Checking for change to occur.";

		(*t1Copy2.deeperColumn("sddSpectrum"))[3]->setValue("sddEV", 512, -1.0);
		// check for correct values within copy, even after modifications.
		for(unsigned i=0; i<t1Copy2.count(); i++)
			for(unsigned j=0; j<(*t1Copy2.deeperColumn("sddSpectrum"))[i]->count(); j++) {
			if(i==3 && j==512)
				QCOMPARE( (*t1Copy2.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  -1.0);
			else
				QCOMPARE( (*t1Copy2.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
		}
		//qDebug() << "modified value is:" << (*t1Copy2.deeperColumn("sddSpectrum"))[3]->value("sddEV", 512);


		//qDebug() << "checking that original tree is unaffected by changes to the copy.";
		// check for correct values within original tree, even after modifications.
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<(*t1.deeperColumn("sddSpectrum"))[i]->count(); j++) {
			QCOMPARE( (*t1.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);
			QCOMPARE( (*t1.deeperColumn("sddSpectrum"))[i]->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);
		}

		//qDebug() << "original value is:" << t1.deeper("sddSpectrum", 3)->value("sddEV", 512);


		//qDebug() << "copying the multi-level tree to t1Copy3, using assignment operator.";
		AMDataTree t1Copy3 = t1;
		// check for correct values:
		QCOMPARE(t1Copy3.count(), t1.count());
		for(unsigned i=0; i<t1Copy3.count(); i++)
			for(unsigned j=0; j<(*t1Copy3.deeperColumn("sddSpectrum"))[i]->count(); j++) {
			QCOMPARE( (*t1Copy3.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
			QCOMPARE( (*t1Copy3.deeperColumn("sddSpectrum"))[i]->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
		}

		//qDebug() << "changing a value in t1Copy3, at top-index 3, subtree index 512 in one of the trees; causes deep copies to be made. Checking for change to occur.";

		(*t1Copy3.deeperColumn("sddSpectrum"))[3]->setValue("sddEV", 512, -1.0);
		// check for correct values within copy, even after modifications.
		for(unsigned i=0; i<t1Copy3.count(); i++)
			for(unsigned j=0; j<(*t1Copy3.deeperColumn("sddSpectrum"))[i]->count(); j++) {
			if(i==3 && j==512) {
				QCOMPARE( (*t1Copy3.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  -1.0);
				QCOMPARE( (*t1Copy3.deeperColumn("sddSpectrum"))[i]->column("sddEV")[j],  -1.0);
			}
			else {
				QCOMPARE( (*t1Copy3.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
				QCOMPARE( (*t1Copy3.deeperColumn("sddSpectrum"))[i]->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);	// check for successful retrieve.
			}
		}
		//qDebug() << "modified value is:" << (*t1Copy3.deeperColumn("sddSpectrum"))[3]->value("sddEV", 512);


		//qDebug() << "checking that original tree is unaffected by changes to the copy.";
		// check for correct values within original tree, even after modifications.
		for(unsigned i=0; i<t1.count(); i++)
			for(unsigned j=0; j<(*t1.deeperColumn("sddSpectrum"))[i]->count(); j++) {
			QCOMPARE( (*t1.deeperColumn("sddSpectrum"))[i]->value("sddEV", j),  200+j*(1300.0-200.0)/1024.0);
			QCOMPARE( (*t1.deeperColumn("sddSpectrum"))[i]->column("sddEV")[j],  200+j*(1300.0-200.0)/1024.0);
		}

		//qDebug() << "original value is:" << (*t1.deeperColumn("sddSpectrum"))[3]->value("sddEV", 512);


	}



	/// test append() to datatrees without subtrees:
	void appendAMDataTree() {

		//qDebug() << "creating empty tree t1, no x values:";
		AMDataTree t1(0, "x", false);
		QCOMPARE(t1.count(), (unsigned)0);
		//qDebug() << "appending 1 value";
		t1.append(3);
		QCOMPARE(t1.count(), (unsigned)1);
		QCOMPARE(t1.x(0), 0.0);	// since we don't have explicit x values, should be 0.
		//qDebug() << "appending another value";
		t1.append(42);
		QCOMPARE(t1.count(), (unsigned)2);
		QCOMPARE(t1.x(1), 1.0);	// since we don't have explicit x values, should be 1.

		//qDebug() << "creating new tree, with actual x values.";
		t1 = AMDataTree(0, "x", true);
		QCOMPARE(t1.count(), (unsigned)0);
		//qDebug() << "appending 1 value";
		t1.append(3);
		QCOMPARE(t1.count(), (unsigned)1);
		QCOMPARE(t1.x(0), 3.);
		//qDebug() << "appending another value";
		t1.append(42);
		QCOMPARE(t1.count(), (unsigned)2);
		QCOMPARE(t1.x(1), 42.);
		//qDebug() << "testing setLastValue()";
		t1.setLastValue("x", -1.234);
		QCOMPARE(t1.x(1), -1.234);

		//qDebug() << "creating new tree, with x and y columns.";
		t1 = AMDataTree(0, "x", true);
		t1.createColumn("y");
		QCOMPARE(t1.count(), (unsigned)0);
		//qDebug() << "appending 1 value";
		t1.append(3);
		QCOMPARE(t1.value("y", 0), AMDATATREE_INSERT_VALUE);	// should be default-inserted value for now
		t1.setLastValue("y", 3*3.);
		QCOMPARE(t1.count(), (unsigned)1);
		QCOMPARE(t1.x(0), 3.);
		QCOMPARE(t1.value("y", 0), 3*3.);
		//qDebug() << "appending another value";
		t1.append(42);
		t1.setLastValue("y", 42*42.);
		QCOMPARE(t1.count(), (unsigned)2);
		QCOMPARE(t1.x(1), 42.);
		QCOMPARE(t1.value("y", 1), 42*42.);
		//qDebug() << "testing setLastValue()";
		t1.setLastValue("y", -1.234);
		QCOMPARE(t1.column("y")[1], -1.234);

		//qDebug() << "creating new tree, with x and y columns and initially 1 value.";
		t1 = AMDataTree(1, "x", true);
		t1.createColumn("y");
		QCOMPARE(t1.count(), (unsigned)1);
		//qDebug() << "appending 1 value";
		t1.append(3);
		QCOMPARE(t1.value("y", 1), AMDATATREE_INSERT_VALUE);	// should be default-inserted value for now
		t1.setLastValue("y", 3*3.);
		QCOMPARE(t1.count(), (unsigned)2);
		QCOMPARE(t1.x(1), 3.);
		QCOMPARE(t1.value("y", 1), 3*3.);
		//qDebug() << "appending another value";
		t1.append(42);
		t1.setLastValue("y", 42*42.);
		QCOMPARE(t1.count(), (unsigned)3);
		QCOMPARE(t1.x(2), 42.);
		QCOMPARE(t1.value("y", 2), 42*42.);
		//qDebug() << "testing setLastValue()";
		t1.setLastValue("y", -1.234);
		QCOMPARE(t1.column("y")[2], -1.234);



		//qDebug() << "creating new empty tree, with x and y columns and subtree of count 5 (meaningless)";
		t1 = AMDataTree(0, "x", true);
		t1.createColumn("y");
		t1.createSubtreeColumn("s1", new AMDataTree(5, "s1_x", true));
		QCOMPARE(t1.count(), (unsigned)0);
		//qDebug() << "appending 1 value";
		t1.append(3);
		//DC		QCOMPARE(t1.deeper("s1", 0)->count(), unsigned(0));	// because the top tree count was 0, there were no copies made of the prototype tree, and a default tree was inserted.
		//DC		QCOMPARE(t1.deeper("s1", 0)->xName(), AMDataTree().xName()); // because the top tree count was 0, there were no copies made of the prototype tree, and a default tree was inserted.
		QCOMPARE(t1.deeper("s1", 0)->count(), unsigned(5));	// because the top tree count was 0, prototype tree was delayed until an append occurred.
		QCOMPARE(t1.deeper("s1", 0)->xName(), QString("s1_x")); // because the top tree count was 0, prototype tree was delayed until an append occurred.

		//qDebug() << "creating new tree, count 1, with x and y columns and subtree of count 5";
		t1 = AMDataTree(1, "x", true);
		t1.createColumn("y");
		t1.createSubtreeColumn("s1", new AMDataTree(5, "s1_x", true));
		QCOMPARE(t1.count(), (unsigned)1);
		QCOMPARE(t1.deeper("s1", 0)->count(), unsigned(5));	// confirm creation of prototype tree
		QCOMPARE(t1.deeper("s1", 0)->xName(), QString("s1_x")); // confirm creation of prototype tree
		for(unsigned i=0; i<5; i++)
			t1.deeper("s1",0)->setX(i, i+2.3);
		for(unsigned i=0; i<5; i++)
			QCOMPARE(t1.deeper("s1", 0)->x(i), i+2.3);
		//qDebug() << "appending 2nd value. Subtree at i=0 should be copied into i=1";
		t1.append(3);
		QCOMPARE(t1.deeper("s1", 1)->count(), unsigned(5));	// confirm copying of prototype tree
		QCOMPARE(t1.deeper("s1", 1)->xName(), QString("s1_x")); // confirm copying of prototype tree
		for(unsigned i=0; i<5; i++)
			QCOMPARE(t1.deeper("s1", 1)->x(i), i+2.3);



	}


	/// test loading an AMXASScan from legacy SGM data. Also tests creation of default channels inside SGM2004FileLoader::loadFromFile()

	void loadAMXASScan() {
		AMXASScan s1;
		SGM2004FileLoader s1Loader(&s1);
		/// \todo move this into proper storage location in data dir.
		QString fileName = AMUserSettings::userDataFolder + "testScriptData/sgm001.dat";
		//		qDebug() << "loading sgm data from file and checking for proper read:" << fileName;
		QVERIFY(s1Loader.loadFromFile(fileName));
		QCOMPARE(s1.scanSize(0), int(301));
		QCOMPARE(s1.notes(), QString("0.916667"));	// NOTE: this test fails, because we're currently putting the Grating and Integration Time inside the comment field (for lack of a better place) Eventually, the Grating and Integration time should become part of the scan configuration, or beamline state.
		QCOMPARE(s1.dateTime().toTime_t(), uint(1285706567));
		//qDebug() << "s1 raw data columns ('detectors')" << s1.detectors();


		int tey_n, tfy_n, tey, tfy, io;
		QVERIFY((tey = s1.rawDataSources()->indexOf("tey")) != -1);
		QVERIFY((tfy = s1.rawDataSources()->indexOf("tfy")) != -1);
		QVERIFY((io = s1.rawDataSources()->indexOf("I0")) != -1);
		QVERIFY((tey_n = s1.analyzedDataSources()->indexOf("tey_n")) != -1);
		QVERIFY((tfy_n = s1.analyzedDataSources()->indexOf("tfy_n")) != -1);
		QCOMPARE(qobject_cast<AM1DExpressionAB*>(s1.analyzedDataSources()->at(tey_n))->expression(), QString("tey/I0"));
		QCOMPARE(qobject_cast<AM1DExpressionAB*>(s1.analyzedDataSources()->at(tfy_n))->expression(), QString("-1*tfy/I0"));
		// test math parser:
		for(int i=0; i<s1.scanSize(0); i++)
			QVERIFY((double)s1.analyzedDataSources()->at(tey_n)->value(i) == (double)s1.rawDataSources()->at(tey)->value(i) / (double)s1.rawDataSources()->at(io)->value(i));
		// test math parser:
		for(int i=0; i<s1.scanSize(0); i++)
			QVERIFY((double)s1.analyzedDataSources()->at(tfy_n)->value(i) == (double)s1.rawDataSources()->at(tfy)->value(i) / (double)s1.rawDataSources()->at(io)->value(i));
	}


	/// Using test file testScriptData/sgm001.dat, tests AM1DExpressionAB expressions (setting expression, setting xExpression, using default xExpression, evaluating expressions, setting and evaluating invalid expressions)
	void testAM1DExpressionAB() {
		AMXASScan s1;
		SGM2004FileLoader s1Loader(&s1);
		/// \todo move this into proper storage location in data dir.
		QString fileName = AMUserSettings::userDataFolder + "testScriptData/sgm001.dat";
		//qDebug() << "loading sgm data from file and checking for proper read:" << fileName;
		QVERIFY(s1Loader.loadFromFile(fileName));

		// create simple math channel
		AM1DExpressionAB* const5Channel = new AM1DExpressionAB("const5");
		QList<AMDataSource*> rawDataSources;
		foreach(AMRawDataSource* ds, s1.rawDataSources()->toList())
			rawDataSources << ds;
		const5Channel->setInputDataSources(rawDataSources);
		QVERIFY(const5Channel->setExpression("3+2"));
		QVERIFY(const5Channel->isValid());
		s1.addAnalyzedDataSource(const5Channel);

		QVERIFY(const5Channel->value(0) == AMNumber(5.0));
		QVERIFY(const5Channel->value(3) == AMNumber(5.0));
		QVERIFY(const5Channel->value(900) == AMNumber(AMNumber::OutOfBoundsError));


		// verify auto-created channels (inside loadFromFile())
		QVERIFY(s1.analyzedDataSources()->indexOf("tey_n") == 0);
		QVERIFY(s1.analyzedDataSources()->indexOf("tfy_n") == 1);

		AM1DExpressionAB* teyn = qobject_cast<AM1DExpressionAB*>(s1.analyzedDataSources()->at(0));
		AM1DExpressionAB* tfyn = qobject_cast<AM1DExpressionAB*>(s1.analyzedDataSources()->at(1));
		QVERIFY(teyn);
		QVERIFY(tfyn);

		for(int i=0; i<teyn->size(0); i++)
			QVERIFY(teyn->axisValue(0, i) == s1.rawData()->axisValue(0, i));

		// set x expression and check for adjusted values. (ex: calibrating by +1 eV)
		QVERIFY(teyn->setXExpression("tey.x + 1"));
		QVERIFY(teyn->isValid());
		for(int i=0; i<teyn->size(0); i++)
			QVERIFY(double(teyn->axisValue(0, i)) == double(s1.rawData()->axisValue(0, i))+1.0);

		// restore the x expression to default and check values:
		QVERIFY(teyn->setXExpression());
		QVERIFY(teyn->isValid());
		for(int i=0; i<teyn->size(0); i++)
			QVERIFY(teyn->axisValue(0, i) == s1.rawData()->axisValue(0, i));


		// set the y expression and check values:
		QVERIFY(teyn->setExpression("tey/I0 + 4"));
		QVERIFY(teyn->isValid());

		// need to get index of tey column and I0 column in data tree...
		int teyColIndex = -1;
		for(int i=0; i<s1.rawData()->measurementCount(); i++) {
			if(s1.rawData()->measurementAt(i).name == "tey") {
				teyColIndex = i;
				break;
			}
		}
		int ioColIndex = -1;
		for(int i=0; i<s1.rawData()->measurementCount(); i++) {
			if(s1.rawData()->measurementAt(i).name == "I0") {
				ioColIndex = i;
				break;
			}
		}
		QVERIFY(teyColIndex != -1);
		QVERIFY(ioColIndex != -1);

		for(int i=0; i<teyn->size(0); i++)
			QVERIFY(teyn->value(i) == (double)s1.rawData()->value(i, teyColIndex, AMnDIndex()) / (double)s1.rawData()->value(i, ioColIndex, AMnDIndex())+4.0);
		// reset the y expression and check values:
		QVERIFY(teyn->setExpression("tey/I0"));
		QVERIFY(teyn->isValid());
		for(int i=0; i<teyn->size(0); i++)
			QVERIFY(teyn->value(i) == (double)s1.rawData()->value(i, teyColIndex, AMnDIndex()) / (double)s1.rawData()->value(i, ioColIndex, AMnDIndex()));

		// set a bad expression, check for failure, and check all values = InvalidValue.
		QVERIFY(teyn->setExpression("garbage + 4") == false);
		QVERIFY(!teyn->isValid());
		for(int i=0; i<teyn->size(0); i++)
			QVERIFY(teyn->value(i) == AMNumber(AMNumber::InvalidError));

		// set a bad x expression, check for failure, and check all values = c1->invalidValue()
		QVERIFY(teyn->setXExpression("garbage + kj3423") == false);
		QVERIFY(!teyn->isValid());
		for(int i=0; i<teyn->size(0); i++)
			QVERIFY(teyn->axisValue(0, i) == AMNumber(AMNumber::InvalidError));


	}


	void testDbObjectComposition() {
		AMDbObjectSupport::registerClass<AMTestDbObject>();

		AMTestDbObject* t1 = new AMTestDbObject();
		t1->storeToDb(AMDatabase::userdb());

		t1->loadFromDb(t1->database(), t1->id());
		QVERIFY(t1->myScan() == 0);

		AMScan* s = new AMScan();
		s->setName("special scan test name! With secret sauce.");
		t1->reloadMyScan(s);
		QVERIFY(t1->myScan() == s);

		t1->storeToDb(t1->database());

		s->setName("NO secret sauce.");

		t1->loadFromDb(t1->database(), t1->id());

		QVERIFY(t1->myScan() == s);	// first version of loadFromDb. Since myScan() returns a valid pointer, should simply call loadFromDb() on s.
		QCOMPARE(t1->myScan()->name(), QString("special scan test name! With secret sauce."));	// verifies s was reloaded properly.

		t1->myScan()->setName("NO secret sauce.");
		t1->reloadMyScan(0);	// deletes s.  myScan() will no longer return valid object.
		QVERIFY(t1->myScan() == 0);
		t1->loadFromDb(t1->database(), t1->id());
		// second version of loadFromDb(). Since myScan() didn't return valid, a new object was created.  myScan() should now have the original properties of s, but the pointer should be a different object than s. (But pointer might match s! the memory allocator is sneaky like that... there's an empty hole of just the write size to re-allocate the new one into.)
		QVERIFY(t1->myScan() != 0);
		//this might actually be (and turns out to happen). Could luckily allocate to just-deleted spot of s. QVERIFY(t1->myScan() != s);
		QCOMPARE(t1->myScan()->name(), QString("special scan test name! With secret sauce."));	// verifies "new s" was reloaded properly.


		// reload a completely new one...
		AMTestDbObject* t2 = new AMTestDbObject();
		t2->loadFromDb(t1->database(), t1->id());

		QCOMPARE(t2->myScan()->name(), QString("special scan test name! With secret sauce."));


		// test list of AMDbObjects...

		AMDbObjectList threeObjects;
		AMDbObject* o1 = new AMDbObject(); o1->setName("AMDbObject: object1");
		AMScan* o2 = new AMScan(); o2->setName("AMScan: object2");
		AMXASScan* o3 = new AMXASScan(); o3->setName("AMXASScan: object3");

		threeObjects << o1 << o2 << o3;

		t2->reloadMyDbObjects(threeObjects);
		QCOMPARE(t2->myScan()->name(), QString("special scan test name! With secret sauce."));

		QCOMPARE(t2->numScans(), 3);

		t2->storeToDb(AMDatabase::userdb());


		QCOMPARE(t1->numScans(), 0);
		t1->loadFromDb(AMDatabase::userdb(), t2->id());	// should create brand new objects; t1 didn't have three in the list.

		// catches a bug where member variable locations weren't being stored in the db if the member object wasn't modified. If it's not modified and already in the db, we don't need to re-storeToDb() it, but we still need remember that we own it, and remember where it is.
		QVERIFY(t1->myScan());
		QCOMPARE(t1->myScan()->name(), QString("special scan test name! With secret sauce."));

		QCOMPARE(t1->numScans(), 3);
		AMDbObjectList checkList = t1->myDbObjects();
		QCOMPARE(checkList.count(), 3);
		QCOMPARE(checkList.at(0)->name(), QString("AMDbObject: object1"));
		QCOMPARE(checkList.at(1)->name(), QString("AMScan: object2"));
		QCOMPARE(checkList.at(2)->name(), QString("AMXASScan: object3"));

		// how do we know that these are new objects? Could try accessing o1, o2, o3, but they should have been deleted. A crash would indicate success ;p  Hmmm...


		t2->myDbObjects().at(0)->setName("Going going gone...");
		QCOMPARE(t2->myDbObjects().at(0)->name(), QString("Going going gone..."));

		t2->loadFromDb(t2->database(), t2->id());	// first version of loadFromDb()... all three exist already and match in type... should not call setProperty().  Verify that "AMTestDbObject: creating/loading brand new scans" only appears twice in the test log.
		QCOMPARE(t2->myDbObjects().at(0)->name(), QString("AMDbObject: object1")); // should be restored from last saved state...

	}


	void testAMOrderedSet() {
		AMOrderedSet<QString, int> s1;

		s1.setAllowsDuplicateKeys(false);

		QVERIFY(s1.append(0, "0"));
		QVERIFY(s1.append(1, "1"));
		QVERIFY(s1.append(2, "2"));
		QVERIFY(!s1.append(3, "2"));	// duplicate key.
		QVERIFY(s1.append(3, "3"));
		QVERIFY(s1.append(4, "4"));

		QVERIFY(s1.at(0) == 0);
		QVERIFY(s1.at(1) == 1);
		QVERIFY(s1.at(2) == 2);
		QVERIFY(s1.at(3) == 3);
		QVERIFY(s1.at(4) == 4);

		QVERIFY(s1.indexOf("0") == 0);
		QVERIFY(s1.indexOf("1") == 1);
		QVERIFY(s1.indexOf("2") == 2);
		QVERIFY(s1.indexOf("3") == 3);
		QVERIFY(s1.indexOf("4") == 4);

		// test remove in middle:
		s1.remove(2);
		QVERIFY(s1.count() == 4);

		QVERIFY(s1.indexOf("0") == 0);
		QVERIFY(s1.indexOf("1") == 1);
		QVERIFY(s1.indexOf("2") == -1);
		QVERIFY(s1.indexOf("3") == 2);	// index moves down
		QVERIFY(s1.indexOf("4") == 3);

		QVERIFY(s1.at(0) == 0);
		QVERIFY(s1.at(1) == 1);
		QVERIFY(s1.at(2) == 3);	// value moves down
		QVERIFY(s1.at(3) == 4);


		// check insert inside:
		s1.insert(1, 88, "eighty-eight");

		QVERIFY(s1.count() == 5);

		QVERIFY(s1.indexOf("0") == 0);
		QVERIFY(s1.indexOf("eighty-eight") == 1);
		QVERIFY(s1.indexOf("1") == 2);	// index moves up; was inserted in front
		QVERIFY(s1.indexOf("2") == -1);
		QVERIFY(s1.indexOf("3") == 3);
		QVERIFY(s1.indexOf("4") == 4);

		QVERIFY(s1.at(0) == 0);
		QVERIFY(s1.at(1) == 88);
		QVERIFY(s1.at(2) == 1);	// value moves down
		QVERIFY(s1.at(3) == 3);
		QVERIFY(s1.at(4) == 4);

		QVERIFY(s1.replace(1, 77, "seventyseven") == 88);
		QVERIFY(s1.indexOf("eighty-eight") == -1);
		QVERIFY(s1.indexOf("seventyseven") == 1);

		s1.clear();
		QVERIFY(s1.indexOf("seventyseven") == -1);
		QVERIFY(s1.count() == 0);

	}

};

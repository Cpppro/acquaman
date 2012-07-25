#include "VESPERSConfigurationFileBuilder.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>

VESPERSConfigurationFileBuilder::VESPERSConfigurationFileBuilder(QObject *parent)
	: QObject(parent)
{
	dimensions_ = 0;
	singleElement_ = false;
	fourElement_ = false;
	roperCCD_ = false;
	pvNameAxis1_ = "";
	pvNameAxis2_ = "";
}

bool VESPERSConfigurationFileBuilder::buildConfigurationFile()
{
	// Find out which path we are using for acquaman (depends on whether you are on Mac or Linux or beamline OPI).
	QString homeDir = QDir::homePath();
	if(QDir(homeDir+"/dev").exists())
		homeDir.append("/dev");
	else if(QDir(homeDir+"/beamline/programming").exists())
		homeDir.append("/beamline/programming");

	homeDir.append("/acquaman/devConfigurationFiles/VESPERS");

	QFile file;
	file.setFileName(homeDir+"/template.cfg");

	if (!file.open(QIODevice::WriteOnly))
		return false;

	if (dimensions_ != 1 && dimensions_ != 2)
		return false;

	if (pvNameAxis1_.isEmpty() || (dimensions_ == 2 && pvNameAxis2_.isEmpty()))
		return false;

	double delay = 0;
	QString status1 = pvNameAxis1_;
	QString status2 = pvNameAxis2_;

	if (pvNameAxis1_.contains("Mono")){

		delay = 0.1;
		status1  = "SMTR1607-`-B20-20:status";
		status2 = "";
	}

	else{

		delay = 0.1;
		status1.replace(":mm", ":status");
		status2.replace(":mm", ":status");
	}

	QString contents;

	// Begin phase.
	contents.append("# Version 20600\n");

	if (dimensions_ == 1)
		contents.append("# Scan \"axis1\" 1 onStart:1\n");
	else
		contents.append("# Scan \"axis2\" 1 onStart:1\n");

	contents.append(QString("# Control \"%1\" start:0 delta:1 final:1 active:7\n").arg(pvNameAxis1_));

	if (roperCCD_){

		contents.append("# Action Begin SetPV \"IOC1607-003:det1:NumImages\" \"1\"\n");
		contents.append("# Action Begin SetPV \"IOC1607-003:det1:NumAcquisitions\" \"1\"\n");
		contents.append("# Action Begin SetPV \"IOC1607-003:det1:TriggerMode\" \"1\"\n");
		contents.append("# Action Begin SetPV \"DIO1607-01:CCD:ExtSync.HIGH\" \"0.01\"\n");
	}

	if (singleElement_){

		contents.append("# Action Begin SetPV \"IOC1607-004:mca1Status.SCAN\" \"9\"\n");
		contents.append("# Action Begin SetPV \"IOC1607-004:mca1Read.SCAN\" \"0\"\n");
	}

	if (fourElement_){

		contents.append("# Action Begin SetPV \"dxp1607--B21-04:StatusAll.SCAN\" \"9\"\n");
		contents.append("# Action Begin SetPV \"dxp1607--B21-04:ReadAll.SCAN\" \"0\"\n");
		contents.append("# Action Begin SetPV \"dxp1607--B21-04:ReadDXPs.SCAN\" \"0\"\n");
	}

	contents.append("# Action Begin CallEvent \"background\" 1\n");
	contents.append("# Action Begin Delay 0.5\n");

	// Move phase.
	if (dimensions_ == 1){

		contents.append("# Action Move WaitPV \"07B2:POE_BeamStatus\" \"1\"\n");

		if (roperCCD_)
			contents.append("# Action Move SetPV \"IOC1607-003:det1:Acquire\" \"1\"\n");

		contents.append(QString("# Action Move Delay %1\n").arg(delay));
		contents.append(QString("# Action Move WaitPV \"%1\" \"MOVE DONE\"\n").arg(status1));
	}

	else if (dimensions_ == 2){

		contents.append(QString("# Action Move Delay %1\n").arg(delay));
		contents.append(QString("# Action Move WaitPV \"%1\" \"MOVE DONE\"\n").arg(status2));
	}

	// Dwell phase.
	if (dimensions_ == 1){

		if (roperCCD_)
			contents.append("# Action Dwell WaitPV \"CCD1607-001:extTrig:lock\" \"unlocked\"\n");

		contents.append(QString("# Action Dwell Delay %1\n").arg(delay));
		contents.append("# Action Dwell SetPV \"BL1607-B2-1:dwell:startScan\" \"1\"\n");
		contents.append("# Action Dwell Delay 0.01\n");
		contents.append("# Action Dwell WaitPV \"BL1607-B2-1:dwell:startScan\" \"Stopped\"\n");
		contents.append("# Action Dwell Delay 0.05\n");

		if (singleElement_){

			contents.append("# Action Dwell SetPV \"IOC1607-004:dxp1:ReadParams.PROC\" \"1\"\n");
			contents.append("# Action Dwell Delay 0.05\n");
		}

		contents.append("# Action Dwell CallEvent \"readMCS\" 1\n");

		if (roperCCD_){

			contents.append("# Action Dwell SetPV \"DIO1607-01:CCD:ExtSync\" \"0\"\n");
			contents.append("# Action Dwell Delay 0.05\n");
		}
	}

	else if (dimensions_ == 2){

		contents.append(QString("# Action Dwell Delay %1\n").arg(delay));
		contents.append(QString("# Action Dwell CallScan \"axis1\" 1\n").arg(status2));
	}

	// Finish phase.
	contents.append("# Action Finish WaitPV \"BL1607-B2-1:dwell:startScan\" \"Stopped\"\n");

	if (singleElement_){

		contents.append("# Action Finish SetPV \"IOC1607-004:mca1Status.SCAN\" \"6\"\n");
		contents.append("# Action Finish SetPV \"IOC1607-004:mca1Read.SCAN\" \"6\"\n");
	}

	if (fourElement_){

		contents.append("# Action Finish SetPV \"dxp1607--B21-04:StatusAll.SCAN\" \"1 second\"\n");
		contents.append("# Action Finish SetPV \"dxp1607--B21-04:ReadAll.SCAN\" \"1 second\"\n");
		contents.append("# Action Finish SetPV \"dxp1607--B21-04:ReadDXPs.SCAN\" \"1 second\"\n");
	}

	// Second axis (if applicable).
	if (dimensions_ == 2){

		contents.append("# Scan \"axis1\" 2 onStart:0\n");
		contents.append(QString("# Control \"%1\" start:0 delta:1 final:1 active:7\n").arg(pvNameAxis2_));
		contents.append("# Action Move WaitPV \"07B2:POE_BeamStatus\" \"1\"\n");

		if (roperCCD_)
			contents.append("# Action Move SetPV \"IOC1607-003:det1:Acquire\" \"1\"\n");

		contents.append(QString("# Action Move Delay %1\n").arg(delay));
		contents.append(QString("# Action Move WaitPV \"%1\" \"MOVE DONE\"\n").arg(status2));
		if (roperCCD_)
			contents.append("# Action Dwell WaitPV \"CCD1607-001:extTrig:lock\" \"unlocked\"\n");

		contents.append(QString("# Action Dwell Delay %1\n").arg(delay));
		contents.append("# Action Dwell SetPV \"BL1607-B2-1:dwell:startScan\" \"1\"\n");
		contents.append("# Action Dwell Delay 0.05\n");
		contents.append("# Action Dwell WaitPV \"BL1607-B2-1:dwell:startScan\" \"Stopped\"\n");
		contents.append("# Action Dwell Delay 0.05\n");

		if (singleElement_){

			contents.append("# Action Dwell SetPV \"IOC1607-004:dxp1:ReadParams.PROC\" \"1\"\n");
			contents.append("# Action Dwell Delay 0.05\n");
		}

		contents.append("# Action Dwell CallEvent \"readMCS\" 1\n");

		if (roperCCD_){

			contents.append("# Action Dwell SetPV \"DIO1607-01:CCD:ExtSync\" \"0\"\n");
			contents.append("# Action Dwell Delay 0.05\n");
		}
	}

	// Event "readMCS"
	contents.append("# Event \"readMCS\" 1\n");
	contents.append("# datastream columns: eventID, absolute/relative time stamps\n");
	contents.append("# eventID:1 AbsTime:0 relPTime:0\n");
	contents.append("# commentPrefix: 0\n");
	contents.append(QString("# PV 0: \"%1\" disable:0 format:\"\%.4f\" spectrum:0 ready:0\n").arg(pvNameAxis1_));
	if (dimensions_ == 2)
		contents.append(QString("# PV 1: \"%1\" disable:0 format:\"\%.4f\" spectrum:0 ready:0\n").arg(pvNameAxis2_));

	for (int i = 0; i < 90-dimensions_; i++)
		contents.append(QString("# PV %1: \"Goober\", disable:0 spectrum:0 ready:0\n").arg(i+dimensions_));

	// Event "background"
	contents.append("#\n");
	contents.append("# Event \"background\" 2\n");
	contents.append("# datastream columns: eventID, absolute/relative time stamps\n");
	contents.append("# eventID:0 AbsTime:0 Rel0Time:0 relPtime:0\n");
	contents.append("# commentPrefix: 1\n");
	if (dimensions_ == 1){

		contents.append("# PV 0: \"BL1607-B2-1:mcs:background[0]\" disable:0 format:\"Background counts time=%g(x .1 microseconds)\" spectrum:0 ready:0\n");
		contents.append("# PV 1: \"BL1607-B2-1:mcs:background[5-10]\" disable:0 spectrum:0 ready:0\n");
		contents.append("# PV 2: \"07B2_Mono_SineB_Eo\" disable:0 spectrum:0 ready:0\n");
	}
	else if (dimensions_ == 2){

		contents.append("# PV 0: \"$(Background)[0]\" disable:0 format:\"Background counts time=\%g(x .1 microseconds)\" spectrum:0 ready:0\n");
		contents.append("# PV 1: \"$(Background)[1-4]\" disable:0 spectrum:0 ready:0\n");
		contents.append("# PV 2: \"$(AbsEnergyName)\" disable:0 format:\"\%10.6f\" spectrum:0 ready:0\n");
		contents.append("# PV 3: \"$(EnergyFeedback)\" disable:0 format:\"\%10.6f\" spectrum:0 ready:0\n");
		contents.append("# PV 4: \"$(acqDwellmSec)\" disable:0 spectrum:0 ready:0\n");
		contents.append("# PV 5: \"$(4Elem):PresetReal\" disable:0 spectrum:0 ready:0\n");
		contents.append("# PV 6: \"$(4Elem):EnergyPkTime\" disable:0 format:\"PkTime=\%.2f\" spectrum:0 ready:0\n");
		contents.append("# PV 7: \"$(4ElemDXP)1.GAPTIM\" disable:0 format:\"GapTime=\%.2f\" spectrum:0 ready:0\n");
		contents.append("# PV 8: \"$(4Elem):mcaEMax\" disable:0 spectrum:0 ready:0\n");
		contents.append("# PV 9: \"$(4ElemMCA)Corrected.NUSE\" disable:0 spectrum:0 ready:0\n");
	}

	contents.append("#\n");

	// Writing file info.
	contents.append("# File: \"(null)\" sequence:0 time:0 datedir:0\n");
	contents.append("# \"(null)\" sequence:1\n");
	contents.append("# Header: 1\n");
	contents.append("# SpectrumFormat: 2\n");

	// Write to file.
	QTextStream out(&file);

	out << contents;

	file.close();

	return true;
}
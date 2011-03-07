#ifndef AMDETECTORINFOVIEW_H
#define AMDETECTORINFOVIEW_H

#include <QGroupBox>
#include "acquaman/AMDetectorInfoList.h"
#include "acquaman/AMDetectorInfoList.h"
#include <QBoxLayout>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QFormLayout>

class AMDetectorInfoView : public QGroupBox
{
	Q_OBJECT
public:
	AMDetectorInfoView(AMDetectorInfo *detectorInfo, AMDetectorInfo *writeDetectorInfo = 0, bool interactive = false, QWidget *parent = 0);

protected:
	AMDetectorInfo *detectorInfo_;
	AMDetectorInfo *writeDetectorInfo_;
	QPushButton *switchToEditBox_;
	bool interactive_;
	QHBoxLayout *hl_;
	QVBoxLayout *vl_;
};

class PGTOldDetectorInfoView : public AMDetectorInfoView
{
	Q_OBJECT
public:
	PGTOldDetectorInfoView(PGTDetectorInfo *detectorInfo, AMDetectorInfo *writeDetectorInfo = 0, bool interactive = false, QWidget *parent = 0);

protected:
	PGTDetectorInfo *sDetectorInfo_;
	QDoubleSpinBox *integrationTimeBox_;
	QComboBox *integrationModeBox_;
	QDoubleSpinBox *hvSetpointBox_;
	QList<QWidget*> allBoxes_;
	QFormLayout *fl_;
};

class MCPOldDetectorInfoView : public AMDetectorInfoView
{
	Q_OBJECT
public:
	MCPOldDetectorInfoView(MCPDetectorInfo *detectorInfo, AMDetectorInfo *writeDetectorInfo = 0, bool interactive = false, QWidget *parent = 0);

protected:
	MCPDetectorInfo *sDetectorInfo_;
	QDoubleSpinBox *hvSetpointBox_;
	QList<QWidget*> allBoxes_;
	QFormLayout *fl_;
};



class AMDetectorInfoSetView : public QGroupBox
{
	Q_OBJECT
public:
	AMDetectorInfoSetView(AMOldDetectorInfoSet *viewSet, AMOldDetectorInfoSet *writeSet = 0, bool setup = true, QWidget *parent = 0);

	QWidget* boxByName(const QString &name){
		return detectorBoxes_.at(viewSet_->indexOf(name));
	}

	QWidget* boxAt(int row){
		return detectorBoxes_.at(row);
	}

protected:
	AMOldDetectorInfoSet *viewSet_;
	AMOldDetectorInfoSet *writeSet_;
	QList< QWidget* > detectorBoxes_;
	QList< QWidget* > detectorDetails_;
	QList< QWidget* > detailViews_;
	QHBoxLayout *hl_;

	virtual void runSetup();
	virtual QWidget* detailViewByType(AMDetectorInfo *detectorInfo, AMDetectorInfo *writeDetectorInfo);
};

#endif // AMDETECTORINFOVIEW_H

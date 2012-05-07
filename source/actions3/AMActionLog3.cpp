/*
Copyright 2010-2012 Mark Boots, David Chevrier, and Darren Hunter.

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


#include "AMActionLog3.h"

#include "actions3/AMLoopAction3.h"
#include "dataman/database/AMDbObjectSupport.h"
#include "util/AMErrorMonitor.h"

AMActionLog3::AMActionLog3(QObject *parent) :
	AMDbObject(parent)
{
	info_ = 0;
	finalState_ = 0;
}

AMActionLog3::AMActionLog3(const AMAction3 *completedAction, QObject *parent) :
	AMDbObject(parent)
{
	if(completedAction){
		const AMListAction3 *listAction = qobject_cast<const AMListAction3*>(completedAction);
		const AMLoopAction3 *loopAction = qobject_cast<const AMLoopAction3*>(completedAction);
		if(listAction){
			actionInheritedList_ = true;
			info_ = const_cast<AMActionInfo3*>(completedAction->info());
		}
		else{
			actionInheritedList_ = false;
			info_ = completedAction->info()->createCopy();
		}
		if(loopAction)
			actionInheritedLoop_ = true;
		else
			actionInheritedLoop_ = false;
		startDateTime_ = completedAction->startDateTime();
		finalState_ = completedAction->state();
		setName(info_->shortDescription());
		if(completedAction->inFinalState())
			endDateTime_ = completedAction->endDateTime();
		else
			endDateTime_ = QDateTime::currentDateTime();
	}
	else{
		info_ = 0;
		finalState_ = 0;
	}
}

AMActionLog3::AMActionLog3(const AMActionLog3 &other) :
	AMDbObject(other)
{
	if(other.isValid()) {
		actionInheritedList_ = other.actionInheritedList();
		if(actionInheritedList_)
			info_ = const_cast<AMActionInfo3*>(other.info());
		else
			info_ = other.info()->createCopy();
		actionInheritedLoop_ = other.actionInheritedLoop();
		finalState_ = other.finalState();
		startDateTime_ = other.startDateTime();
		endDateTime_ = other.endDateTime();
		setName(info_->shortDescription());
	}
	else {
		info_ = 0;
		finalState_ = 0;
	}
}

AMActionLog3::~AMActionLog3() {
	if(!actionInheritedList_)
		delete info_;
	info_ = 0;
}

bool AMActionLog3::setFromAction(const AMAction3 *completedAction)
{
	if(completedAction && completedAction->inFinalState()) {
		if(!actionInheritedList_)
			delete info_;
		info_ = completedAction->info()->createCopy();
		finalState_ = completedAction->state();
		startDateTime_ = completedAction->startDateTime();
		endDateTime_ = completedAction->endDateTime();
		setName(info_->shortDescription());
		setModified(true);
		return true;
	}
	else {
		return false;
	}
}

void AMActionLog3::setParentId(int parentId){
	parentId_ = parentId;
}

void AMActionLog3::dbLoadStartDateTime(const AMHighPrecisionDateTime &startDateTime)
{
	startDateTime_ = startDateTime;
	setModified(true);
}

void AMActionLog3::dbLoadEndDateTime(const AMHighPrecisionDateTime &endDateTime)
{
	endDateTime_ = endDateTime;
	setModified(true);
}

void AMActionLog3::dbLoadFinalState(int finalState)
{
	finalState_ = finalState;
	setModified(true);
}

void AMActionLog3::dbLoadInfo(AMDbObject *newInfo)
{
	AMActionInfo3* info = qobject_cast<AMActionInfo3*>(newInfo);
	if(info) {
		if(!actionInheritedList_)
			delete info_;
		info_ = info;
		setName(info_->shortDescription());
		setModified(true);
	}
	else {
		// not doing anything with this object because it's the wrong type. However, it's our responsibility now, so delete it.
		delete newInfo;
	}
}

void AMActionLog3::dbLoadActionInheritedLoop(bool actionInheritedLoop){
	actionInheritedLoop_ = actionInheritedLoop;
	setModified(true);
}

#include <QDebug>
bool AMActionLog3::logUncompletedAction(const AMAction3 *uncompletedAction, int parentLogId, AMDatabase *database){
	if(uncompletedAction && !uncompletedAction->inFinalState()){
		qDebug() << "Logging uncompleted action";
		AMActionLog3 actionLog(uncompletedAction);
		actionLog.setParentId(parentLogId);
		bool success = actionLog.storeToDb(database);
		const AMListAction3 *listAction = qobject_cast<const AMListAction3*>(uncompletedAction);
		if(success && listAction){
			AMListAction3 *modifyListAction = const_cast<AMListAction3*>(listAction);
			modifyListAction->setLogActionId(actionLog.id());
		}
		return success;
	}
	return false;
}

bool AMActionLog3::updateCompletedAction(const AMAction3 *completedAction, AMDatabase *database){
	if(completedAction && completedAction->inFinalState()) {
		int infoId = completedAction->info()->id();
		if(infoId < 1){
			AMErrorMon::alert(0, AMACTIONLOG_CANNOT_UPDATE_UNSAVED_ACTIONLOG, "The actions logging system attempted to update a log action that hadn't already been saved. Please report this problem to the Acquaman developers.");
			return false;
		}
		QString infoValue = QString("%1;%2").arg(AMDbObjectSupport::s()->tableNameForClass(completedAction->info()->metaObject()->className())).arg(infoId);
		QList<int> matchingIds = database->objectsMatching(AMDbObjectSupport::s()->tableNameForClass<AMActionLog3>(), "info", QVariant(infoValue));
		if(matchingIds.count() == 0){
			AMErrorMon::alert(0, AMACTIONLOG_CANNOT_UPDATE_BAD_INDEX, QString("The actions logging system attempted to update a log action with a bad database index (%1). Please report this problem to the Acquaman developers.").arg(infoId));
			return false;
		}
		int logId = matchingIds.last();
		AMActionLog3 actionLog;
		actionLog.loadFromDb(database, logId);
		actionLog.setFromAction(completedAction);
		return actionLog.storeToDb(database);
	}
	else {
		AMErrorMon::alert(0, AMACTIONLOG_CANNOT_UPDATE_UNCOMPLETED_ACTION, QString("The actions logging system attempted to update a log action that hadn't yet finished running. Please report this problem to the Acquaman developers."));
		return false;
	}
}

bool AMActionLog3::logCompletedAction(const AMAction3 *completedAction, int parentLogId, AMDatabase *database){
	if(completedAction && completedAction->inFinalState()) {
		qDebug() << "Logging completed action";
		AMActionLog3 actionLog(completedAction);
		actionLog.setParentId(parentLogId);
		return actionLog.storeToDb(database);
	}
	else {
		return false;
	}
}

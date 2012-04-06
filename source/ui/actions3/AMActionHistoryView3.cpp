#include "AMActionHistoryView3.h"

#include "actions3/AMActionLog3.h"
#include "actions3/AMActionRunner3.h"
#include "dataman/database/AMDbObjectSupport.h"
#include "actions3/AMActionRegistry3.h"

#include "dataman/AMRun.h"
#include "dataman/AMUser.h"

#include "util/AMDateTimeUtils.h"
#include "util/AMFontSizes.h"
#include "util/AMErrorMonitor.h"

#include <QTreeView>
#include <QBoxLayout>
#include <QTimer>
#include <QScrollBar>
#include <QComboBox>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QToolButton>
#include <QMessageBox>
#include <QMenu>

#include <QStringBuilder>
#include <QPixmapCache>

#include <QHeaderView>
#include <QDebug>

// AMActionLogItem
////////////////////////////

AMActionLogItem3::AMActionLogItem3(AMDatabase *db, int id)
{
	db_ = db;
	id_ = id;
	loadedFromDb_ = false;
}


bool AMActionLogItem3::loadLogDetailsFromDb() const
{
	if(!db_)
		return false;
	if(id_ < 1)
		return false;

	QStringList columns;
	columns << "name" << "longDescription" << "iconFileName" << "startDateTime" << "endDateTime" << "finalState" << "info" << "parentId";
	QVariantList values = db_->retrieve(id_, AMDbObjectSupport::s()->tableNameForClass<AMActionLog3>(), columns);
	if(values.isEmpty())
		return false;

	shortDescription_ = values.at(0).toString();
	longDescription_  = values.at(1).toString();
	iconFileName_ = values.at(2).toString();
	startDateTime_ = values.at(3).toDateTime();
	endDateTime_ = values.at(4).toDateTime();
	finalState_ = values.at(5).toInt();
	canCopy_ = db_->retrieve(values.at(6).toString().section(';', -1).toInt(), values.at(6).toString().split(';').first(), "canCopy").toBool();
	parentId_ = values.at(7).toInt();
	loadedFromDb_ = true;

	return true;
}

QDateTime AMActionLogItem3::endDateTime() const
{
	if(!loadedFromDb_)
		loadLogDetailsFromDb();
	return endDateTime_;
}

QDateTime AMActionLogItem3::startDateTime() const
{
	if(!loadedFromDb_)
		loadLogDetailsFromDb();
	return startDateTime_;
}

QString AMActionLogItem3::shortDescription() const
{
	if(!loadedFromDb_)
		loadLogDetailsFromDb();
	return shortDescription_;
}

QString AMActionLogItem3::longDescription() const
{
	if(!loadedFromDb_)
		loadLogDetailsFromDb();
	return longDescription_;
}

int AMActionLogItem3::finalState() const
{
	if(!loadedFromDb_)
		loadLogDetailsFromDb();
	return finalState_;
}

QString AMActionLogItem3::iconFileName() const
{
	if(!loadedFromDb_)
		loadLogDetailsFromDb();
	return iconFileName_;
}

bool AMActionLogItem3::canCopy() const
{
	if (!loadedFromDb_)
		loadLogDetailsFromDb();
	return canCopy_;
}

int AMActionLogItem3::parentId() const{
	if(!loadedFromDb_)
		loadLogDetailsFromDb();
	return parentId_;
}

// AMActionHistoryModel
////////////////////////////

AMActionHistoryModel3::AMActionHistoryModel3(AMDatabase *db, QObject *parent) : QAbstractItemModel(parent)
{
	db_ = db;
	actionLogTableName_ = AMDbObjectSupport::s()->tableNameForClass<AMActionLog3>();
	visibleActionsCount_ = 0;
	maximumActionsLimit_ = 200;

	connect(&refreshFunctionCall_, SIGNAL(executed()), this, SLOT(refreshFromDb()));
	connect(&specificRefreshFunctionCall_, SIGNAL(executed()), this, SLOT(refreshSpecificIds()));

	if(db_) {
		connect(db_, SIGNAL(created(QString,int)), this, SLOT(onDatabaseItemCreated(QString,int)));
		connect(db_, SIGNAL(removed(QString,int)), this, SLOT(onDatabaseItemRemoved(QString,int)));
		connect(db_, SIGNAL(updated(QString,int)), this, SLOT(onDatabaseItemUpdated(QString,int)));
	}

	succeededIcon_ = QPixmap(":/22x22/greenCheck.png");
	cancelledIcon_ = QPixmap(":/22x22/orangeX.png");
	failedIcon_ = QPixmap(":/22x22/redCrash.png");
	unknownIcon_ = QPixmap(":/22x22/dialog-question.png");
}

void AMActionHistoryModel3::setVisibleDateTimeRange(const QDateTime &oldest, const QDateTime &newest)
{
	if(oldest == visibleRangeOldest_ && newest == visibleRangeNewest_)
		return;

	visibleRangeOldest_ = oldest;
	visibleRangeNewest_ = newest;
	refreshFunctionCall_.schedule();
}

void AMActionHistoryModel3::setMaximumActionsToDisplay(int maximumActionsCount)
{
	if(maximumActionsLimit_ == maximumActionsCount)
		return;

	maximumActionsLimit_ = maximumActionsCount;
	refreshFunctionCall_.schedule();
	/// \todo Optimization: instead of a full refresh, we could just load the additional ones we need, by specifying a LIMIT and OFFSET in the SQL query.
}

void AMActionHistoryModel3::refreshFromDb()
{
	emit modelAboutToBeRefreshed();

	// clear the model:
	clear();

	if(!db_)
		return;

	// Get all of the ids of the AMActionLogs to display. Need to order in descending (most recent first) so we get the right limit.
	QList<int> ids;
	QList<int> parentIds;
	QSqlQuery q;
	QSqlQuery q2;	// used to count the total number in the visible range, if we didn't have a limit.

	// need to setup the query differently based on whether we have oldest and newest visible range limits.

	// both newest and oldest limits:
	if(visibleRangeOldest_.isValid() && visibleRangeNewest_.isValid()) {
		q = db_->select(actionLogTableName_,
				"id,parentId",
				"endDateTime BETWEEN ? AND ? ORDER BY endDateTime DESC LIMIT ?");
		q.bindValue(0, visibleRangeOldest_);
		q.bindValue(1, visibleRangeNewest_);
		q.bindValue(2, maximumActionsLimit_);
		q2 = db_->select(actionLogTableName_,
				 "COUNT(1)",
				 "endDateTime BETWEEN ? AND ?");
		q2.bindValue(0, visibleRangeOldest_);
		q2.bindValue(1, visibleRangeNewest_);
	}
	// only an oldest limit:
	else if(visibleRangeOldest_.isValid()) {
		q = db_->select(actionLogTableName_,
				"id,parentId",
				"endDateTime >= ? ORDER BY endDateTime DESC LIMIT ?");
		q.bindValue(0, visibleRangeOldest_);
		q.bindValue(1, maximumActionsLimit_);
		q2 = db_->select(actionLogTableName_,
				 "COUNT(1)",
				 "endDateTime >= ?");
		q2.bindValue(0, visibleRangeOldest_);
	}
	// only a newest limit:
	else if(visibleRangeNewest_.isValid()) {
		q = db_->select(actionLogTableName_,
				"id,parentId",
				"endDateTime <= ? ORDER BY endDateTime DESC LIMIT ?");
		q.bindValue(0, visibleRangeNewest_);
		q.bindValue(1, maximumActionsLimit_);
		q2 = db_->select(actionLogTableName_,
				 "COUNT(1)",
				 "endDateTime <= ?");
		q2.bindValue(0, visibleRangeNewest_);
	}
	// everything:
	else {
		q = db_->select(actionLogTableName_,
				"id,parentId",
				"1 ORDER BY endDateTime DESC LIMIT ?");
		q.bindValue(0, maximumActionsLimit_);
		q2 = db_->select(actionLogTableName_,
				 "COUNT(1)");
	}

	// run the query and get the ids:
	if(!q.exec())
		AMErrorMon::alert(this, -333, "Could not execute the query to refresh the action history. Please report this problem to the Acquaman developers." % q.lastError().text());
	while(q.next()){
		ids << q.value(0).toInt();
		parentIds << q.value(1).toInt();
	}
	q.finish();

	// get the total count:
	if(q2.exec() && q2.first()) {
		visibleActionsCount_ = q2.value(0).toInt();
	}
	else {
		AMErrorMon::alert(this, -333, "Could not execute the query to refresh the action history. Please report this problem to the Acquaman developers." % q.lastError().text());
		visibleActionsCount_ = -1;	// you should never see this
	}
	q2.finish();


	if(!ids.isEmpty()) {
		// switch order
		QMap<int, int> parentIdsAndIdsAscending;
		for(int i=ids.count()-1; i>=0; i--)
			parentIdsAndIdsAscending.insertMulti(parentIds.at(i), ids.at(i));

		if(!recurseActionsLogLevelCreate(-1, parentIdsAndIdsAscending)){
			//NEM April 6th, 2012 ... problem generating internal model
		}
	}

	emit modelRefreshed();
}


void AMActionHistoryModel3::onDatabaseItemCreated(const QString &tableName, int id)
{
	if(tableName != actionLogTableName_)
		return;

	// if id is -1, there could be updates to the whole table.
	if(id < 1) {
		// qDebug() << "AMActionHistoryModel: global refresh";
		refreshFunctionCall_.schedule();
		return;
	}

	// qDebug() << "AMActionHistoryModel: precision refresh";

	// OK, this is a specific update.
	// find out if this action's endDateTime is within our visible date range
	AMActionLogItem3* item = new AMActionLogItem3(db_, id);
	if(insideVisibleDateTimeRange(item->endDateTime())) {
		emit modelAboutToBeRefreshed();
		/// \todo Ordering... This may end up at the wrong spot until a full refresh is done.  Most of the time, any actions added will be the most recent ones, however that is not guaranteed.
		appendItem(item);
		visibleActionsCount_++;
		emit modelRefreshed();
	}
	else {
		delete item;
	}
}

void AMActionHistoryModel3::onDatabaseItemUpdated(const QString &tableName, int id)
{
	if(tableName != actionLogTableName_)
		return;
	// if id is -1, there could be updates to the whole table.
	if(id < 1) {
		refreshFunctionCall_.schedule();
		return;
	}

	// OK, this is a specific update.
	idsRequiringRefresh_ << id;
	specificRefreshFunctionCall_.schedule();
}

void AMActionHistoryModel3::onDatabaseItemRemoved(const QString &tableName, int id)
{
	if(tableName != actionLogTableName_)
		return;

	// This probably won't happen very often, so for now let's just take the easy way out and do a full refresh.
	Q_UNUSED(id)
	refreshFunctionCall_.schedule();
}

void AMActionHistoryModel3::refreshSpecificIds()
{
	//NTBA David Chevrier, April 6th, 2012 ... not taking care of separating out master lists/loops
	// will contain the indexes of any rows that should be deleted.
	QList<int> rowsToDelete;

	// go through all our rows, and see if any of them need to be updated.
	for(int i=0, cc=rowCount(); i<cc; i++) {
		AMActionLogItem3* itemToRefresh = items_.at(i);
		if(idsRequiringRefresh_.contains(itemToRefresh->id())) {
			itemToRefresh->refresh();
			// If the end date time has changed to be outside of our visible range, it shouldn't be shown any more.
			if(!insideVisibleDateTimeRange(itemToRefresh->endDateTime())) {
				rowsToDelete << i;
			}
			else {
				QModelIndex changedIndexFirst = indexForLogItem(itemToRefresh);
				QModelIndex changedIndexLast = indexForLogItem(itemToRefresh).sibling(changedIndexFirst.row(), 2);
				qDebug() << "Data changed in refreshSpecificIds for " << changedIndexFirst.row() << changedIndexFirst.column() << changedIndexLast.row() << changedIndexLast.column();
				emit dataChanged(changedIndexFirst, changedIndexLast);
				//emit dataChanged(index(i, 0), index(i, 2));
			}
		}
	}

	idsRequiringRefresh_.clear();

	// Now delete any rows that shouldn't be there anymore. Need to go backwards so that indexes don't change as we delete.
	if(!rowsToDelete.isEmpty()) {
		emit modelAboutToBeRefreshed();
		for(int i=rowsToDelete.count()-1; i>=0; i--) {
			removeRow(rowsToDelete.at(i));
			visibleActionsCount_--;
		}
		emit modelRefreshed();
	}
}

bool AMActionHistoryModel3::insideVisibleDateTimeRange(const QDateTime &dateTime)
{
	if(visibleRangeOldest_.isValid() && dateTime < visibleRangeOldest_)
		return false;
	if(visibleRangeNewest_.isValid() && dateTime > visibleRangeNewest_)
		return false;

	return true;
}




AMActionHistoryView3::AMActionHistoryView3(AMActionRunner3 *actionRunner, AMDatabase *db, QWidget *parent) : QWidget(parent)
{
	actionRunner_ = actionRunner;
	db_ = db;

	model_ = new AMActionHistoryModel3(db_, this);
	model_->setMaximumActionsToDisplay(100);
	//QDateTime fourHoursAgo = QDateTime::currentDateTime().addSecs(-4*60*60);
	//model_->setVisibleDateTimeRange(fourHoursAgo);
	QDateTime everAgo = QDateTime();
	model_->setVisibleDateTimeRange(everAgo);

	// Setup UI
	//////////////////////
	isCollapsed_ = false;

	QFrame* topFrame = new QFrame();
	topFrame->setObjectName("topFrame");
	topFrame->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	topFrame->setStyleSheet("QFrame#topFrame {\nbackground-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(89, 89, 89, 255), stop:0.494444 rgba(89, 89, 89, 255), stop:0.5 rgba(58, 58, 58, 255), stop:1 rgba(58, 58, 58, 255));\nborder-bottom: 1px solid black;\n}");
	QHBoxLayout* hl = new QHBoxLayout(topFrame);
	hl->setSpacing(0);
	hl->setContentsMargins(6, 2, 12, 1);

	QToolButton* hideButton = new QToolButton();
	hideButton->setStyleSheet("QToolButton {\nborder: none;\nbackground-color: rgba(255, 255, 255, 0);\ncolor: white;\n image: url(:/22x22/arrow-white-down.png)} \nQToolButton::checked {\n	image: url(:/22x22/arrow-white-right.png);\n}\n");
	hideButton->setCheckable(true);
	hideButton->setChecked(false);
	hl->addWidget(hideButton);
	hl->addSpacing(10);

	QVBoxLayout* vl2 = new QVBoxLayout();
	vl2->setContentsMargins(0,0,0,0);
	vl2->setSpacing(0);
	headerTitle_ = new QLabel("Completed Actions");
	headerTitle_->setStyleSheet("color: white;\nfont: " AM_FONT_XLARGE_ "pt \"Lucida Grande\"");
	headerSubTitle_ = new QLabel("No actions to show.");
	headerSubTitle_->setStyleSheet("color: rgb(204, 204, 204);\nfont: " AM_FONT_REGULAR_ "pt \"Lucida Grande\"");
	vl2->addWidget(headerTitle_);
	vl2->addWidget(headerSubTitle_);
	hl->addLayout(vl2);
	hl->addSpacing(10);

	showMoreActionsButton_ = new QPushButton("Show More");
	hl->addWidget(showMoreActionsButton_);
	hl->addSpacing(40);
	hl->addStretch(0);

	reRunActionButton_ = new QPushButton("Re-run this action");
	reRunActionButton_->setDisabled(true);
	hl->addWidget(reRunActionButton_);
	hl->addSpacing(40);
	QLabel* showLabel = new QLabel("Show: ");
	showLabel->setStyleSheet("color: rgb(204, 204, 204);\nfont: " AM_FONT_REGULAR_ "pt \"Lucida Grande\"");
	hl->addWidget(showLabel);
	rangeComboBox_ = new QComboBox();
	rangeComboBox_->addItem("Last Hour");
	rangeComboBox_->addItem("Last 4 Hours");
	rangeComboBox_->addItem("Last Day");
	rangeComboBox_->addItem("This Run");
	rangeComboBox_->addItem("Ever");
	//rangeComboBox_->setCurrentIndex(1);
	rangeComboBox_->setCurrentIndex(4);
	hl->addWidget(rangeComboBox_);

	treeView_ = new QTreeView();
	treeView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	treeView_->setModel(model_);
	treeView_->setSelectionBehavior(QAbstractItemView::SelectRows);
	treeView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
	treeView_->setAlternatingRowColors(true);
	//treeView_->setRootIsDecorated(false);	// gets rid of indentation at top level.
	//treeView_->setHeaderHidden(true);
	treeView_->scrollToBottom();
	scrolledToBottom_ = true;
	treeView_->header()->setStretchLastSection(false);
	treeView_->header()->setResizeMode(0, QHeaderView::Stretch);
	treeView_->header()->setResizeMode(1, QHeaderView::Fixed);
	treeView_->header()->setResizeMode(2, QHeaderView::Fixed);
	treeView_->header()->setResizeMode(3, QHeaderView::Fixed);
	treeView_->header()->resizeSection(1, 48);
	treeView_->header()->resizeSection(2, 180);
	treeView_->header()->resizeSection(3, 160);
	treeView_->setAttribute(Qt::WA_MacShowFocusRect, false);

	QVBoxLayout* vl = new QVBoxLayout(this);
	vl->setSpacing(0);
	vl->setContentsMargins(0,0,0,0);
	vl->addWidget(topFrame);
	vl->addWidget(treeView_);

	treeView_->setContextMenuPolicy(Qt::CustomContextMenu);

	// Make connections:
	////////////////

	connect(hideButton, SIGNAL(toggled(bool)), this, SLOT(collapse(bool)));

	connect(model_, SIGNAL(modelAboutToBeRefreshed()), this, SLOT(onModelAboutToBeRefreshed()));
	connect(model_, SIGNAL(modelRefreshed()), this, SLOT(onModelRefreshed()));

	connect(rangeComboBox_, SIGNAL(activated(int)), this, SLOT(onRangeComboBoxActivated(int)));
	connect(showMoreActionsButton_, SIGNAL(clicked()), this, SLOT(onShowMoreActionsButtonClicked()));

	connect(reRunActionButton_, SIGNAL(clicked()), this, SLOT(onReRunActionButtonClicked()));
	connect(treeView_->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onSelectionChanged()));

	connect(treeView_, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onCustomContextMenuRequested(QPoint)));
}

void AMActionHistoryView3::onCustomContextMenuRequested(QPoint point)
{
	QMenu popup(treeView_);
	popup.addAction(treeView_->selectionModel()->selectedRows().size() == 1 ? "Re-run this action" : "Re-run these actions");

	QAction *temp = popup.exec(treeView_->mapToGlobal(point));

	if (temp && (temp->text() == "Re-run this action" || temp->text() == "Re-run these actions"))
		onReRunActionButtonClicked();
}

void AMActionHistoryView3::onModelAboutToBeRefreshed()
{
	// we're scrolled to the bottom if the vertical scroll bar value is at maximum. Remember that so we can go back to the bottom after the refresh.
	scrolledToBottom_ = (treeView_->verticalScrollBar()->value() == treeView_->verticalScrollBar()->maximum());
	// qDebug() << "Scrolled to bottom:" << scrolledToBottom_;
}

void AMActionHistoryView3::onModelRefreshed()
{
	// if we were scrolled to the most recent (bottom) action before the refresh, let's scroll back there.
	if(scrolledToBottom_) {
		// doesn't work: treeView_->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);

		// also doesn't work... Sometimes only goes most of the way there.
		treeView_->scrollToBottom();
		// looks like if the tree view is not visible at this time, this won't take effect. Adding another check in our showEvent() to scroll the treeView_ to the bottom if should be scrolledToBottom_
		// qDebug() << "ScrollING to bottom";
	}

	// update subtitle text with how many, of how many total.
	QString filterText;
	switch(rangeComboBox_->currentIndex()) {
	case 0: filterText = " in the last hour."; break;
	case 1: filterText = " in the last 4 hours."; break;
	case 2: filterText = " in the last day."; break;
	case 3: filterText = " in this run."; break;
	case 4: filterText = " ever done."; break;
	}

	int actionsShown = model_->rowCount();
	int totalActions = model_->visibleActionsCount();
	if(actionsShown == 0) {
		showMoreActionsButton_->hide();
		headerSubTitle_->setText("No actions" % filterText);
	}
	else if(actionsShown == totalActions) {
		showMoreActionsButton_->hide();
		headerSubTitle_->setText("Showing " % QString::number(actionsShown) % " actions" % filterText);
	}
	else {
		showMoreActionsButton_->show();
		headerSubTitle_->setText("Showing " % QString::number(actionsShown) % " of " % QString::number(totalActions) % " actions" % filterText);
	}
}

void AMActionHistoryView3::onShowMoreActionsButtonClicked()
{
	model_->setMaximumActionsToDisplay(model_->maximumActionsToDisplay()*2);
}

void AMActionHistoryView3::onRangeComboBoxActivated(int rangeIndex)
{
	QDateTime oldest = QDateTime::currentDateTime();

	switch(rangeIndex) {
	case 0: // last hour
		oldest = oldest.addSecs(-60*60);
		break;
	case 1: // last 4 hours
		oldest = oldest.addSecs(-4*60*60);
		break;
	case 2: // last day
		oldest = oldest.addDays(-1);
		break;
	case 3: // this run
	{
		AMRun currentRun;
		currentRun.loadFromDb(AMDatabase::database("user"), AMUser::user()->currentRunId());
		oldest = currentRun.dateTime();
	}
		break;
	case 4: // ever
	default:
		oldest = QDateTime();
		break;
	}

	model_->setVisibleDateTimeRange(oldest);
}

QModelIndex AMActionHistoryModel3::index(int row, int column, const QModelIndex &parent) const
{
	// Return bad index if it's outside the column range
	if(column < 0 || column > 4)
		return QModelIndex();

	// if no parent is top level
	if(!parent.isValid()){
		//qDebug() << "Looking at no parent, must be top level";
		if(row < 0){// what about other limit?
			//NEM April 5th, 2012 ... invalid?
			return QModelIndex();
		}
		int foundTopLevels = 0;
		for(int x = 0; x < items_.count(); x++){
			if(items_.at(x)->parentId() == -1)
				foundTopLevels++;
			if(foundTopLevels == row+1)
				return createIndex(row, column, items_.at(x));
		}
		//NEM April 5th, 2012 ... ran out of list to check?
		return QModelIndex();
	}
	// if parent then it's sub-level
	else{
		//qDebug() << "Looking at something sub-level";
		AMActionLogItem3 *parentLogItem = logItem(parent);
		if(!parentLogItem){
			//NEM April 5th, 2012 ... returned bad parent pointer
			return QModelIndex();
		}
		int parentIndexInList = items_.indexOf(parentLogItem);
		int siblingsFound = 0;
		for(int x = parentIndexInList; x < items_.count(); x++){
			if(items_.at(x)->parentId() == parentLogItem->id())
				siblingsFound++;
			if(siblingsFound == row+1)
				return createIndex(row, column, items_.at(x));
		}
		// NEM April 5th, 2012 ... ran out of list to check?
		return QModelIndex();
	}
}

QModelIndex AMActionHistoryModel3::parent(const QModelIndex &child) const
{
	if(!child.isValid())
		return QModelIndex();

	AMActionLogItem3 *childLogItem = logItem(child);
	if(!childLogItem)
		return QModelIndex();

	for(int x = 0; x < items_.count(); x++)
		if(items_.at(x)->id() == childLogItem->parentId())
			return indexForLogItem(items_.at(x));

	//NEM April 5th, 2012 ... couldn't find parent
	return QModelIndex();
}

int AMActionHistoryModel3::rowCount(const QModelIndex &parent) const
{
	// top level
	if(!parent.isValid()){
		int topLevelsFound = 0;
		for(int x = 0; x < items_.count(); x++)
			if(items_.at(x)->parentId() == -1)
				topLevelsFound++;

		return topLevelsFound;
	}
	// some sub-level
	else{
		AMActionLogItem3 *parentLogItem = logItem(parent);
		if(!parentLogItem){
			//NEM April 5th, 2012 ... returned bad parent pointer
			return 0;
		}
		int childrenFound = 0;
		int parentIndexInList = items_.indexOf(parentLogItem);
		for(int x = parentIndexInList; x < items_.count(); x++)//might optimize to figure a good ending point
			if(items_.at(x)->parentId() == parentLogItem->id())
				childrenFound++;

		return childrenFound;
	}
}

int AMActionHistoryModel3::columnCount(const QModelIndex &parent) const
{
	if(!parent.isValid())
		return 4;
	else
		return 0;
}

Qt::ItemFlags AMActionHistoryModel3::flags(const QModelIndex &index) const
{
	AMActionLogItem3 *item = logItem(index);
	if (item && item->canCopy())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return Qt::ItemIsEnabled;
}

QVariant AMActionHistoryModel3::data(const QModelIndex &index, int role) const
{
	AMActionLogItem3 *item = logItem(index);
	if(!item){
		//NEM April 5th, 2012 ... no logItem at index
		return QVariant();
	}

	if(role == Qt::DisplayRole) {
		switch(index.column()) {
		case 0: return item->shortDescription();
		case 1: return QVariant();
		case 2: return AMDateTimeUtils::prettyDateTime(item->endDateTime());
		case 3: return AMDateTimeUtils::prettyDuration(item->startDateTime(), item->endDateTime());
		}
	}
	else if(role == Qt::DecorationRole) {
		// column 0: return the action icon.
		if(index.column() == 0) {
			QString iconFileName = item->iconFileName();
			if(iconFileName.isEmpty())
				iconFileName = ":/64x64/generic-action.png";
			QPixmap p;
			if(QPixmapCache::find("AMActionLogItemIcon" % iconFileName, &p))
				return p;
			else {
				p.load(iconFileName);
				p = p.scaledToHeight(22, Qt::SmoothTransformation);
				QPixmapCache::insert("AMActionLogItemIcon" % iconFileName, p);
				return p;
			}
		}
		// column 1: return the status icon
		else if(index.column() == 1) {
			switch(item->finalState()) {
			case AMAction3::Succeeded: return succeededIcon_;
			case AMAction3::Cancelled: return cancelledIcon_;
			case AMAction3::Failed: return failedIcon_;
			default: return unknownIcon_;
			}
		}
		else
			return QVariant();
	}
	else if(role == Qt::SizeHintRole) {
		return QSize(-1, 32);
	}
	else if(role == Qt::ToolTipRole) {
		if(index.column() == 0) {
			return item->longDescription();
		}
		else if(index.column() == 1) {
			switch(item->finalState()) {
			case AMAction3::Succeeded: return "Succeeded";
			case AMAction3::Cancelled: return "Cancelled";
			case AMAction3::Failed: return "Failed";
			default: return "[?]";
			}
		}
	}
	else if(role == Qt::BackgroundRole) {
		switch(item->finalState()) {
		case AMAction3::Succeeded:
			return QColor(126, 255, 106);// light green
		case AMAction3::Cancelled:
			return QColor(255, 176, 106);// light orange
		case AMAction3::Failed:
			return QColor(255, 104, 106);// light red
		default:
			return QVariant();
		}
	}

	return QVariant();
}

QVariant AMActionHistoryModel3::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch(section) {
		case 0: return QString("Action");
		case 1: return QString("Status");
		case 2: return QString("Finished");
		case 3: return QString("Duration");
		default: return QVariant();
		}
	}
	return QVariant();
}

bool AMActionHistoryModel3::hasChildren(const QModelIndex &parent) const{
	if(!parent.isValid())
		return true; // top level must have children
	else{
		AMActionLogItem3 *parentLogItem = logItem(parent);
		if(!parentLogItem){
			//NEM April 5th, 2012 ... returned bad parent pointer
			return false;
		}
		int parentIndexInList = items_.indexOf(parentLogItem);
		for(int x = parentIndexInList; x < items_.count(); x++)//might optimize to figure a good ending point
			if(items_.at(x)->parentId() == parentLogItem->id())
				return true;

		return false;
	}
}

void AMActionHistoryModel3::appendItem(AMActionLogItem3 *item)
{
	/*
	int currentCount = items_.count();
	beginInsertRows(QModelIndex(), currentCount, currentCount);
	items_.append(item);
	endInsertRows();
	*/
	QModelIndex parentIndex = indexForLogItem(item).parent();
	int parentRowCount = rowCount(parentIndex);
	beginInsertRows(parentIndex, parentRowCount, parentRowCount);
	items_.append(item);
	endInsertRows();
}

void AMActionHistoryModel3::clear()
{
	qDebug() << "\n\nYO YO, A CLEAR IS BEING ATTEMPTED";
	if(items_.isEmpty()){
		qDebug() << "Nothing to clear\n\n";
		return;
	}
	qDebug() << "Something to clear\n\n";
	if(!recurseActionsLogLevelClear(QModelIndex())){
		//NEM April 6th, 2012 ... something went wrong clearing
	}
	/*
	if(items_.isEmpty())
		return;

	int currentCount = items_.count();
	beginRemoveRows(QModelIndex(), 0, currentCount-1);
	qDeleteAll(items_);
	items_.clear();
	endRemoveRows();
	*/
}

bool AMActionHistoryModel3::recurseActionsLogLevelCreate(int parentId, QMap<int, int> parentIdsAndIds){
	QList<int> childIdsInReverse = parentIdsAndIds.values(parentId);
	if(childIdsInReverse.count() == 0)
		return true;
	int indexOfParent;
	if(!items_.count() == 0){
		for(int x = 0; x < items_.count(); x++)
			if(items_.at(x)->id() == parentId)
				indexOfParent = x+1;
	}
	else
		indexOfParent = 0;

	// Place this level of items in reverse order
	for(int x = 0; x < childIdsInReverse.count(); x++)
		items_.insert(indexOfParent, new AMActionLogItem3(db_, childIdsInReverse.at(x)));

	// Ask to place the children for each child in order
	bool success = true;
	for(int x = childIdsInReverse.count()-1; x >= 0; x--)
		success &= recurseActionsLogLevelCreate(childIdsInReverse.at(x), parentIdsAndIds);
	return success;
}

bool AMActionHistoryModel3::recurseActionsLogLevelClear(QModelIndex parentIndex){
	int childrenCount = rowCount(parentIndex);
	if(childrenCount == 0)
		return true;
	bool success = true;
	for(int x = childrenCount-1; x >= 0; x--)
		success &= recurseActionsLogLevelClear(index(x, 0, parentIndex));

	AMActionLogItem3 *item;
	beginRemoveRows(parentIndex, 0, childrenCount);
	for(int x = childrenCount-1; x >= 0; x--){
		item = logItem(index(x, 0, parentIndex));
		success &= items_.removeOne(item);
		delete item;
	}
	endRemoveRows();

	return success;
}

AMActionHistoryModel3::~AMActionHistoryModel3() {
	clear();
}

void AMActionHistoryView3::onReRunActionButtonClicked()
{
	// we need a valid action runner to re-queue actions. This button shouldn't be enabled if there is no actionRunner_, but just in case:
	if(!actionRunner_)
		return;

	// go through all selected rows.
	foreach(QModelIndex i, treeView_->selectionModel()->selectedRows(0)) {
		AMActionLogItem3* item = model_->logItem(i);
		if(!item)
			continue;
		// load the full actionLog.
		AMActionLog3 actionLog;
		if(!actionLog.loadFromDb(item->database(), item->id())) {
			AMErrorMon::report(AMErrorReport(this, AMErrorReport::Debug, -57, "Could not load the action log for id " % QString::number(item->id()) % " from the database. Please report this problem to the Acquaman developers."));
			continue;
		}
		if(!actionLog.info()) {
			AMErrorMon::report(AMErrorReport(this, AMErrorReport::Debug, -58, "Could not load the action info for the action log with id " % QString::number(item->id()) % " from the database. It is likely because the action info class hasn't been registered with the database system. Please report this problem to the Acquaman developers."));
			continue;
		}
		// make a copy of the AMActionInfo.
		AMActionInfo3* info = actionLog.info()->createCopy();

		// make an action (assuming the actionInfo is registered with a corresponding action)
		AMAction3* action = AMActionRegistry3::s()->createActionFromInfo(info);
		if(!action) {
			QMessageBox::warning(this, "Cannot re-run this action", "Could not re-run this action because running the '" % info->typeDescription() %  "' action isn't enabled for your beamline's version of Acquaman. If you don't think this should be the case, please report this to the Acquaman developers.");
			AMErrorMon::report(AMErrorReport(this, AMErrorReport::Alert, -59, "Could not re-run this action because running '" % actionLog.name() % "' isn't enabled for your beamline's version of Acquaman. If you don't think this should be the case, please report this to the Acquaman developers."));
			delete info;
			continue;
		}
		actionRunner_->addActionToQueue(action);
	}
}

void AMActionHistoryView3::onSelectionChanged()
{
	// can only re-run actions if we have a valid actionRunner_
	if(actionRunner_) {
		int numSelected = treeView_->selectionModel()->selectedRows().count();
		if(numSelected == 0) {
			reRunActionButton_->setDisabled(true);
		}
		else if(numSelected == 1) {
			reRunActionButton_->setEnabled(true);
			reRunActionButton_->setText("Re-run this action");
		}
		else {
			reRunActionButton_->setEnabled(true);
			reRunActionButton_->setText("Re-run these actions");
		}
	}
}

AMActionLogItem3 * AMActionHistoryModel3::logItem(const QModelIndex &index) const
{
	if(!index.isValid())
		return 0;
	return static_cast<AMActionLogItem3*>(index.internalPointer());
}

QModelIndex AMActionHistoryModel3::indexForLogItem(AMActionLogItem3 *logItem) const{
	if(!logItem)
		return QModelIndex();

	if(logItem->parentId() == -1){
		// top level logItem
		int topLevelBefore = 0;
		for(int x = 0; x < items_.count(); x++){
			if(items_.at(x) == logItem){
				return createIndex(topLevelBefore, 0, logItem);
			}
			if(items_.at(x)->parentId() == -1)
				topLevelBefore++;
		}
		//NEM April 5th, 2012 ... logItem not found
		return QModelIndex();
	}
	else{
		// we have a parent logitem
		bool foundParent = false;
		int backwardsIndex = items_.indexOf(logItem); //find myself
		int numberOfSiblings = 0;
		while(!foundParent){
			if(backwardsIndex == 0){
				//NEM April 5th, 2012 ... logItem couldn't find parent
				return QModelIndex();
			}
			backwardsIndex--;
			if(items_.at(backwardsIndex)->id() == logItem->parentId()) //mark parent found if id matches
				foundParent = true;
			else if(items_.at(backwardsIndex)->parentId() == logItem->parentId())
				numberOfSiblings++; //or incremement the number of your siblings
		}
		return createIndex(numberOfSiblings, 0, logItem);
	}
}

void AMActionHistoryView3::showEvent(QShowEvent *e)
{
	QWidget::showEvent(e);

	if(scrolledToBottom_)
		treeView_->scrollToBottom();
}


void AMActionHistoryView3::collapse(bool doCollapse)
{
	bool wasCollapsed = isCollapsed();
	treeView_->setHidden(doCollapse);
	isCollapsed_ = doCollapse;

	if(isCollapsed_ != wasCollapsed)
		emit collapsed(doCollapse);
}



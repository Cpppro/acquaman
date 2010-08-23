#include "AMSamplePlateView.h"

AMSamplePlateView::AMSamplePlateView(QString title, QWidget *parent) :
	QGroupBox(title, parent)
{
	//samplePlate_ = new AMSamplePlate(this);
	manipulator_ = NULL;

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	plateNameLabel_ = new QLabel(samplePlate_.plateName());
	plateNameLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	existingPlates_ = new QComboBox();
	existingPlates_->addItem("Load Existing");

	QSqlQuery q2 = AMDatabase::userdb()->query();
	q2.prepare(QString("SELECT id,name,createTime FROM %1 ORDER BY createTime DESC").arg(AMDatabaseDefinition::samplePlateTableName()));
	q2.exec();
	int id;
	QString name;
	QDateTime createTime;
	while(q2.next()) {
		id = q2.value(0).toInt();
		name = q2.value(1).toString();
		createTime = q2.value(2).toDateTime();
		existingPlates_->insertItem(1, name);
		existingPlates_->setItemData(1, id, AM::IdRole);
		existingPlates_->setItemData(1, createTime, AM::DateTimeRole);
	}
	connect(existingPlates_, SIGNAL(currentIndexChanged(int)), this, SLOT(onLoadExistingPlate(int)));

	sampleTableModel_ = new QStandardItemModel();
	QStandardItem *tmpItem;
	QSqlQuery q = AMDatabase::userdb()->query();
	q.prepare(QString("SELECT id,name,dateTime FROM %1 ORDER BY dateTime DESC").arg(AMDatabaseDefinition::sampleTableName()));
	q.exec();
	QDateTime dateTime;
	while(q.next()){
		id = q.value(0).toInt();
		name = q.value(1).toString();
		dateTime = q.value(2).toDateTime();
		tmpItem = new QStandardItem(name);
		tmpItem->setData(id, AM::IdRole);
		tmpItem->setData(dateTime, AM::DateTimeRole);
		sampleTableModel_->setItem(sampleTableModel_->rowCount(), tmpItem);
	}

	for(int x = 0; x < sampleTableModel_->rowCount(); x++){
		qDebug() << x << sampleTableModel_->item(x, 0)->text() << sampleTableModel_->item(x, 0)->data(AM::IdRole) << sampleTableModel_->item(x, 0)->data(AM::DateTimeRole);
	}

	sampleListView_ = new AMSampleListView(&samplePlate_, sampleTableModel_);
	sampleListView_->setManipulator(manipulator_);
	//connect(this, SIGNAL(loadExistingPlate(int)), sampleListView_, SLOT(onLoadExistingPlate(int)));
	//sampleListView_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
	vl_ = new QVBoxLayout();
	vl_->setAlignment(Qt::AlignTop);
	vl_->addWidget(plateNameLabel_);
	vl_->addWidget(existingPlates_);
	vl_->addWidget(sampleListView_);
	setLayout(vl_);
}

void AMSamplePlateView::setManipulator(AMControlSet *manipulator){
	manipulator_ = manipulator;
	sampleListView_->setManipulator(manipulator_);
}

void AMSamplePlateView::onLoadExistingPlate(int index){
	int id = existingPlates_->itemData(index, AM::IdRole).toInt();
	if(id == 0)
		return;
	samplePlate_.loadFromDb(AMDatabase::userdb(), id);
	qDebug() << "Sample plate has " << samplePlate_.count() << " positions.";
}

AMSampleListView::AMSampleListView(AMSamplePlate *samplePlate, QStandardItemModel *sampleTableModel, QWidget *parent) :
		QFrame(parent)
{
	samplePlate_ = samplePlate;
	sampleTableModel_ = sampleTableModel;
	connect(samplePlate_, SIGNAL(samplePositionChanged(int)), this, SLOT(onSamplePositionChanged(int)));
	connect(samplePlate_, SIGNAL(samplePositionAdded(int)), this, SLOT(onSamplePositionAdded(int)));
	connect(samplePlate_, SIGNAL(samplePositionRemoved(int)), this, SLOT(onSamplePositionRemoved(int)));

	saf_ = new QFrame();
	il_ = new QVBoxLayout();
	il_->setAlignment(Qt::AlignTop);
	saf_->setLayout(il_);
	saf_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	sa_ = new QScrollArea();
	sa_->setWidget(saf_);
	sa_->setWidgetResizable(true);
	sa_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QVBoxLayout *ol = new QVBoxLayout();
	ol->addWidget(sa_);
	setLayout(ol);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	adder_ = new QPushButton(QIcon(":/add.png"), "Mark a New Sample Position ...");
	connect(adder_, SIGNAL(clicked()), this, SLOT(addNewSampleToPlate()));
	il_->addWidget(adder_);
}

void AMSampleListView::addNewSampleToPlate(){
	if(!manipulator_)
		return;

	QSqlQuery q = AMDatabase::userdb()->query();
	q.prepare(QString("SELECT name FROM %1 WHERE name LIKE 'Sample %'").arg(AMDatabaseDefinition::sampleTableName()));
	q.exec();
	int index;
	int max = 1;
	bool convOK;
	while(q.next()) {
		index = q.value(0).toString().remove("Sample ").toInt(&convOK);
		qDebug() << "testing index " << index;
		if(index > max)
			max = index;
	}

	QString tmpStr;
	tmpStr.setNum(max+1);
	AMSample *tmpSample = new AMSample("Sample "+tmpStr, this);
	tmpSample->storeToDb(AMDatabase::userdb());
	AMControlSetInfo *tmpPosition = NULL;
	tmpPosition = new AMControlSetInfo(manipulator_->info(), this);
	tmpPosition->storeToDb(AMDatabase::userdb());
	qDebug() << "tmpPosition with id of " << tmpPosition->id();
	samplePlate_->appendSamplePosition(tmpSample, tmpPosition);
	samplePlate_->storeToDb(AMDatabase::userdb());
}

void AMSampleListView::setManipulator(AMControlSet *manipulator){
	manipulator_ = manipulator;
}

void AMSampleListView::onSamplePositionChanged(int index){
	qDebug() << "Claims change at " << index;
}

void AMSampleListView::onSamplePositionAdded(int index){
	qDebug() << "Claims added at " << index;
	if(!manipulator_ || !sampleTableModel_)
		return;
	AMSamplePositionItemView *tmpSPIView = new AMSamplePositionItemView(samplePlate_->samplePositionAt(index), sampleTableModel_, manipulator_, index+1);
	il_->insertWidget(index, tmpSPIView, 0, Qt::AlignTop);
	tmpSPIView->setFocus();
	sa_->ensureVisible(0, 0);
}

void AMSampleListView::onSamplePositionRemoved(int index){
	qDebug() << "Claims removed at " << index;
	if(il_->itemAt(index) == 0)
		return;
	AMSamplePositionItemView *tmpSPIView = (AMSamplePositionItemView*)il_->itemAt(index)->widget();
	qDebug() << "Actually deleting " << index;
	delete tmpSPIView;
}

QSize AMSampleListView::sizeHint() const{
	QSize tmpSize = QFrame::sizeHint();
	int newWidth;
	if(sa_->verticalScrollBar() && sa_->verticalScrollBar()->isVisible())
		newWidth = 1.1*(tmpSize.width()+sa_->verticalScrollBar()->width());
	else
		newWidth = tmpSize.width()*1.1;
	tmpSize.setWidth(newWidth);
	qDebug() << "Width will be " << tmpSize.width();
	return tmpSize;
}

AMSamplePositionItemView::AMSamplePositionItemView(AMSamplePosition *samplePosition, QStandardItemModel *sampleTableModel, AMControlSet *manipulator, int index, QWidget *parent) :
		QFrame(parent)
{
	index_ = index;
	inFocus_ = false;
	setFocusPolicy(Qt::StrongFocus);
	samplePosition_ = samplePosition;
	manipulator_ = manipulator;
	sampleTableModel_ = sampleTableModel;

	vl_ = NULL;
	hl_ = NULL;
	indexLabel_ = NULL;
	sampleBox_ = NULL;
	positionLabel_ = NULL;
	savePositionButton_ = NULL;
	recallPositionButton_ = NULL;

	setLineWidth(1);
	setFrameStyle(QFrame::StyledPanel);

	onSamplePositionUpdate(-1);
	if(samplePosition_)
		connect(samplePosition_, SIGNAL(positionValuesChanged(int)), this, SLOT(onSamplePositionUpdate(int)));
}

int AMSamplePositionItemView::index(){
	return index_;
}

AMControlSet* AMSamplePositionItemView::manipulator(){
	return manipulator_;
}

void AMSamplePositionItemView::setIndex(int index){
	index_ = index;
}

void AMSamplePositionItemView::setManipulator(AMControlSet *manipulator){
	manipulator_ = manipulator;
}

bool AMSamplePositionItemView::onSavePositionClicked(){
	if(!manipulator_)
		return false;
	samplePosition_->position()->copyFrom(manipulator_->info());
	samplePosition_->position()->storeToDb(AMDatabase::userdb());
	return true;
}

bool AMSamplePositionItemView::onRecallPositionClicked(){
	if(!manipulator_)
		return false;
	manipulator_->setFromInfo(samplePosition_->position());
	return true;
}

bool AMSamplePositionItemView::onSampleNameChanged(){
	samplePosition_->sample()->setName(sampleBox_->currentText());
	return samplePosition_->sample()->storeToDb(AMDatabase::userdb());
}

void AMSamplePositionItemView::updateLook(){
	if(inFocus_)
		setFrameStyle(QFrame::Box);
	if(inFocus_)
		setStyleSheet("AMSamplePositionItemView { background : rgb(194, 210, 215) }");
	else{
		setStyleSheet("AMSamplePositionItemView { background : rgb(230, 222, 214) }");
		setFrameStyle(QFrame::StyledPanel);
	}
	if(inFocus_ && (vl_->indexOf(positionLabel_) == -1) ){
		vl_->addWidget(positionLabel_);
		positionLabel_->show();
	}
	else if(!inFocus_ && (vl_->indexOf(positionLabel_) != -1) ){
		vl_->removeWidget(positionLabel_);
		positionLabel_->hide();
	}
}

void AMSamplePositionItemView::onSamplePositionUpdate(int index){
	if(!samplePosition_)
		return;
	if(!vl_){
		vl_ = new QVBoxLayout();
		setLayout(vl_);
	}
	if(!hl_){
		hl_ = new QHBoxLayout();
		vl_->addLayout(hl_);
	}
	if(!indexLabel_){
		indexLabel_ = new QLabel();
		indexLabel_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
		hl_->addWidget(indexLabel_, 0, Qt::AlignLeft);
	}
	if(!sampleBox_){
		sampleBox_ = new QComboBox();
		sampleBox_->setModel(sampleTableModel_);
		sampleBox_->setEditable(true);
		sampleBox_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
		setFocusProxy(sampleBox_);
		/*
		QSqlQuery q = AMDatabase::userdb()->query();
		qDebug() << "Before db query";
//		q.prepare(QString("SELECT id,name,dateTime FROM %1 ORDER BY dateTime ASC").arg(AMDatabaseDefinition::sampleTableName()));
		q.prepare(QString("SELECT id,name,dateTime FROM %1").arg(AMDatabaseDefinition::sampleTableName()));
		q.exec();
		qDebug() << "After db query";
		int id;
		QString name;
		QDateTime dateTime;
		while(q.next()) {
			id = q.value(0).toInt();
			name = q.value(1).toString();
			dateTime = q.value(2).toDateTime();
			sampleBox_->insertItem(0, name);
			sampleBox_->setItemData(0, id, AM::IdRole);
			sampleBox_->setItemData(0, dateTime, AM::DateTimeRole);
		}
		*/
		connect(sampleBox_->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onSampleNameChanged()));
		hl_->addWidget(sampleBox_, 3, Qt::AlignLeft);
		sampleBox_->setMaxVisibleItems(5);
		qDebug() << "Setting max visible to " << sampleBox_->maxVisibleItems();
	}
	if(!positionLabel_){
		positionLabel_ = new QLabel(this);
		positionLabel_->hide();
	}
	if(!savePositionButton_){
		savePositionButton_ = new QPushButton(QIcon(":/save.png"), "");
		savePositionButton_->setToolTip("Save Current Position");
		connect(savePositionButton_, SIGNAL(clicked()), this, SLOT(onSavePositionClicked()));
		hl_->addWidget(savePositionButton_, 0, Qt::AlignRight);
	}
	if(!recallPositionButton_){
		recallPositionButton_ = new QPushButton(QIcon(":/go-next.png"), "");
		recallPositionButton_->setToolTip("Move to Saved Position");
		connect(recallPositionButton_, SIGNAL(clicked()), this, SLOT(onRecallPositionClicked()));
		hl_->addWidget(recallPositionButton_, 0, Qt::AlignRight);
	}
	QString tmpStr;
	tmpStr.setNum(index_);
	indexLabel_->setText(tmpStr+". ");
//	sampleBox_->insertItem(0, samplePosition_->sample()->name());
	sampleBox_->setCurrentIndex( sampleBox_->findText(samplePosition_->sample()->name()) );
	sampleBox_->lineEdit()->selectAll();

	QString positionText;
	qDebug() << "Looping position info";
	for(int x = 0; x < samplePosition_->position()->count(); x++){
		qDebug() << samplePosition_->position()->nameAt(x) << samplePosition_->position()->valueAt(x);
		tmpStr.setNum(samplePosition_->position()->valueAt(x));
		tmpStr.prepend(": ");
		tmpStr.prepend(samplePosition_->position()->nameAt(x));
		tmpStr.append("\n");
		positionText.append(tmpStr);
	}
	positionLabel_->setText(positionText);
}

void AMSamplePositionItemView::defocusItem(){
	inFocus_ = false;
	updateLook();
}

void AMSamplePositionItemView::mousePressEvent(QMouseEvent *event){
	if (event->button() != Qt::LeftButton) {
		event->ignore();
		return;
	}
	if(inFocus_)
		defocusItem();
	else{
		inFocus_ = true;
		updateLook();
		//emit focusRequested(action_);
	}
}

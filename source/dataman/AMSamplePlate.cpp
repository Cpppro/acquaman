#include "AMSamplePlate.h"

AMSamplePlate::AMSamplePlate(QObject *parent) :
	QObject(parent)
{
	insertRowLatch = -1;
	userName_ = "SGM";
	timeName_ = "loaded "+QDateTime::currentDateTime().toString("MMM d yyyy, h:m ap");
	samples_ = NULL;
	setupModel();
}

AMSamplePlateModel* AMSamplePlate::model(){
	return samples_;
}

QString AMSamplePlate::plateName() const{
	return userName_+" "+timeName_;
}

int AMSamplePlate::count(){
	return samples_->rowCount(QModelIndex());
}

AMSamplePosition* AMSamplePlate::samplePositionAt(size_t index){
	QVariant retVal = samples_->data(samples_->index(index, 0), Qt::DisplayRole);
	if(retVal.isValid())
		return (AMSamplePosition*) retVal.value<void*>();
	return NULL;
}

AMSamplePosition* AMSamplePlate::samplePositionByName(const QString &name){
	if(sampleName2samplePosition_.containsF(name))
		return sampleName2samplePosition_.valueF(name);
	return NULL;
}

AMSample* AMSamplePlate::sampleAt(size_t index){
	AMSamplePosition *sp = samplePositionAt(index);
	if(sp)
		return sp->sample();
	return NULL;
}

AMSample* AMSamplePlate::sampleByName(const QString &name){
	if(sampleName2samplePosition_.containsF(name))
		return samplePositionByName(name)->sample();
	return NULL;
}

AMControlSetInfo* AMSamplePlate::positionAt(size_t index){
	AMSamplePosition *sp = samplePositionAt(index);
	if(sp)
		return sp->position();
	return NULL;
}

AMControlSetInfo* AMSamplePlate::positionByName(const QString &name){
	if(sampleName2samplePosition_.containsF(name))
		return samplePositionByName(name)->position();
	return NULL;
}

int AMSamplePlate::indexOf(const QString &name){
	if(sampleName2samplePosition_.containsF(name)){
		AMSamplePosition *sp = sampleName2samplePosition_.valueF(name);
		for(int x = 0; x < count(); x++)
			if(samplePositionAt(x) == sp)
				return x;
	}
	return -1;
}

void AMSamplePlate::setName(const QString &name){
	userName_ = name;
}

bool AMSamplePlate::setSamplePosition(size_t index, AMSamplePosition *sp){
	AMSamplePosition *tmpSP = samplePositionAt(index);
	bool retVal = samples_->setData(samples_->index(index, 0), qVariantFromValue((void*)sp), Qt::EditRole);
	if(retVal){
		sampleName2samplePosition_.removeR(tmpSP);
		sampleName2samplePosition_.set(sp->sample()->name(), sp);
	}
	return retVal;
}

bool AMSamplePlate::addSamplePosition(size_t index, AMSamplePosition *sp){
	if(!sp || !sp->sample() || !sp->position())
		return false;
	else if(sampleName2samplePosition_.containsR(sp))
		return false;
	else if(!samples_->insertRows(index, 1))
		return false;
	return setSamplePosition(index, sp);
}

bool AMSamplePlate::addSamplePosition(size_t index, AMSample *sample, AMControlSetInfo *position){
	if(!sample || !position)
		return false;
	AMSamplePosition *tmpSP = new AMSamplePosition(sample, position, this);
	return addSamplePosition(index, tmpSP);
}

bool AMSamplePlate::appendSamplePosition(AMSamplePosition *sp){
	return addSamplePosition(count(), sp);
}

bool AMSamplePlate::appendSamplePosition(AMSample *sample, AMControlSetInfo *position){
	return addSamplePosition(count(), sample, position);
}

bool AMSamplePlate::removeSamplePosition(AMSamplePosition *sp){
	return removeSamplePosition( indexOf(sp->sample()->name()) );
}

bool AMSamplePlate::removeSamplePosition(size_t index){
	if( (int)index >= count())
		return false;
	AMSamplePosition *rSP = samplePositionAt(index);
	bool retVal = samples_->removeRows(index, 1);
	if(retVal)
		sampleName2samplePosition_.removeR(rSP);
	return retVal;
}

void AMSamplePlate::onDataChanged(QModelIndex a, QModelIndex b){
	if(a.row() != b.row())
		return;
	if(insertRowLatch != -1 && insertRowLatch == a.row()){
		insertRowLatch = -1;
		emit samplePositionAdded(a.row());
	}
	else
		emit samplePositionChanged(a.row());
}

void AMSamplePlate::onRowsInserted(QModelIndex parent, int start, int end){
	Q_UNUSED(parent);
	if(start != end)
		return;
	insertRowLatch = start;
}

void AMSamplePlate::onRowsRemoved(QModelIndex parent, int start, int end){
	Q_UNUSED(parent);
	if(start != end)
		return;
	emit samplePositionRemoved(start);
}

bool AMSamplePlate::setupModel(){
	samples_ = new AMSamplePlateModel(this);
	if(samples_){
		connect(samples_, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onDataChanged(QModelIndex,QModelIndex)));
		connect(samples_, SIGNAL(rowsInserted(const QModelIndex,int, int)), this, SLOT(onRowsInserted(QModelIndex,int,int)));
		connect(samples_, SIGNAL(rowsRemoved(const QModelIndex,int,int)), this, SLOT(onRowsRemoved(QModelIndex,int,int)));
		return true;
	}
	return false;
}

AMSamplePlateModel::AMSamplePlateModel(QObject *parent) :
		QAbstractListModel(parent)
{
	samples_ = new QList<AMSamplePosition*>();
}

int AMSamplePlateModel::rowCount(const QModelIndex & /*parent*/) const{
	return samples_->count();
}

QVariant AMSamplePlateModel::data(const QModelIndex &index, int role) const{
	if(!index.isValid())
		return QVariant();
	if(index.row() >= samples_->count())
		return QVariant();
	if(role == Qt::DisplayRole)
		return qVariantFromValue((void*)samples_->at(index.row()));
	else
		return QVariant();
}

QVariant AMSamplePlateModel::headerData(int section, Qt::Orientation orientation, int role) const{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
		return QString("Column %1").arg(section);
	else
		return QString("Row %1").arg(section);
}

bool AMSamplePlateModel::setData(const QModelIndex &index, const QVariant &value, int role){
	if (index.isValid()  && index.row() < samples_->count() && role == Qt::EditRole) {
		AMSamplePosition* sp;
		sp = (AMSamplePosition*) value.value<void*>();

		samples_->replace(index.row(), sp);
		emit dataChanged(index, index);
		return true;
	}
	return false;	// no value set
}

bool AMSamplePlateModel::insertRows(int position, int rows, const QModelIndex &index){
	if (index.row() <= samples_->count() && position <= samples_->count()) {
		beginInsertRows(QModelIndex(), position, position+rows-1);
		AMSamplePosition *sp = NULL;
		for (int row = 0; row < rows; ++row) {
			samples_->insert(position, sp);
		}
		endInsertRows();
		return true;
	}
	return false;
}

bool AMSamplePlateModel::removeRows(int position, int rows, const QModelIndex &index){
	if (index.row() < samples_->count() && position < samples_->count()) {
		beginRemoveRows(QModelIndex(), position, position+rows-1);
		for (int row = 0; row < rows; ++row) {
			samples_->removeAt(position);
		}
		endRemoveRows();
		return true;
	}
	return false;
}

Qt::ItemFlags AMSamplePlateModel::flags(const QModelIndex &index) const{
	Qt::ItemFlags flags;
	if (index.isValid() && index.row() < samples_->count() && index.column()<4)
		flags = Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return flags;
}



AMSamplePosition::AMSamplePosition(AMSample *sample, AMControlSetInfo *position, QObject *parent) :
		QObject(parent)
{
	sample_ = sample;
	position_ = position;
	if(position_)
		connect(position_, SIGNAL(valuesChanged(int)), this, SIGNAL(positionValuesChanged(int)));
}

AMSample* AMSamplePosition::sample(){
	return sample_;
}

AMControlSetInfo* AMSamplePosition::position(){
	return position_;
}

void AMSamplePosition::setSample(AMSample *sample){
	sample_ = sample;
}

void AMSamplePosition::setPosition(AMControlSetInfo *position){
	if(position_)
		disconnect(position_, SIGNAL(valuesChanged()), this, SIGNAL(positionValuesChanged()));
	position_ = position;
	if(position_)
		connect(position_, SIGNAL(valuesChanged()), this, SIGNAL(positionValuesChanged()));
}

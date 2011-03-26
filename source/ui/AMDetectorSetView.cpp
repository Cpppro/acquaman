#include "AMDetectorSetView.h"

AMDetectorSetView::AMDetectorSetView(AMDetectorSet *viewSet, bool configureOnly, QWidget *parent) :
		QGroupBox(parent)
{
	viewSet_ = viewSet;
	configureOnly_ = configureOnly;
	setTitle(viewSet->name());
	gl_ = new QGridLayout();
	AMDetectorView *tmpDV;
	AMDetector *tmpD;
	QLabel *tmpLabel;
	QPushButton *tmpButton;
	QCheckBox *tmpCheck;
	AMDetectorView *tmpDetails;
	for(int x = 0; x < viewSet_->count(); x++){
		tmpD = viewSet_->detectorAt(x);
		tmpDV = AMDetectorViewSupport::createBriefDetectorView(tmpD, configureOnly_);
		connect(tmpDV, SIGNAL(settingsConfigureRequested()), this, SLOT(onDetectorSetConfigurationRequested()));
		tmpLabel = new QLabel(tmpD->description());
		tmpButton = new QPushButton("Details");
		if(AMDetectorViewSupport::supportedDetailedViews(tmpD).count() == 0){
			tmpButton->setEnabled(false);
			detectorDetails_.append(0);
		}
		else{
			tmpDetails = AMDetectorViewSupport::createDetailedDetectorView(tmpD, configureOnly_);
			detectorDetails_.append(tmpDetails);
			connect(tmpButton, SIGNAL(clicked()), tmpDetails, SLOT(show()));
			connect(tmpDetails, SIGNAL(settingsConfigureRequested()), this, SLOT(onDetectorSetConfigurationRequested()));
		}
		if(configureOnly_){
			tmpCheck = new QCheckBox();
			if(viewSet_->isDefaultAt(x))
				tmpCheck->setChecked(true);
			checkBoxes_.append(tmpCheck);
			connect(tmpCheck, SIGNAL(toggled(bool)), this, SLOT(onDetectorSetConfigurationRequested()));
		}
		detectorBoxes_.append(tmpDV);

		gl_->addWidget(tmpLabel,		x, 0, 1, 1, 0);
		gl_->addWidget(tmpDV,			x, 2, 1, 1, 0);
		if(configureOnly_)
			gl_->addWidget(tmpCheck,	x, 1, 1, 1, 0);
		gl_->addWidget(tmpButton,		x, 3, 1, 1, 0);
	}

	gl_->setRowStretch(viewSet_->count(), 10);
	gl_->setVerticalSpacing(1);
	gl_->setHorizontalSpacing(1);
	setLayout(gl_);

	connect(viewSet_, SIGNAL(detectorAdded(int)), this, SLOT(onDetectorAddedToSet(int)));
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
}

int AMDetectorSetView::count() const{
	return viewSet_->count();
}

AMDetectorSet* AMDetectorSetView::detectorSet(){
	return viewSet_;
}

AMDetectorView* AMDetectorSetView::boxByName(const QString &name){
	return detectorBoxes_.at(viewSet_->indexOfKey(name));
}

AMDetectorView const * const AMDetectorSetView::boxAt(int row) const{
	return detectorBoxes_.at(row);
}

AMDetectorView* AMDetectorSetView::detailByName(const QString &name){
	return detectorDetails_.at(viewSet_->indexOf(name));
}

AMDetectorView const * const AMDetectorSetView::detailAt(int row) const{
	return detectorDetails_.at(row);
}

bool AMDetectorSetView::checkedAt(int row) const{
	if(!configureOnly_)
		return false;
	if(checkBoxes_.at(row))
		return checkBoxes_.at(row)->isChecked();
}

AMDetectorInfoSet AMDetectorSetView::currentValues(){
	viewSet_->toInfoSet();
}

AMDetectorInfoSet AMDetectorSetView::configValues(){
	AMDetectorInfoSet rv;

	if(!configureOnly_)
		return currentValues();

	for(int x = 0; x < viewSet_->count(); x++){
		if(detailAt(x))
			rv.addDetectorInfo(detailAt(x)->configurationSettings(), checkedAt(x));
		else
			rv.addDetectorInfo(boxAt(x)->configurationSettings(), checkedAt(x));
	}
	return rv;
}

QDebug operator<<(QDebug d, const AMDetectorSetView& dsv){
	for(int x = 0; x < dsv.count(); x++){
		if(dsv.boxAt(x) && dsv.boxAt(x)->configurationSettings())
			d << *(dsv.boxAt(x)->configurationSettings());
		if(dsv.detailAt(x) && dsv.detailAt(x)->configurationSettings())
			d << *(dsv.detailAt(x)->configurationSettings());
	}
	return d;
}

void AMDetectorSetView::onDetectorAddedToSet(int index){
	AMDetectorView *tmpDV;
	AMDetector *tmpD;
	QLabel *tmpLabel;
	QPushButton *tmpButton;
	QCheckBox *tmpCheck;
	AMDetectorView *tmpDetails;
	tmpD = viewSet_->detectorAt(index);
	tmpDV = AMDetectorViewSupport::createBriefDetectorView(tmpD, configureOnly_);
	connect(tmpDV, SIGNAL(settingsConfigureRequested()), this, SLOT(onDetectorSetConfigurationRequested()));
	tmpLabel = new QLabel(tmpD->description());
	tmpButton = new QPushButton("Details");
	if(AMDetectorViewSupport::supportedDetailedViews(tmpD).count() == 0){
		tmpButton->setEnabled(false);
		detectorDetails_.insert(index, 0);
	}
	else{
		tmpDetails = AMDetectorViewSupport::createDetailedDetectorView(tmpD, configureOnly_);
		detectorDetails_.insert(index, tmpDetails);
		connect(tmpButton, SIGNAL(clicked()), tmpDetails, SLOT(show()));
		connect(tmpDetails, SIGNAL(settingsConfigureRequested()), this, SLOT(onDetectorSetConfigurationRequested()));
	}
	if(configureOnly_){
		tmpCheck = new QCheckBox();
		if(viewSet_->isDefaultAt(index))
			tmpCheck->setChecked(true);
		checkBoxes_.insert(index, tmpCheck);
		connect(tmpCheck, SIGNAL(toggled(bool)), this, SLOT(onDetectorSetConfigurationRequested()));
	}
	detectorBoxes_.insert(index, tmpDV);
	gl_->addWidget(tmpLabel,		index, 0, 1, 1, 0);
	gl_->addWidget(tmpDV,			index, 2, 1, 1, 0);
	if(configureOnly_)
		gl_->addWidget(tmpCheck,	index, 1, 1, 1, 0);
	gl_->addWidget(tmpButton,		index, 3, 1, 1, 0);

	for(int x = 0; x < viewSet_->count()-1; x++){
		gl_->setRowStretch(x ,1);
	}
	gl_->setRowStretch(viewSet_->count(), 10);
}

void AMDetectorSetView::onDetectorSetSettingsChanged(){

}

void AMDetectorSetView::onDetectorSetConfigurationRequested(){
	emit configValuesChanged();
}
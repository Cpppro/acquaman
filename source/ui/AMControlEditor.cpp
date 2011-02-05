/*
Copyright 2010, 2011 Mark Boots, David Chevrier.

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


#include "AMControlEditor.h"
#include <QApplication>
#include <QMouseEvent>

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>

AMBasicControlEditor::AMBasicControlEditor(AMControl* control, QWidget *parent) :
	QFrame(parent)
{
	setObjectName("AMNumericControl");

	control_ = control;

	// Create objects:
	valueLabel_ = new QLabel("[unconnected]");
	unitsLabel_ = new QLabel("?");


	// Layout:
	QHBoxLayout* hl = new QHBoxLayout();
	hl->addWidget(valueLabel_, 2);
	hl->addSpacing(2);
	hl->addWidget(unitsLabel_, 0);
	hl->setMargin(2);

	setLayout(hl);

	// Style: TODO: move out of this constructor into app-wide stylesheet
	valueLabel_->setStyleSheet("color: rgb(0, 0, 0); background-color: rgb(255, 255, 255);");
	valueLabel_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	setFrameStyle(QFrame::StyledPanel);
	setStyleSheet("QFrame#AMNumericControl { background: white; } ");
	setHappy(false);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);


	// Make connections:
	if(control_) {
		connect(control_, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
		connect(control_, SIGNAL(unitsChanged(QString)), this, SLOT(onUnitsChanged(QString)));
		connect(control_, SIGNAL(connected(bool)), this, SLOT(setHappy(bool)));
		connect(control_, SIGNAL(movingChanged(bool)), this, SLOT(onMotion(bool)));
	}
	connect(this, SIGNAL(clicked()), this, SLOT(onEditStart()));

	// Create the editor dialog:
	dialog_ = new AMBasicControlEditorStyledInputDialog(this);
	// dialog_->setInputMode(QInputDialog::DoubleInput);
	dialog_->hide();
	dialog_->setWindowModality(Qt::NonModal);
	connect(dialog_, SIGNAL(doubleValueSelected(double)), control_, SLOT(move(double)));

}

void AMBasicControlEditor::onValueChanged(double newVal) {
	valueLabel_->setText(QString("%1").arg(newVal));
}
void AMBasicControlEditor::onUnitsChanged(const QString& units) {
	unitsLabel_->setText(units);
}


void AMBasicControlEditor::setHappy(bool happy) {
	if(happy)
		unitsLabel_->setStyleSheet("border: 1px outset #00df00; background: #d4ffdf; padding: 1px; width: 100%; color: #00df00;");
	else
		unitsLabel_->setStyleSheet("border: 1px outset #f20000; background: #ffdfdf;	padding: 1px; color: #f20000;");
}

void AMBasicControlEditor::onMotion(bool moving) {
	if(moving)
		unitsLabel_->setStyleSheet("border: 1px outset blue; background: #ffdfdf;	padding: 1px; color: blue;");
	else
		setHappy(control_->isConnected());
}

void AMBasicControlEditor::onEditStart() {

	if(!control_->canMove()) {
		QApplication::beep();
		return;
	}

	//bool ok;

	dialog_->setDoubleValue(control_->value());
	dialog_->setDoubleMaximum(control_->maximumValue());
	dialog_->setDoubleMinimum(control_->minimumValue());
	dialog_->setDoubleDecimals(3);	// todo: display precision?
	dialog_->setLabelText(control_->objectName());
	dialog_->setSuffix(control_->units());
	dialog_->show();
	dialog_->move( mapToGlobal(QPoint(width()/2,height()/2)) - QPoint(dialog_->width()/2, dialog_->height()/2) );

}

void AMBasicControlEditor::mouseReleaseEvent ( QMouseEvent * event ) {
	if(event->button() == Qt::LeftButton) {
		event->accept();
		emit clicked();
	}
	else
		event->ignore();

}

void AMBasicControlEditorStyledInputDialog::resizeEvent(QResizeEvent *  event )
{
	QDialog::resizeEvent(event);


	//int side = qMin(width(), height());
	// Create a rounded-rectangle mask to shape this window:
	QPainterPath path;
	path.addRoundedRect(0, 0, width(), height(), 14, 14);
	QPolygonF polygonf = path.toFillPolygon(QTransform());
	QRegion maskedRegion( polygonf.toPolygon() );
	setMask(maskedRegion);

}

AMBasicControlEditorStyledInputDialog::AMBasicControlEditorStyledInputDialog( QWidget * parent, Qt::WindowFlags flags ) : QDialog(parent, flags) {
	setObjectName("styledDialog");
	setStyleSheet("#styledDialog { background-color: rgb(31,62,125); border: 2px outset white; border-radius: 10px; }  QLabel { color: white; font: bold 12pt \"Helvetica\"; } QPushButton { color: white; /* font: bold 12pt  \"Helvetica\"*/; border: 1px outset rgb(158,158,158); border-radius: 5px; min-height: 24px; padding: 3px; width: 80px; margin: 3px;} #okButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(191, 218, 178, 255), stop:0.34 rgba(135, 206, 96, 255), stop:1 rgba(65, 157, 0, 255));} #cancelButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(232, 209, 209, 255), stop:0.34 rgba(229, 112, 119, 255), stop:1 rgba(197, 20, 32, 255)); } QDoubleSpinBox { padding: 3px; color: black; font: bold 12pt \"Helvetica\"; border: 1px outset rgb(158,158,158); selection-background-color: rgb(205, 220, 243); selection-color: black;}");// TODO: continue here...
	// setStyleSheet("#styledDialog {  border: 1px outset black; }  QLabel { color: black; font: bold 12pt \"Helvetica\"; } QPushButton { min-height: 24px; padding: 3px; width: 80px; margin: 3px;} #okButton {border: 1px outset #00df00; background: #d4ffdf; padding: 1px; width: 100%; color: #00df00; } #cancelButton { border: 1px outset #f20000; background: #ffdfdf;	padding: 1px; color: #f20000; } QDoubleSpinBox { padding: 3px; color: black; font: bold 12pt \"Helvetica\"; border: 1px outset rgb(158,158,158); selection-background-color: rgb(205, 220, 243); selection-color: black;}");// TODO: continue here...

	label_ = new QLabel("New value:");
	label_->setAlignment(Qt::AlignCenter);
	spinBox_ = new QDoubleSpinBox();
	spinBox_->setObjectName("valueEntry");
	spinBox_->setButtonSymbols(QAbstractSpinBox::NoButtons);
	okButton_ = new QPushButton("Ok");
	okButton_->setObjectName("okButton");
	cancelButton_ = new QPushButton("Cancel");
	cancelButton_->setObjectName("cancelButton");

	QVBoxLayout* vl = new QVBoxLayout();
	vl->addWidget(label_);
	vl->addWidget(spinBox_);

	QHBoxLayout* hl = new QHBoxLayout();
	hl->addWidget(cancelButton_);
	hl->addWidget(okButton_);
	vl->addLayout(hl);

	setLayout(vl);

	connect(this, SIGNAL(accepted()), this, SLOT(onAccepted()));
	connect(okButton_, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton_, SIGNAL(clicked()), this, SLOT(reject()));

	okButton_->setDefault(true);
}

void AMBasicControlEditorStyledInputDialog::onAccepted() {
	emit doubleValueSelected(spinBox_->value());
}

void AMBasicControlEditorStyledInputDialog::setDoubleValue(double d) { spinBox_->setValue(d); }
void AMBasicControlEditorStyledInputDialog::setDoubleMaximum(double d) { spinBox_->setMaximum(d); }
void AMBasicControlEditorStyledInputDialog::setDoubleMinimum(double d) { spinBox_->setMinimum(d); }
void AMBasicControlEditorStyledInputDialog::setDoubleDecimals(double d) { spinBox_->setDecimals(d); }
void AMBasicControlEditorStyledInputDialog::setLabelText(const QString& s) { label_->setText(s); }
void AMBasicControlEditorStyledInputDialog::setSuffix(const QString& s) { spinBox_->setSuffix(QString(" ") + s); }

void AMBasicControlEditorStyledInputDialog::showEvent ( QShowEvent * event ) { QDialog::showEvent(event); spinBox_->setFocus(); }


AMControlEditor::AMControlEditor(AMControl* control, AMControl* statusTagControl, bool readOnly, QWidget *parent) :
	QGroupBox(parent)
{
	setObjectName("AMControlEdit");

	control_ = control;
	readOnly_ = readOnly;
	statusTagControl_ = statusTagControl;
	if(!control_->canMove())
		readOnly_ = true;

	// Create objects:
	valueLabel_ = new QLabel("[unconnected]");
	unitsLabel_ = new QLabel("?");
	if(statusTagControl_){
		QFont statusFont;
		statusFont.setPointSize(10);
		statusLabel_ = new QLabel("[unconnected]");
		statusLabel_->setFont(statusFont);
		statusLabel_->setMargin(1);
	}
//	nameLabel_ = new QLabel("?");
	if(control_)
		setTitle(control_->name());

	// Layout:
	QHBoxLayout* hl = new QHBoxLayout();
	QVBoxLayout* vl = new QVBoxLayout();
	hl->addWidget(valueLabel_, 2);
	hl->addSpacing(2);
	hl->addWidget(unitsLabel_, 0);
	hl->setMargin(2);
	vl->addLayout(hl);
	if(statusTagControl_){
		QHBoxLayout* hl2 = new QHBoxLayout();
		hl2->addWidget(statusLabel_, Qt::AlignCenter);
		hl2->setStretch(0, 2);
		//vl->addWidget(statusLabel_, Qt::AlignHCenter);
		vl->addLayout(hl2);
	}
	vl->setSpacing(1);
	vl->setMargin(2);

//	QVBoxLayout *vl = new QVBoxLayout();
//	vl->addWidget(nameLabel_, 0, Qt::AlignLeft);
//	vl->addLayout(hl, 2);
	setLayout(vl);
//	setLayout(hl);

	// Style: TODO: move out of this constructor into app-wide stylesheet
	valueLabel_->setStyleSheet("color: rgb(0, 0, 0); background-color: rgb(255, 255, 255);");
	valueLabel_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

	//setFrameStyle(QFrame::StyledPanel);
	//setStyleSheet("QFrame#AMControlEdit { background: white; } ");
	//setStyleSheet("QGroupBox#AMControlEdit { background: white; } ");
	setHappy(false);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);


	// Make connections:
	if(control_) {
		connect(control_, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
		connect(control_, SIGNAL(unitsChanged(QString)), this, SLOT(onUnitsChanged(QString)));
		connect(control_, SIGNAL(connected(bool)), this, SLOT(setHappy(bool)));
		connect(control_, SIGNAL(movingChanged(bool)), this, SLOT(onMotion(bool)));
	}
	if(statusTagControl_)
		connect(statusTagControl_, SIGNAL(valueChanged(double)), this, SLOT(onStatusValueChanged(double)));
	connect(this, SIGNAL(clicked()), this, SLOT(onEditStart()));

	// Create the editor dialog:
	QStringList enumNames = QStringList();
	dialog_ = new AMControlEditorStyledInputDialog(enumNames, this);
	// dialog_->setInputMode(QInputDialog::DoubleInput);
	dialog_->hide();
	dialog_->setWindowModality(Qt::NonModal);
	connect(dialog_, SIGNAL(doubleValueSelected(double)), control_, SLOT(move(double)));
	connect(control_, SIGNAL(enumChanges(QStringList)), dialog_, SLOT(setEnumNames(QStringList)));

	if(control_ && control_->isConnected()){
		onValueChanged(control_->value());
		onUnitsChanged(control_->units());
		setHappy(control_->isConnected());
		if(control_->isEnum())
			dialog_->setEnumNames(control_->enumNames());
	}
	if(statusTagControl_ && statusTagControl_->isConnected())
		onStatusValueChanged(statusTagControl_->value());
}

void AMControlEditor::setReadOnly(bool readOnly){
	readOnly_ = readOnly;
	if(!control_->canMove())
		readOnly_ = true;
}

void AMControlEditor::setNoUnitsBox(bool noUnitsBox){
	if(noUnitsBox && !unitsLabel_->isHidden())
		unitsLabel_->hide();
	else if(!noUnitsBox && unitsLabel_->isHidden())
		unitsLabel_->show();
}

void AMControlEditor::overrideTitle(const QString &title){
	setTitle(title);
}

void AMControlEditor::onValueChanged(double newVal) {
	if(control_->isEnum()){
		valueLabel_->setText(control_->enumNameAt(newVal));
		unitsLabel_->setText("");
	}
	else
		valueLabel_->setText(QString("%1").arg(newVal));
}

void AMControlEditor::onUnitsChanged(const QString& units) {
	if(control_->isEnum())
		unitsLabel_->setText("");
	else
		unitsLabel_->setText(units);
}


void AMControlEditor::setHappy(bool happy) {
	if(happy){
		unitsLabel_->setStyleSheet("border: 1px outset #00df00; background: #d4ffdf; padding: 1px; width: 100%; color: #00df00;");
		readOnly_ = !control_->canMove();
		onUnitsChanged(control_->units());
	}
	else
		unitsLabel_->setStyleSheet("border: 1px outset #f20000; background: #ffdfdf;	padding: 1px; color: #f20000;");
}

void AMControlEditor::onMotion(bool moving) {
	if(moving)
		unitsLabel_->setStyleSheet("border: 1px outset blue; background: #ffdfdf;	padding: 1px; color: blue;");
	else
		setHappy(control_->isConnected());
}

void AMControlEditor::onEditStart() {

	if(readOnly_ || !control_->canMove()) {
		QApplication::beep();
		return;
	}

	//bool ok;

	dialog_->setDoubleValue(control_->value());
	dialog_->setDoubleMaximum(control_->maximumValue());
	dialog_->setDoubleMinimum(control_->minimumValue());
	dialog_->setDoubleDecimals(3);	// todo: display precision?
	dialog_->setLabelText(control_->objectName());
	dialog_->setSuffix(control_->units());
	dialog_->show();
	dialog_->move( mapToGlobal(QPoint(width()/2,height()/2)) - QPoint(dialog_->width()/2, dialog_->height()/2) );

}

void AMControlEditor::onStatusValueChanged(double newVal){
	if(statusTagControl_->isEnum())
		statusLabel_->setText(statusTagControl_->enumNameAt(newVal));
	else
		statusLabel_->setText(QString("%1").arg(newVal));
}

QSize AMControlEditor::sizeHint() const{
	QSize newHint = QGroupBox::sizeHint();
	newHint.setHeight(newHint.height()+6);
	return newHint;
}

void AMControlEditor::mouseReleaseEvent ( QMouseEvent * event ) {
	if(event->button() == Qt::LeftButton) {
		event->accept();
		emit clicked();
	}
	else
		event->ignore();

}

AMControlEditorStyledInputDialog::AMControlEditorStyledInputDialog( QStringList enumNames, QWidget * parent, Qt::WindowFlags flags ) : QDialog(parent, flags) {
	setObjectName("styledDialog");
	setStyleSheet("#styledDialog { background-color: rgb(31,62,125); border: 2px outset white; border-radius: 10px; }  QLabel { color: white; font: bold 12pt \"Helvetica\"; } QPushButton { color: white; /* font: bold 12pt  \"Helvetica\"*/; border: 1px outset rgb(158,158,158); border-radius: 5px; min-height: 24px; padding: 3px; width: 80px; margin: 3px;} #okButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(191, 218, 178, 255), stop:0.34 rgba(135, 206, 96, 255), stop:1 rgba(65, 157, 0, 255));} #cancelButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(232, 209, 209, 255), stop:0.34 rgba(229, 112, 119, 255), stop:1 rgba(197, 20, 32, 255)); } QDoubleSpinBox { padding: 3px; color: black; font: bold 12pt \"Helvetica\"; border: 1px outset rgb(158,158,158); selection-background-color: rgb(205, 220, 243); selection-color: black;}");// TODO: continue here...
	// setStyleSheet("#styledDialog {  border: 1px outset black; }  QLabel { color: black; font: bold 12pt \"Helvetica\"; } QPushButton { min-height: 24px; padding: 3px; width: 80px; margin: 3px;} #okButton {border: 1px outset #00df00; background: #d4ffdf; padding: 1px; width: 100%; color: #00df00; } #cancelButton { border: 1px outset #f20000; background: #ffdfdf;	padding: 1px; color: #f20000; } QDoubleSpinBox { padding: 3px; color: black; font: bold 12pt \"Helvetica\"; border: 1px outset rgb(158,158,158); selection-background-color: rgb(205, 220, 243); selection-color: black;}");// TODO: continue here...

	label_ = new QLabel("New value:");
	label_->setAlignment(Qt::AlignCenter);
	enumNames_ = enumNames;
	if(enumNames_.count() > 0)
		isEnum_ = true;
	spinBox_ = new QDoubleSpinBox();
	spinBox_->setObjectName("valueEntry");
	spinBox_->setButtonSymbols(QAbstractSpinBox::NoButtons);
	comboBox_ = new QComboBox();
	comboBox_->setObjectName("valueEntry");
	comboBox_->addItems(enumNames_);
	okButton_ = new QPushButton("Ok");
	okButton_->setObjectName("okButton");
	cancelButton_ = new QPushButton("Cancel");
	cancelButton_->setObjectName("cancelButton");

	vl_ = new QVBoxLayout();
	vl_->addWidget(label_);
	if(!isEnum_){
		vl_->addWidget(spinBox_);
		comboBox_->hide();
	}
	else{
		vl_->addWidget(comboBox_);
		spinBox_->hide();
	}

	hl_ = new QHBoxLayout();
	hl_->addWidget(cancelButton_);
	hl_->addWidget(okButton_);
	vl_->addLayout(hl_);

	setLayout(vl_);

	connect(this, SIGNAL(accepted()), this, SLOT(onAccepted()));
	connect(okButton_, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton_, SIGNAL(clicked()), this, SLOT(reject()));

	okButton_->setDefault(true);
}

void AMControlEditorStyledInputDialog::setDoubleValue(double d) {
	if(!isEnum_)
		spinBox_->setValue(d);
	else
		comboBox_->setCurrentIndex((int)d);
}

void AMControlEditorStyledInputDialog::setDoubleMaximum(double d) {
	if(!isEnum_)
	spinBox_->setMaximum(d);
}

void AMControlEditorStyledInputDialog::setDoubleMinimum(double d) {
	if(!isEnum_)
		spinBox_->setMinimum(d);
}

void AMControlEditorStyledInputDialog::setDoubleDecimals(double d) {
	if(!isEnum_)
		spinBox_->setDecimals(d);
}

void AMControlEditorStyledInputDialog::setLabelText(const QString& s) {
	label_->setText(s);
}

void AMControlEditorStyledInputDialog::setEnumNames(const QStringList &sl){
	bool oldIsEnum = isEnum_;
	if(sl.count() > 0)
		isEnum_ = true;
	else
		isEnum_ = false;
	enumNames_ = sl;
	comboBox_->clear();
	comboBox_->addItems(enumNames_);
	if(oldIsEnum != isEnum_){
		if(!isEnum_){
			vl_->removeWidget(comboBox_);
			vl_->insertWidget(1, spinBox_);
			comboBox_->hide();
			spinBox_->show();
		}
		else{
			vl_->removeWidget(spinBox_);
			vl_->insertWidget(1, comboBox_);
			spinBox_->hide();
			comboBox_->show();
		}
	}
}

void AMControlEditorStyledInputDialog::setSuffix(const QString& s) {
	spinBox_->setSuffix(QString(" ") + s);
}

void AMControlEditorStyledInputDialog::onAccepted() {
	double eVal;
	if(!isEnum_)
		eVal = spinBox_->value();
	else
		eVal = comboBox_->currentIndex();
	emit doubleValueSelected(eVal);
}

void AMControlEditorStyledInputDialog::resizeEvent(QResizeEvent *  event )
{
	QDialog::resizeEvent(event);


	//int side = qMin(width(), height());
	// Create a rounded-rectangle mask to shape this window:
	QPainterPath path;
	path.addRoundedRect(0, 0, width(), height(), 14, 14);
	QPolygonF polygonf = path.toFillPolygon(QTransform());
	QRegion maskedRegion( polygonf.toPolygon() );
	setMask(maskedRegion);

}

void AMControlEditorStyledInputDialog::showEvent ( QShowEvent * event ) {
	QDialog::showEvent(event);
	if(!isEnum_)
		spinBox_->setFocus();
	else
		comboBox_->setFocus();
}

AMControlButton::AMControlButton(AMControl *control, QWidget *parent) :
//		QPushButton(parent)
		QToolButton(parent)
{
	setObjectName("AMControlButton");

	control_ = control;
	downValue_ = 1;
	upValue_ = 0;
	programaticToggle_ = false;
	if(control_)
		setText(control_->name());
	setHappy(false);

	// Make connections:
	if(control_) {
		connect(control_, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
		connect(control_, SIGNAL(connected(bool)), this, SLOT(setHappy(bool)));
	}
	connect(this, SIGNAL(clicked()), this, SLOT(onClicked()));
	connect(this, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));

	if(control_ && control_->isConnected()){
		programaticToggle_ = true;
		onValueChanged(control_->value());
		setHappy(control_->isConnected());
	}
}

void AMControlButton::setDownValue(double downValue){
	downValue_ = downValue;
}

void AMControlButton::setUpValue(double upValue){
	upValue_ = upValue;
}

void AMControlButton::overrideText(const QString &text){
	setText(text);
}

void AMControlButton::setCheckable(bool checkable){
	//QPushButton::setCheckable(checkable);
	QToolButton::setCheckable(checkable);
	onValueChanged(control_->value());
}

void AMControlButton::onValueChanged(double newVal) {
	if(isCheckable()){
		if(control_->value() == downValue_)
			setChecked(true);
			//setDown(true);
		else if(control_->value() == upValue_)
			setChecked(false);
			//setDown(false);

		qDebug() << "Down " << isDown() << " Checked " << isChecked();
	}
}

void AMControlButton::onClicked(){
	qDebug() << "Detected clicked";
	if(isCheckable())
		return;
	qDebug() << "Moved for clicked";
	control_->move(downValue_);
}

void AMControlButton::onToggled(bool toggled){
	if(programaticToggle_){
		programaticToggle_ = false;
		return;
	}
	qDebug() << "Detected toggle";
	if(toggled){
		qDebug() << "Toggled, go down";
		control_->move(downValue_);
	}
	else{
		qDebug() << "Untoggled, go up";
		control_->move(upValue_);
	}
}

void AMControlButton::setHappy(bool happy) {
	/*
	if(happy){
		setStyleSheet("border: 1px outset #00df00; background: #d4ffdf; padding: 1px; width: 100%; color: #00df00;");
	}
	else
		setStyleSheet("border: 1px outset #f20000; background: #ffdfdf;	padding: 1px; color: #f20000;");
	*/
}

AMBeamlineActionsListButton::AMBeamlineActionsListButton(AMBeamlineActionsList *actionsList, QWidget *parent) :
		QToolButton(parent)
{
	actionsList_ = actionsList;
	connect(this, SIGNAL(clicked()), this, SLOT(onClicked()));
}

void AMBeamlineActionsListButton::overrideText(const QString &text){
	setText(text);
}

void AMBeamlineActionsListButton::onClicked(){
	qDebug() << "Trying to start actionsList";
	if(actionsList_ && actionsList_->count() > 0 && !actionsList_->action(0)->hasFinished()){
		qDebug() << "Starting it";
		actionsList_->action(0)->start();
	}
}
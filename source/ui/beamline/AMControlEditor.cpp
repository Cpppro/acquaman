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


#include "AMControlEditor.h"
#include <QApplication>
#include <QMouseEvent>

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QPixmap>
#include <QMovie>
#include <QMenu>

#include "util/AMFontSizes.h"

QMovie* AMBasicControlEditor::movingIcon_ = 0;
QPixmap* AMBasicControlEditor::invalidIcon_ = 0;
QPixmap* AMBasicControlEditor::minorIcon_ = 0;
QPixmap* AMBasicControlEditor::majorIcon_ = 0;
QPixmap* AMBasicControlEditor::lockedIcon_ = 0;

AMBasicControlEditor::AMBasicControlEditor(AMControl* control, QWidget *parent) :
	QFrame(parent)
{
	setObjectName("AMControlEditor");

	control_ = control;
	readOnly_ = false;

	// create static caches, if not already here:
	if(!movingIcon_) {
		movingIcon_ = new QMovie(":/loading2_transparent.gif");
		movingIcon_->start();
	}
	if(!invalidIcon_)
		invalidIcon_ = new QPixmap(":/16x16/network-error.png");
	if(!minorIcon_)
		minorIcon_ = new QPixmap(":/16x16/dialog-warning.png");
	if(!majorIcon_)
		majorIcon_ = new QPixmap(":/22x22/redCrash.png");
	if(!lockedIcon_)
		lockedIcon_ = new QPixmap(":/16x16/locked.png");

	// Create objects. Thanks to implicit sharing of QPixmap, all instances of this widget will all use the same QPixmap storage, created in the static icons.
	valueLabel_ = new QLabel();
	enumButton_ = new QToolButton();
	enumMenu_ = new QMenu(this);
	enumButton_->setMenu(enumMenu_);
	enumButton_->setPopupMode(QToolButton::InstantPopup);
	connect(enumMenu_, SIGNAL(triggered(QAction*)), this, SLOT(onEnumValueChosen(QAction*)));

	movingLabel_ = new QLabel();
	movingLabel_->setMovie(movingIcon_);
	movingLabel_->setToolTip("Moving...");

	invalidLabel_ = new QLabel();
	invalidLabel_->setPixmap(*invalidIcon_);

	minorLabel_ = new QLabel();
	minorLabel_->setPixmap(*minorIcon_);
	minorLabel_->setToolTip("Alarm: Minor");

	majorLabel_ = new QLabel();
	majorLabel_->setPixmap(*majorIcon_);
	majorLabel_->setToolTip("Alarm: Major");

	lockedLabel_ = new QLabel();
	lockedLabel_->setPixmap(*lockedIcon_);
	lockedLabel_->setToolTip("Cannot Move");

	statusFrame_ = new QFrame();
	statusFrame_->setMinimumWidth(22);
	statusFrame_->setMinimumHeight(22);
	statusFrame_->setObjectName("StatusFrame");

	QHBoxLayout* shl = new QHBoxLayout();
	shl->addWidget(movingLabel_);
	shl->addWidget(lockedLabel_);
	shl->addWidget(invalidLabel_);
	shl->addWidget(minorLabel_);
	shl->addWidget(majorLabel_);
	shl->setContentsMargins(0,0,0,0);
	statusFrame_->setLayout(shl);
	movingLabel_->hide();

	// Layout:
	QHBoxLayout* hl = new QHBoxLayout();
	hl->addWidget(valueLabel_, 2);
	hl->addWidget(enumButton_, 2);
	hl->addWidget(statusFrame_, 0);
	hl->setSpacing(1);
	hl->setContentsMargins(2,2,2,2);

	setLayout(hl);
	enumButton_->hide();

	valueLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	enumButton_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	setFrameStyle(QFrame::StyledPanel);
	setStyleSheet("#AMControlEditor { background: white; } ");
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

	reviewControlState();

	// Create the editor dialog:
	dialog_ = new AMBasicControlEditorStyledInputDialog(this);
	dialog_->hide();
	dialog_->setWindowModality(Qt::NonModal);
	connect(dialog_, SIGNAL(doubleValueSelected(double)), this, SLOT(onNewSetpointChosen(double)));

	// Make connections:
	if(control_) {
		connect(control_, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
		connect(control_, SIGNAL(connected(bool)), this, SLOT(reviewControlState()));
		connect(control_, SIGNAL(unitsChanged(QString)), this, SLOT(onUnitsChanged(QString)));
		connect(control_, SIGNAL(alarmChanged(int,int)), this, SLOT(reviewControlState()));
		connect(control_, SIGNAL(movingChanged(bool)), this, SLOT(onMotion(bool)));

		// If the control is connected already, update our state right now.
		if(control_->isConnected()) {
			onValueChanged(control_->value());
			onMotion(control_->isMoving());
			onEnumChanges();
		}
	}
	connect(this, SIGNAL(clicked()), this, SLOT(onEditStart()));
}

void AMBasicControlEditor::setReadOnly(bool readOnly) {
	readOnly_ = readOnly;

	// disable the popup menu for the enum button
	enumButton_->setMenu(readOnly_ ? 0 : enumMenu_);

	// update whether the lock label is shown. It's shown if we cannot move the control, or if this widget is configured read-only.
	reviewControlState();
}

void AMBasicControlEditor::onValueChanged(double newVal) {
	if(control_->isEnum())
		enumButton_->setText(control_->enumNameAt(newVal));
	else
		valueLabel_->setText(QString("%1 %2").arg(newVal).arg(control_->units()));
}

void AMBasicControlEditor::onUnitsChanged(const QString &units) {
	valueLabel_->setText(QString("%1 %2").arg(control_->value()).arg(units));
}

void AMBasicControlEditor::reviewControlState() {
	invalidLabel_->hide();
	minorLabel_->hide();
	majorLabel_->hide();
	lockedLabel_->setVisible(readOnly_);

	if(!control_ || !control_->isConnected()) {
		invalidLabel_->show();
		invalidLabel_->setToolTip("Not Connected");
		setStyleSheet("#AMControlEditor {border: 1px outset #f20000; background: #ffffff;}");
		return;
	}

	lockedLabel_->setVisible(!control_->canMove() || readOnly_);

	switch(control_->alarmSeverity()) {
	case AMControl::InvalidAlarm:
		invalidLabel_->show();
		invalidLabel_->setToolTip("Alarm: Invalid");
		setStyleSheet("#AMControlEditor {border: 1px outset #f20000; background: #ffffff;}");
		break;
	case AMControl::MinorAlarm:
		minorLabel_->show();
		setStyleSheet("#AMControlEditor {border: 1px outset rgb(242,242,0); background: rgb(255,255,240);}");
		break;
	case AMControl::MajorAlarm:
		majorLabel_->show();
		setStyleSheet("#AMControlEditor {border: 1px outset #f20000; background: #ffdfdf;}");
		break;
	default: // all good. Normal look.
		setStyleSheet("#AMControlEditor { background: white; } ");
		//	Traditional green: setStyleSheet("#AMControlEditor {border: 1px outset #00df00; background: #d4ffdf;}");
		break;
	}
}

void AMBasicControlEditor::onEnumChanges() {
	if(control_->isEnum()) {
		enumButton_->show();
		valueLabel_->hide();

		// With new enum values, we might have to update the text value on the button.
		onValueChanged(control_->value());

		enumMenu_->clear();
		int i=0;
		foreach(QString enumString, control_->enumNames()) {
			QAction* action = enumMenu_->addAction(enumString);
			action->setData(i++);	// remember the index inside this action.
		}
	}
	else {
		enumButton_->hide();
		valueLabel_->show();
	}
}

void AMBasicControlEditor::onEnumValueChosen(QAction *action) {
	int value = action->data().toInt();
	onNewSetpointChosen(double(value));
}

void AMBasicControlEditor::onMotion(bool moving) {
	if(moving)
		movingLabel_->show();
	else
		movingLabel_->hide();
}

void AMBasicControlEditor::onEditStart() {

	if(!control_ || !control_->canMove() || readOnly_) {
		QApplication::beep();
		return;
	}

	if(control_->isEnum())
		return;	// don't show the editor in this situation; users use the tool button menu directly;

	dialog_->setDoubleMaximum(control_->maximumValue());
	dialog_->setDoubleMinimum(control_->minimumValue());
	dialog_->setDoubleDecimals(3);	/// \todo Display precision
	dialog_->setDoubleValue(control_->value());
	dialog_->setLabelText(control_->description().isEmpty() ? control_->name() : control_->description());
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

	// Create a rounded-rectangle mask to shape this window:
	QPainterPath path;
	path.addRoundedRect(0, 0, width(), height(), 14, 14);
	QPolygonF polygonf = path.toFillPolygon(QTransform());
	QRegion maskedRegion( polygonf.toPolygon() );
	setMask(maskedRegion);

}

AMBasicControlEditorStyledInputDialog::AMBasicControlEditorStyledInputDialog( QWidget * parent, Qt::WindowFlags flags ) : QDialog(parent, flags) {
	setObjectName("styledDialog");
	setStyleSheet("#styledDialog { background-color: rgb(31,62,125); border: 2px outset white; border-radius: 10px; }  QLabel { color: white; font: bold " AM_FONT_REGULAR_ " \"Helvetica\"; } QPushButton { color: white; border: 1px outset rgb(158,158,158); border-radius: 5px; min-height: 24px; padding: 3px; width: 80px; margin: 3px;} #okButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(191, 218, 178, 255), stop:0.34 rgba(135, 206, 96, 255), stop:1 rgba(65, 157, 0, 255));} #cancelButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(232, 209, 209, 255), stop:0.34 rgba(229, 112, 119, 255), stop:1 rgba(197, 20, 32, 255)); } QDoubleSpinBox { padding: 3px; color: black; font: bold " AM_FONT_REGULAR_ " \"Helvetica\"; border: 1px outset rgb(158,158,158); selection-background-color: rgb(205, 220, 243); selection-color: black;}");

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
void AMBasicControlEditorStyledInputDialog::setDoubleDecimals(int d) { spinBox_->setDecimals(d); }
void AMBasicControlEditorStyledInputDialog::setLabelText(const QString& s) { label_->setText(s); }
void AMBasicControlEditorStyledInputDialog::setSuffix(const QString& s) { spinBox_->setSuffix(QString(" ") + s); }

void AMBasicControlEditorStyledInputDialog::showEvent ( QShowEvent * event ) { QDialog::showEvent(event); spinBox_->setFocus(); spinBox_->selectAll(); }


void AMBasicControlEditor::onNewSetpointChosen(double value)
{
	if(control_)
		control_->move(value);
}




// AMControlEditor
////////////////////////////

AMControlEditor::AMControlEditor(AMControl* control, AMControl* statusTagControl, bool readOnly, bool configureOnly, QWidget *parent) :
	QGroupBox(parent)
{
	setObjectName("AMControlEdit");

	control_ = control;
	readOnly_ = readOnly;
	configureOnly_ = configureOnly;
	connectedOnce_ = false;
	newValueOnce_ = false;
	format_ = 'g';
	precision_ = 3;

	statusTagControl_ = statusTagControl;
	if(control_ && !control_->canMove())
		readOnly_ = true;

	// Create objects:
	valueLabel_ = new QLabel("[Not Connected]");
	unitsLabel_ = new QLabel("?");
	if(statusTagControl_){
		QFont statusFont;
		statusFont.setPointSize(10);
		statusLabel_ = new QLabel("[Not Connected]");
		statusLabel_->setFont(statusFont);
		statusLabel_->setMargin(1);
	}
	if(control_){
		if(control_->description() != "")
			setTitle(control_->description());
		else
			setTitle(control_->name());
	}

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
		vl->addLayout(hl2);
	}
	vl->setSpacing(1);
	vl->setMargin(2);

	setLayout(vl);

	// Style: TODO: move out of this constructor into app-wide stylesheet
	valueLabel_->setStyleSheet("color: rgb(0, 0, 0); background-color: rgb(255, 255, 255);");
	valueLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

	setHappy(false);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);


	// Create the editor dialog:
	QStringList enumNames = QStringList();
	dialog_ = new AMControlEditorStyledInputDialog(enumNames, this);
	dialog_->hide();
	dialog_->setWindowModality(Qt::NonModal);
	if(!configureOnly_)
		connect(dialog_, SIGNAL(doubleValueSelected(double)), control_, SLOT(move(double)));
	else
		connect(dialog_, SIGNAL(doubleValueSelected(double)), this, SLOT(onNewSetpoint(double)));
	
	// Make connections:
	if(control_){
		connect(control_, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
		connect(control_, SIGNAL(unitsChanged(QString)), this, SLOT(onUnitsChanged(QString)));
		connect(control_, SIGNAL(connected(bool)), this, SLOT(setHappy(bool)));
		connect(control_, SIGNAL(movingChanged(bool)), this, SLOT(onMotion(bool)));
		connect(control_, SIGNAL(enumChanges(QStringList)), dialog_, SLOT(setEnumNames(QStringList)));
		// If the control is connected already, update our state right now. (We won't get the connected() signal later.)
		if(control_->isConnected()) {
			setHappy(true);
			onValueChanged(control_->value());
			onUnitsChanged(control_->units());
			onMotion(control_->isMoving());
			if(control_->isEnum())
				dialog_->setEnumNames(control_->enumNames());
		}
	}
	if(statusTagControl_) {
		connect(statusTagControl_, SIGNAL(valueChanged(double)), this, SLOT(onStatusValueChanged(double)));
		if(statusTagControl_->isConnected())
			onStatusValueChanged(statusTagControl_->value());
	
	}

	connect(this, SIGNAL(clicked()), this, SLOT(onEditStart()));

}

double AMControlEditor::setpoint() const{
	return dialog_->setpoint();
}

AMControl* AMControlEditor::control() const{
	return control_;
}

bool AMControlEditor::setControlFormat(const QChar& format, int precision){
	if(format == 'g' || format == 'G' || format == 'e' || format == 'E' || format == 'f'){
		format_ = format;
		precision_ = precision;
		if(control_->isConnected())
			onValueChanged(control_->value());
		return true;
	}
	return false;
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

void AMControlEditor::setSetpoint(double newSetpoint){
	dialog_->setDoubleValue(newSetpoint);
	onNewSetpoint(newSetpoint);
}

void AMControlEditor::onValueChanged(double newVal) {
	if(configureOnly_ && connectedOnce_)
		return;
	if(control_->isEnum()){
		valueLabel_->setText(control_->enumNameAt(newVal));
		unitsLabel_->setText("");
	}
	else
		valueLabel_->setText(QString("%1").arg(newVal, 0, format_.toAscii(), precision_));
}

void AMControlEditor::onUnitsChanged(const QString& units) {
	if(configureOnly_ && connectedOnce_)
		return;
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
		onValueChanged(control_->value());
		if(!connectedOnce_){
			dialog_->setDoubleMaximum(control_->maximumValue());
			dialog_->setDoubleMinimum(control_->minimumValue());
			dialog_->setDoubleValue(control_->value());
			connectedOnce_ = true;
		}
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

	dialog_->setDoubleMaximum(control_->maximumValue());
	dialog_->setDoubleMinimum(control_->minimumValue());
	if(!configureOnly_)
		dialog_->setDoubleValue(control_->value());
	dialog_->setDoubleDecimals(3);	// todo: display precision?
	dialog_->setLabelText(control_->objectName());
	dialog_->setSuffix(control_->units());
	dialog_->show();
	dialog_->move( mapToGlobal(QPoint(width()/2,height()/2)) - QPoint(dialog_->width()/2, dialog_->height()/2) );

}

void AMControlEditor::onNewSetpoint(double newVal){
	if(!configureOnly_)
		return;
	if(control_->isEnum()){
		if( fabs(control_->value()-newVal) < control_->tolerance() )
			valueLabel_->setText(QString("%1").arg(control_->enumNameAt(newVal)));
		else
			valueLabel_->setText(QString("%1 (from %2)").arg(control_->enumNameAt(newVal)).arg(control_->enumNameAt(control_->value())) );
		unitsLabel_->setText("");
	}
	else{
		if( fabs(control_->value()-newVal) < control_->tolerance() )
			valueLabel_->setText(QString("%1").arg(newVal));
		else
			valueLabel_->setText(QString("%1 (from %2)").arg(newVal).arg(control_->value()));
	}
	emit setpointRequested(newVal);
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
	setStyleSheet("#styledDialog { background-color: rgb(31,62,125); border: 2px outset white; border-radius: 10px; }  QLabel { color: white; font: bold " AM_FONT_REGULAR_ " \"Helvetica\"; } QPushButton { color: white; border: 1px outset rgb(158,158,158); border-radius: 5px; min-height: 24px; padding: 3px; width: 80px; margin: 3px;} #okButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(191, 218, 178, 255), stop:0.34 rgba(135, 206, 96, 255), stop:1 rgba(65, 157, 0, 255));} #cancelButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(232, 209, 209, 255), stop:0.34 rgba(229, 112, 119, 255), stop:1 rgba(197, 20, 32, 255)); } QDoubleSpinBox { padding: 3px; color: black; font: bold " AM_FONT_REGULAR_ " \"Helvetica\"; border: 1px outset rgb(158,158,158); selection-background-color: rgb(205, 220, 243); selection-color: black;}");// TODO: continue here...

	label_ = new QLabel("New value:");
	label_->setAlignment(Qt::AlignCenter);
	enumNames_ = enumNames;
	isEnum_ = false; //IS THIS THE MISSING PIECE?
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

double AMControlEditorStyledInputDialog::setpoint() const{
	if(!isEnum_)
		return spinBox_->value();
	else
		return comboBox_->currentIndex();
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

void AMControlEditorStyledInputDialog::setDoubleDecimals(int d) {
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
		QToolButton(parent)
{
	setObjectName("AMControlButton");

	control_ = control;
	downValue_ = 1;
	upValue_ = 0;
	programaticToggle_ = false;
	if(control_){
		if(control_->description() != "")
			setText(control_->description());
		else
			setText(control_->name());
	}
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
	QToolButton::setCheckable(checkable);
	onValueChanged(control_->value());
}

void AMControlButton::onValueChanged(double newVal) {
	Q_UNUSED(newVal)
	if(isCheckable()){
		if(control_->value() == downValue_)
			setChecked(true);
		else if(control_->value() == upValue_)
			setChecked(false);
	}
}

void AMControlButton::onClicked(){
	if(isCheckable())
		return;
	control_->move(downValue_);
}

void AMControlButton::onToggled(bool toggled){
	if(programaticToggle_){
		programaticToggle_ = false;
		return;
	}
	if(toggled && !control_->withinTolerance(downValue_))
		control_->move(downValue_);
	else if(!toggled && !control_->withinTolerance(upValue_))
		control_->move(upValue_);
}

void AMControlButton::setHappy(bool happy) {
	Q_UNUSED(happy)
}


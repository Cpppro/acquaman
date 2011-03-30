#include "ui/AMShutterButton.h"
#include <QPainter>
#include <QRadialGradient>
#include <QStyleOption>
#include <QFontMetrics>

AMShutterButton::AMShutterButton(QString title, QString statusPV, QWidget *parent)
	: QPushButton(parent), state_(Between), title_(title)
{
	statePV_ = new AMProcessVariable(statusPV, true, this);
	connect(statePV_, SIGNAL(valueChanged()), this, SLOT(statusUpdate()));

	openPV_ = 0;
	closePV_ = 0;

	setFlat(true);

	// Determining the size of the button.
	QFontMetrics metric(this->font());
	int width = metric.width(title);

	if (width < 40)
		width = 40;

	setMinimumSize(1.4*width, 1.4*width);
}

AMShutterButton::AMShutterButton(QString title, QString statusPV, QString openPV, QString closePV, QWidget *parent)
	: QPushButton(parent), state_(Between), title_(title)
{
	statePV_ = new AMProcessVariable(statusPV, true, this);
	connect(statePV_, SIGNAL(valueChanged()), this, SLOT(statusUpdate()));
	connect(this, SIGNAL(clicked()), this, SLOT(changeState()));

	openPV_ = new AMProcessVariable(openPV, false, this);
	closePV_ = new AMProcessVariable(closePV, false, this);

	setFlat(true);

	// Determining the size of the button.
	QFontMetrics metric(this->font());
	int width = metric.width(title);

	if (width < 40)
		width = 40;

	setMinimumSize(1.4*width, 1.4*width);
}

void AMShutterButton::changeState()
{
	switch(state()){
	case Open:
	case Between:
		closePV_->setValue(1);
		break;
	case Closed:
		openPV_->setValue(1);
	}

	update();
}

void AMShutterButton::statusUpdate()
{
	setEnabled(statePV_->canRead());
	if (openPV_ && closePV_)
		setFlat(!openPV_->canWrite() && !openPV_->canWrite());

	switch(statePV_->getInt()){
	case 1:
		setState(Open);
		break;
	case 4:
		setState(Closed);
		break;
	case 2:
	case 0:
	case 3:
	case 5:
	case 6:
	case 7:
		setState(Between);
		break;
	}

	update();
}

void AMShutterButton::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e)

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	if (!isFlat()){
		// Main button box.  Uses the standand push button style for the basic aspects of the box.
		QStyleOptionButton option;
		option.initFrom(this);
		style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);
	}

	qreal w = width();
	qreal h = height();

	// Draw a black box to act like a case.
	painter.setBrush(QBrush(Qt::black));
	painter.translate(0.5*w-0.4*h, 0.1*h);
	painter.drawRect(QRectF(0, 0, 0.8*h, 0.8*h));

	// Paint the center light.
	QRadialGradient lightGradient(0.45*h, 0.3*h, 0.55*h);
	switch(state()){
	case Open:
		lightGradient.setColorAt(0, QColor(0, 200, 0));
		lightGradient.setColorAt(0.5, QColor(34, 139, 34));
		break;
	case Between:
		lightGradient.setColorAt(0, QColor(255, 255, 0));
		lightGradient.setColorAt(0.5, QColor(200, 200, 0));
		break;
	case Closed:
		lightGradient.setColorAt(0, QColor(255, 0, 0));
		lightGradient.setColorAt(0.5, QColor(200, 0, 0));
		break;
	}
	QBrush lightBrush(lightGradient);
	painter.setBrush(lightBrush);
	painter.drawEllipse(QRectF(0.09*h, 0.09*h, 0.64*h, 0.64*h));

	// Paint on the title.
	QFontMetrics metric(this->font());
	if (state() == Between)
		painter.setPen(Qt::darkBlue);
	else
		painter.setPen(Qt::white);
	painter.drawText(QRectF(0.125*h, 0.25*h, metric.width(title()), metric.height()), title());
}
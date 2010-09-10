#include "AMBeamlineCameraWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QUrl>


AMCrosshairVideoWidget::AMCrosshairVideoWidget(QWidget* parent)
	: AMVideoPlayerWidget(parent) {

	crosshairX_ = 0.5;
	crosshairY_ = 0.5;
	crosshairPen_ = QPen(QColor(Qt::red));
	crosshairVisible_ = true;

	doubleClickInProgress_ = false;
}

void AMCrosshairVideoWidget::paintEvent(QPaintEvent *event) {

	AMVideoPlayerWidget::paintEvent(event);

	if(crosshairVisible_) {
		QRect displayRect = videoDisplayRect_;
		int xCoord = displayRect.left() + displayRect.width()*crosshairX_;
		int yCoord = displayRect.top() + displayRect.height()*crosshairY_;

		QPainter painter(this);
		painter.setPen(crosshairPen_);
		// draw vertical line:
		painter.drawLine(QPointF(xCoord, displayRect.top()), QPointF(xCoord, displayRect.bottom()));
		// draw horizontal line:
		painter.drawLine(QPointF(displayRect.left(), yCoord), QPointF(displayRect.right(), yCoord));
		painter.end();
	}
}


void AMCrosshairVideoWidget::mousePressEvent(QMouseEvent *event) {
	if(event->button() == Qt::LeftButton) {
		emit mousePressed(QPointF(
				double(event->pos().x()-videoDisplayRect_.left())/videoDisplayRect_.width(),
				double(event->pos().y()-videoDisplayRect_.top())/videoDisplayRect_.height()
				));
	}
	AMVideoPlayerWidget::mousePressEvent(event);
}

#include <QApplication>
void AMCrosshairVideoWidget::mouseReleaseEvent(QMouseEvent *event) {

	if(event->button() == Qt::LeftButton) {
		if(doubleClickInProgress_) {
			emit mouseDoubleClicked(QPointF(
					double(event->pos().x()-videoDisplayRect_.left())/videoDisplayRect_.width(),
					double(event->pos().y()-videoDisplayRect_.top())/videoDisplayRect_.height()
					));
			doubleClickInProgress_ = false;
		}
		else {
			emit mouseReleased(QPointF(
					double(event->pos().x()-videoDisplayRect_.left())/videoDisplayRect_.width(),
					double(event->pos().y()-videoDisplayRect_.top())/videoDisplayRect_.height()
					));
		}
	}

	AMVideoPlayerWidget::mouseReleaseEvent(event);
}

void AMCrosshairVideoWidget::mouseDoubleClickEvent(QMouseEvent *event) {
	doubleClickInProgress_ = true;
	AMVideoPlayerWidget::mouseDoubleClickEvent(event);
}




AMBeamlineCameraWidget::AMBeamlineCameraWidget(const QString& cameraName, const QUrl& cameraAddress, QWidget* parent) : QWidget(parent) {

	// Setup UI:
	/////////////////////

	// Layouts: full vertical, with horizontal sub-layout at bottom.
	QVBoxLayout* vl1 = new QVBoxLayout(this);
	QHBoxLayout* hl1 = new QHBoxLayout();

	// VideoWidget takes up main space:
	videoWidget_ = new AMCrosshairVideoWidget(this);
	videoWidget_->setFillBackground(true);

	vl1->addWidget(videoWidget_);

	// add horizontal layout at bottom:
	vl1->addLayout(hl1);

	hl1->addStretch(1);
	QLabel* label = new QLabel("Source");
	hl1->addWidget(label);

	// Add combobox with list of camera names/addresses:
	cameraList_ = new QComboBox();
	hl1->addWidget(cameraList_);

	cameraList_->insertItem(0, cameraName, cameraAddress);
	connect(cameraList_, SIGNAL(activated(int)), this, SLOT(onSourceChanged(int)));
	onSourceChanged( cameraList_->currentIndex() );

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(videoWidget_, SIGNAL(mousePressed(QPointF)), this, SLOT(debugMousePress(QPointF)));
	connect(videoWidget_, SIGNAL(mouseReleased(QPointF)), this, SLOT(debugMouseRelease(QPointF)));
	connect(videoWidget_, SIGNAL(mouseDoubleClicked(QPointF)), this, SLOT(debugMouseDoubleClicked(QPointF)));

}


AMBeamlineCameraWidget::~AMBeamlineCameraWidget()
{
}


#include "AMErrorMonitor.h"

void AMBeamlineCameraWidget::onSourceChanged(int index) {

	videoWidget_->stop();

	QUrl source( cameraList_->itemData(index).toUrl() );

	AMErrorMon::report(AMErrorReport(this, AMErrorReport::Information, 0, QString("Attempting to connect to video server: '%1%2' with user name: '%3' and password: '%4'").arg(source.host()).arg(source.path()).arg(source.userName()).arg(source.password())));

	videoWidget_->openVideoUrl( source.toString() );

	videoWidget_->play();

}

void AMBeamlineCameraWidget::addSource(const QString& cameraName, const QUrl& cameraAddress) {

	cameraList_->addItem(cameraName, QVariant::fromValue(cameraAddress));
}

#ifndef AMTHUMBNAILSCROLLVIEWER_H
#define AMTHUMBNAILSCROLLVIEWER_H

#include <QLabel>
#include <QList>

#include "dataman/AMDbObject.h"
#include "dataman/AMDatabase.h"

/// This widget shows a single thumbnail, either from an AMDbObject or accessed directly from the database. However, by placing the mouse over the thumbnail, one can scroll through a related set of thumbnails.  The thumbnail title and subtitle are super-posed over top of the thumbnail image.

class AMThumbnailScrollViewer : public QLabel
{
	Q_OBJECT
public:
	explicit AMThumbnailScrollViewer(QWidget *parent = 0);

	/// display the thumbnails from a given AMDbObject
	void setSource(AMDbObject* source);
	/// Display thumbnails directly out of a database (from the thumbnail table, with id's given in \c ids)
	void setSource(AMDatabase* db, QList<int> ids);
	/// Display \c count thumbnails directly out of a database (from the thumbnail table, with sequential id's starting at \c startId)
	void setSource(AMDatabase* db, int startId, int count);

	QSize sizeHint() const {
		return QSize(240, 180);
	}

	int heightForWidth(int w) const {
		return w*3/4;
	}

signals:

public slots:

protected:
	QLabel* title_, *subtitle_;
	bool sourceIsDb_;

	AMDbObject* sourceObject_;
	AMDatabase* sourceDb_;
	QList<int> sourceIds_;

	// current thumbnail index:
	int tIndex_;

	void displayThumbnail(AMDbThumbnail t);
	void displayThumbnail(AMDatabase* db, int id);


	void mouseMoveEvent ( QMouseEvent * event );

	/// returns a pixmap (240 x 180) suitable for an invalid/blank background
	static QPixmap invalidPixmap();

};

/// This widget incorporates a thumbnail viewer (AMThumbnailScrollViewer) with a couple lines of caption text
class AMThumbnailScrollWidget : public QWidget {
	Q_OBJECT
public:
	explicit AMThumbnailScrollWidget(const QString& caption1 = QString(), const QString& caption2 = QString(), QWidget* parent = 0);

	/// display the thumbnails from a given AMDbObject
	void setSource(AMDbObject* source) {
		tv_->setSource(source);
	}

	/// Display thumbnails directly out of a database (from the thumbnail table, with id's given in \c ids)
	void setSource(AMDatabase* db, QList<int> ids) {
		tv_->setSource(db, ids);
	}

	/// Display \c count thumbnails directly out of a database (from the thumbnail table, with sequential id's starting at \c startId)
	void setSource(AMDatabase* db, int startId, int count) {
		tv_->setSource(db, startId, count);
	}

public slots:
	void setCaption1(const QString& text) {
		c1_->setText(text);
	}
	void setCaption2(const QString& text) {
		c2_->setText(text);
	}

protected:
	QLabel* c1_, *c2_;
	AMThumbnailScrollViewer* tv_;
};


#include <QGraphicsItem>
#include <QGraphicsLayoutItem>
#include <QDebug>

/// This is a high-performance version of AMThumbnailScrollWidget for use inside the QGraphicsView system
/*! NEEDED!
  You can start by reimplementing important functions: the protected sizeHint() function, as well as the public setGeometry() function. If you want your items to be aware of immediate geometry changes, you can also reimplement updateGeometry().
  */


class AMThumbnailScrollGraphicsWidget : public QGraphicsItem, public QGraphicsLayoutItem {

public:
	explicit AMThumbnailScrollGraphicsWidget(QGraphicsItem* parent = 0);
	virtual ~AMThumbnailScrollGraphicsWidget() {}


	/// This bounding rect is just big enough for the picture box, the text underneath, and some extra room to erase the shadow graphics effect we apply when hover-overed
	virtual QRectF boundingRect() const {
		return QRectF(0,
					  0,
					  width_ + shadowBlurRadius(),
					  width_*3.0/4.0 + textHeight_ + shadowBlurRadius());
	}

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);


	/// display the thumbnails from a given AMDbObject
	void setSource(AMDbObject* source) {
		sourceIsDb_ = false;
		sourceObject_ = source;
		tIndex_ = 0;
		if(sourceObject_)
			displayThumbnail(sourceObject_->thumbnail(tIndex_));
		else
			displayThumbnail(AMDbThumbnail());
	}

	/// Display thumbnails directly out of a database (from the thumbnail table, with id's given in \c ids)
	void setSource(AMDatabase* db, QList<int> ids) {
		sourceIsDb_ = true;
		sourceDb_ = db;
		ids_ = ids;
		tIndex_ = 0;
		displayThumbnail(sourceDb_, tIndex_);
	}

	/// Display \c count thumbnails directly out of a database (from the thumbnail table, with sequential id's starting at \c startId)
	void setSource(AMDatabase* db, int startId, int count) {
		sourceIsDb_ = true;
		sourceDb_ = db;
		ids_.clear();
		for(int i=startId; i<startId+count; i++)
			ids_ << i;
		tIndex_ = 0;
		displayThumbnail(sourceDb_, ids_.at(tIndex_));
	}

	void setWidth(double width) {

		preferredWidth_ = width;
		updateGeometry();
	}

	void setGeometry(const QRectF &rect) {
		prepareGeometryChange();
		setPos(rect.left(), rect.top());
		width_ = rect.width();
		update();
	}

	static double textLineSpacing() { return 2; }
	static double marginLeft() { return 10; }
	static double marginTop() { return 5; }

public:
	void setCaption1(const QString& text) {
		c1_ = text;
	}
	void setCaption2(const QString& text) {
		c2_ = text;
	}

protected:
	/// Some preferences/parameters:
	static double shadowOffsetX() { return 6; }
	static double shadowOffsetY() { return 4; }
	static double shadowBlurRadius() { return 12; }

	double preferredWidth_, width_, textHeight_;
	QPixmap pixmap_;

	/// title and subtitle are written on top of the thumbnail itself.  They are found from the content inside the thumbnail
	QString title_, subtitle_;
	/// captions (c1_, c2_) are written below the thumbnail
	QString c1_, c2_;

	/// index of the current thumbnail (if showing a set a thumbnails)
	int tIndex_;

	/// Indicates that the source for thumbnails is directly out of the database, \c sourceDb_. (If false, the source is from an AMDbObject \c sourceObject_.)
	bool sourceIsDb_;
	/// When sourceIsDb_ = true, this is a pointer to the database that should be queried for thumbnails.
	AMDatabase* sourceDb_;
	/// When sourceIsDb_ = false, this is pointer to the AMDbObject, that can be asked for its thumbnailCount() and thumbnail(int index);
	AMDbObject* sourceObject_;
	/// When sourceIsDb_ = true, this is the set of rows in the thumbnail table to use.
	QList<int> ids_;

	/// Change the view to display this thumbnail object
	void displayThumbnail(AMDbThumbnail t);
	/// Change the view to display the thumbnail data pulled from database \c db at row \c id.
	void displayThumbnail(AMDatabase* db, int id);

	/// returns a pixmap (240 x 180) suitable for an invalid/blank background
	static QPixmap invalidPixmap();


	/// re-implemented from QGraphicsItem to change the thumbnail when the mouse is moved over top
	void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

	/// Re-implemented from QGraphicsItem to be a drag-and-drop source containing the database, table name and id of the object that this thumbnail represents.
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	/// Re-implemented from QGraphicsItem to be a drag-and-drop source containing the database, table name and id of the object that this thumbnail represents.
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	/// Re-implemented from QGraphicsItem to be a drag-and-drop source containing the database, table name and id of the object that this thumbnail represents.
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	/// This is a helper function that creates a new QDrag object and returns a pointer to it.  The QDrag object has the MIME type "text/uri-list" with one URL: 'amd://databaseName/tableName/id', which describes the object represented by this thumbnail.  It also has image data set (MIME type "image/x-") so that the drag icon is visible.  If it's impossible to determine which object this thumbnail represents (for ex: setSource() hasn't been called yet, or was called with an invalid object), this function returns 0.
	QDrag* createDragObject(QWidget* dragSourceWidget);




	QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const {

		Q_UNUSED(which)
		Q_UNUSED(constraint)

		return QSizeF(preferredWidth_, preferredWidth_*3/4 + textHeight_);
	}

};




#endif // AMTHUMBNAILSCROLLVIEWER_H

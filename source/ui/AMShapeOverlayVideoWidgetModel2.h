#ifndef AMSHAPEOVERLAYVIDEOWIDGETMODEL2_H
#define AMSHAPEOVERLAYVIDEOWIDGETMODEL2_H

#include <QGraphicsView>
#include <QWidget>
#include <QObject>
#include <QSizeF>
#include <QMap>

class QGraphicsRectItem;
class AMCrosshairOverlayVideoWidget2;

class AMShapeOverlayVideoWidgetModel2: public QObject
{
    Q_OBJECT
public:
    /// Constructor
    explicit AMShapeOverlayVideoWidgetModel2(QObject *parent = 0);


    /// viewSize and scaled_size used for computing coordinates, get values from AMCrosshairOverlayVideoWidget
    QSizeF viewSize(){return viewSize_;}
    QSizeF scaledSize(){return scaledSize_;}
    void setViewSize(QSizeF viewSize);
    void setScaledSize(QSizeF scaledSize);

    int rectangleListLength(){return index_;}

    QRectF rectangle(int index);


protected:
    QSizeF viewSize_;
    QSizeF scaledSize_;

    QRectF rectangle_;

    QPointF rectangleTopLeft(int index);
    QPointF rectangleBottomRight(int index);

    int index_;

    QMap<int,QRectF> rectangleList_;



    ///
    enum Coordinate { XCOORDINATE,YCOORDINATE};
    /// for ease, create a coordinate transform function
    /// takes a double transforms to where you are actually clicking
    /// as a doubles, must also pass wether it's x or y
    QPointF coordinateTransform(QPointF);

public slots:
    /// start position of rectangle
    void startRectangle(QPointF);

    /// end position of rectangle
    void finishRectangle(QPointF);

    /// delete a rectangle
    void deleteRectangle(QPointF);



};

#endif // AMSHAPEOVERLAYVIDEOWIDGETMODEL2_H

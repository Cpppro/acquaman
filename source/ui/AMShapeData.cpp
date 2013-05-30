#include "AMShapeData.h"



#include <QString>
#include <QVector3D>
#include <QPolygonF>
#include <QDebug>

AMShapeData::AMShapeData()
{
    shape_ = new QPolygonF();
    coordinateIndex_ = -1;

}

AMShapeData::AMShapeData(QPolygonF shape, QString name, QString otherData,  double idNumber)
{
    shape_ = new QPolygonF();
    *shape_ = shape;
    setName(name);
    setOtherData(otherData);
    setIdNumber(idNumber);
    coordinateIndex_ = -1;

}

QPolygonF* AMShapeData::shape()
{
    return shape_;
}

QString AMShapeData::name()
{
    return name_;
}

QString AMShapeData::otherData()
{
    return otherData_;
}

double AMShapeData::idNumber()
{
    return idNumber_;
}

QVector3D AMShapeData::coordinate(int index)
{
    if(validIndex(index))
        return coordinate_[index];
    else
        return QVector3D(0,0,0);
}

double AMShapeData::height()
{
    return height_;
}

double AMShapeData::width()
{
    return width_;
}

double AMShapeData::rotation()
{
    return rotation_;
}

double AMShapeData::tilt()
{
    return tilt_;
}

void AMShapeData::setShape(QPolygonF shape)
{
    *shape_ = shape;
}

void AMShapeData::setName(QString name)
{
    name_ = name;
}

void AMShapeData::setOtherData(QString otherData)
{
    otherData_ = otherData;
}

void AMShapeData::setIdNumber(double idNumber)
{
    idNumber_ = idNumber;
}

void AMShapeData::setCoordinate(QVector3D coordinate, int index)
{
    if(validIndex(index))
        coordinate_[index] = coordinate;
    else qDebug()<<"Failed to set coordinate; invalid index";
}

void AMShapeData::setCoordinateShape(QVector<QVector3D> coordinates, int count)
{
    if(coordinates.isEmpty()) return;
    coordinate_.clear();
    for(int i = 0; i < count; i++)
    {
        if(coordinateIndex_ < i) coordinateIndex_ = i;
        coordinate_<<coordinates[i];
    }
}

void AMShapeData::setHeight(double height)
{
    height_ = height;
}

void AMShapeData::setWidth(double width)
{
    width_ = width;
}

void AMShapeData::setRotation(double rotation)
{
    rotation_ = rotation;
}

void AMShapeData::setTilt(double tilt)
{
    tilt_ = tilt;
}

QVector3D AMShapeData::centerCoordinate()
{
    QVector3D center = QVector3D(0,0,0);
    for(int i = 0; i < 4; i++)//only want the first four points
    {
        center += coordinate(i);
    }
    return center/4.0;
}

void AMShapeData::shift(QVector3D shift)
{
    for(int i = 0; i < 5; i++)
    {
        setCoordinate(coordinate_[i] +  shift,i);
    }
}

void AMShapeData::shiftTo(QVector3D shift)
{
    shift -= centerCoordinate();
    for(int i = 0; i < 5; i++)
    {
        setCoordinate(coordinate_[i] +  shift,i);
    }
}

int AMShapeData::count()
{
    return coordinateIndex_;
}

bool AMShapeData::validIndex(int index)
{
    return (index >= 0 && index <= coordinateIndex_);
}

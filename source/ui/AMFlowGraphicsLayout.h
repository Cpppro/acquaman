#ifndef AMFLOWGRAPHICSLAYOUT_H
#define AMFLOWGRAPHICSLAYOUT_H

#include <QGraphicsLayout>

#include <QtGui/qgraphicslayout.h>

/// This class implements a "flow"-style layout for a QGraphicsWidget. It is taken from the Qt Flow Layout example (graphicsview/flowlayout/flowlayout.h).
 /****************************************************************************
 **
 ** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the examples of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL$
 ** Commercial Usage
 ** Licensees holding valid Qt Commercial licenses may use this file in
 ** accordance with the Qt Commercial License Agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and Nokia.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Nokia gives you certain additional
 ** rights.  These rights are described in the Nokia Qt LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3.0 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU General Public License version 3.0 requirements will be
 ** met: http://www.gnu.org/copyleft/gpl.html.
 **
 ** If you have questions regarding the use of this file, please contact
 ** Nokia at qt-info@nokia.com.
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

 class AMFlowGraphicsLayout : public QGraphicsLayout
 {
 public:
	 AMFlowGraphicsLayout();

	 /// Added by mark.boots@usask.ca to fix crashes when a layout is deleted before the layout items it contains.
	 virtual ~AMFlowGraphicsLayout();
	 /// Added to manually allow setting a width constraint, which will be used whenever the supplied width constraint is < 0:
	 void setWidthConstraint(double widthConstraint) { widthConstraint_ = widthConstraint; updateGeometry();  }

	 inline void addItem(QGraphicsLayoutItem *item);
	 void insertItem(int index, QGraphicsLayoutItem *item);
	 void setSpacing(Qt::Orientations o, qreal spacing);
	 qreal spacing(Qt::Orientation o) const;

	 // inherited functions
	 void setGeometry(const QRectF &geom);

	 int count() const;
	 QGraphicsLayoutItem *itemAt(int index) const;
	 void removeAt(int index);

 protected:


	 QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

 private:
	 qreal doLayout(const QRectF &geom, bool applyNewGeometry) const;
	 QSizeF minSize(const QSizeF &constraint) const;
	 QSizeF prefSize() const;
	 QSizeF maxSize() const;

	 /// This function searches up through the hierachy of parentLayoutItems, finding the first one with a maximum width set (!= -1), and returns that for a width constraint.  If it doesn't find one, it returns -1 (no constraint).
	 double findWidthConstraint(QGraphicsLayoutItem* parent) const;

	 QList<QGraphicsLayoutItem*> m_items;
	 qreal m_spacing[2];

	 double widthConstraint_;
 };

 inline void AMFlowGraphicsLayout::addItem(QGraphicsLayoutItem *item)
 {
	 insertItem(-1, item);
 }

#endif // AMFLOWGRAPHICSLAYOUT_H

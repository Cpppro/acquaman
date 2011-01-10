#ifndef ACQUAMAN_H
#define ACQUAMAN_H

#include <Qt>
#include <QStandardItem>
#include <QList>
#include <QEvent>


/// This namespace contains global definitions for the Acquaman / Dataman framework.

namespace AM {

	/// Application-wide role definitions used in the Qt StandardItem Model/View system
	enum ItemRoles { IdRole = Qt::UserRole+20, DateTimeRole, EndDateTimeRole, WeightRole, LinkRole, PointerRole, PriorityRole, LinePenRole, ModifiedRole, DescriptionRole, WidgetRole, CanCloseRole, UserRole };

	/// Application-wide type definitions used inthe Qt StandardItem Model/View system
	enum ItemTypes { DragDropItem = QStandardItem::UserType + 1, SidebarItem, SidebarHeadingItem };


	// REMOVED: Application-wide QVariant user types
	// enum AcquamanType { IntList = QVariant::UserType + 20, DoubleList };

	/// Acquaman-specific event types
	enum EventType { AcqEvent = QEvent::User+20, VideoResizeEvent };

	/// custom QGraphicsItem types (see qgraphicsitem_cast<>() for more information)
	enum GraphicsItemType { ThumbnailScrollGraphicsWidgetType = 65540 };

	/// Register acquaman-defined types with the QMetaType system. Call on application startup before using these types.
	void registerTypes();

}

class AMAcqEvent : public QEvent{
public:
	AMAcqEvent() : QEvent( (QEvent::Type)AM::AcqEvent)
	{}

	QMap<int, double> dataPackage_;
	QMap<int, QList<double> > spectraPackage_;
};

class AMDbObject;
typedef QList<int> AMIntList;
typedef QList<double> AMDoubleList;
typedef QList<AMDbObject*> AMDbObjectList;

Q_DECLARE_METATYPE(AMIntList);
Q_DECLARE_METATYPE(AMDoubleList);
Q_DECLARE_METATYPE(AMDbObjectList);

#endif // ACQUAMAN_H

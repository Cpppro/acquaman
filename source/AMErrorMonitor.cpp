#include "AMErrorMonitor.h"

#include <QSystemTrayIcon>
#include <QMutableMapIterator>
#include <QDebug>

AMErrorMon* AMErrorMon::instance_ = 0;

AMErrorMon::AMErrorMon() : QObject() {
	qRegisterMetaType<AMErrorReport>("AMErrorReport");
	sicon_ = new QSystemTrayIcon(QIcon(":/utilities-system-monitor.png"), this);
	sicon_->show();


	/// don't display debug notifications by default:
	debugEnabled_ = false;
}


/// Subscribe to all errors from object 'originator'
void AMErrorMon::subscribeToObjectI(const QObject* originator, QObject* notifyMe, const char* errorSlot) {
	/// \test Unit test insertions
	objectSubs_.insertMulti(originator, QPair<QObject*,QString>(notifyMe, errorSlot));

}

/// Subscribe to all errors from this class:
void AMErrorMon::subscribeToClassI(const QString& className, QObject* notifyMe, const char* errorSlot) {
	/// \test Unit test insertions
	classSubs_.insertMulti(className, QPair<QObject*,QString>(notifyMe,errorSlot));
}

/// Subscribe to all errors that have code 'errorCode'
void AMErrorMon::subscribeToCodeI(int errorCode, QObject* notifyMe, const char* errorSlot) {
	/// \test Unit test insertions
	codeSubs_.insertMulti(errorCode, QPair<QObject*,QString>(notifyMe,errorSlot));
}

/// Unsubscribe <notifyMe, errorSlot> from everything.  If errorSlot==0, unsubscribes all of notifyMe's subscriptions.
void AMErrorMon::unsubscribeI(QObject* notifyMe, const char* errorSlot) {

	/// \test unit test removals (all cases)

	// Iterate through all object subscriptions
	QMutableMapIterator<const QObject*, QPair<QObject*,QString> > i(objectSubs_);
	while(i.hasNext()) {
		i.next();
		if( (notifyMe==i.value().first) && (errorSlot==0 || errorSlot==i.value().second) )
			i.remove();
	}

	// Iterate through all className subscriptions:
	QMutableMapIterator<QString, QPair<QObject*,QString> > i2(classSubs_);
	while(i2.hasNext()) {
		if( (notifyMe==i2.value().first) && (errorSlot==0 || errorSlot==i2.value().second) )
			i2.remove();
	}

	// Iterate through all errorCode subscriptions:
	QMutableMapIterator<int, QPair<QObject*,QString> > i3(codeSubs_);
	while(i3.hasNext()) {
		if( (notifyMe==i3.value().first) && (errorSlot==0 || errorSlot==i3.value().second) )
			i3.remove();
	}

}



/// Report an error:
void AMErrorMon::reportI(const AMErrorReport& e) {

	QString className;
	if(e.source && e.source->metaObject())
		className = e.source->metaObject()->className();

	QString reportMessage = QString("in [%1]: %2 (code %3)").arg(className).arg(e.description).arg(e.errorCode);


	// Chapter 0: If we're in debug mode, throw a qDebug() for everything:
	if(debugEnabled_) {
		switch(e.level) {
		case AMErrorReport::Debug:
			qDebug() << "Debug:" << reportMessage;
			break;
		case AMErrorReport::Information:
			qDebug() << "Information:" << reportMessage;
			break;
		case AMErrorReport::Alert:
			qDebug() << "Alert:" << reportMessage;
			break;
		case AMErrorReport::Serious:
			qDebug() << "Serious:" << reportMessage;
			break;
		}
	}


	// Chapter 1: Emit signals, as long as it's not a debug message and debug is disabled.

	if( e.level == AMErrorReport::Debug && !debugEnabled_)
		; // do nothing
	else
		emit error(e);

	switch(e.level) {
		case AMErrorReport::Information:
			emit information(e);
			break;
		case AMErrorReport::Alert:
			emit alert(e);
			break;
        case AMErrorReport::Serious:
			emit serious(e);
			sicon_->showMessage(QString("Error in %1: (%2)").arg(className).arg(e.errorCode), e.description, QSystemTrayIcon::Critical, 5000);
			break;
		case AMErrorReport::Debug:
			if(debugEnabled_)
				emit debug(e);
		break;
	}

	// Chapter 2: go through various subscriptions:


	// This looks complicated.  It's a list of object pointers and corresponding slotNames that need to be called.  We fill it using the subscription records.
	QList<QPair<QObject*, QString> > targets;

	// Object subscriptions:
	targets = objectSubs_.values(e.source);
	// Classname subscriptions:
	if(e.source)
		targets.append( classSubs_.values(e.source->metaObject()->className()) );
	// Errorcode subscriptions:
	targets.append( codeSubs_.values(e.errorCode) );


	// Invoke the slot in each object (We use this to be thread-safe... The notification is delivered just like a slot call.
	// If the object is in the same thread, it's delivered right away.  If in different threads, it's queued into the event loop.
	QPair<QObject*, QString> target;
	foreach(target, targets)
		target.first->metaObject()->invokeMethod( target.first, target.second.toAscii().data(), Q_ARG(AMErrorReport, e));


}

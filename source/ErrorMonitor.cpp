#include "ErrorMonitor.h"

#include <QSystemTrayIcon>

ErrorMon* ErrorMon::instance_ = 0;

ErrorMon::ErrorMon() : QObject() {
	qRegisterMetaType<ErrorReport>("ErrorReport");
	sicon_ = new QSystemTrayIcon();
	sicon_->show();
}


/// Subscribe to all errors from object 'originator'
void ErrorMon::subscribeToObjectI(QObject* originator, QObject* notifyMe, const char* errorSlot) {
	// Remember that this

}

/// Subscribe to all errors from this class:
void ErrorMon::subscribeToClassI(const QString& className, QObject* notifyMe, const char* errorSlot) {

}

/// Subscribe to all errors that have code 'errorCode'
void ErrorMon::subscribeToCodeI(int errorCode, QObject* notifyMe, const char* errorSlot) {

}

/// Unsubscribe from everything:
void ErrorMon::unsubscribeI(QObject* notifyMe) {

}



/// Report an error:
void ErrorMon::reportI(const ErrorReport& e) {
	// Chapter 1: Emit signals:
	emit error(e);

	QString className = "[]";

	switch(e.level) {
	case ErrorReport::Information: emit information(e);
		break;
	case ErrorReport::Alert: emit alert(e);
		break;
	case ErrorReport::Serious:
		emit serious(e);
		if(e.source)
			className = e.source->metaObject()->className();
		sicon_->showMessage(QString("Error in %1: (%2)").arg(className).arg(e.errorCode), e.description, QSystemTrayIcon::Critical, 5000);
		break;
	case ErrorReport::Debug: emit debug(e);
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
	for(int i=0; i< targets.count(); i++) {
		QPair<QObject*, QString>& target = targets[i];
		target.first->metaObject()->invokeMethod( target.first, target.second.toAscii().data(), Q_ARG(ErrorReport, e));
	}
	

}

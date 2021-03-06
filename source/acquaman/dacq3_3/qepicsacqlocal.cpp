// $Header: qepicsacq.cpp 1.5.1.1 2007/05/19 12:25:35CST Glen Wright (wrightg) Exp Glen Wright (wrightg)(2007/07/18 12:03:40CST) $
// Copyright Canadian Light Source, Inc. All rights reserved.
//
// Description:


#include "qepicsacqlocal.h"
#include "displayAlias.h"
#include "qregexp.h"
#include "qtimer.h"

// allows the name to be seen in the parameter list, but removes the 'unused variable' warning when compiling
#define UNUSED(parameter)

//
// callback routines are static and then call the method for the class.
//

static acqFactory registerMe("local", QEpicsAcqLocal::buildMe);

static bool tryCopy(char **c_ptr, const QString &str);
static bool trySet( double *var, double val);
static acqControl_t * setupControlLine(acqScan_t *, int);

// return a base class pointer for this derived class
QEpicsAcqClass *
QEpicsAcqLocal::buildMe( QWidget *parent)
{
	return new QEpicsAcqLocal(parent);
}

void
QEpicsAcqLocal::localShowMode(acqMaster_t *m, acqState mode)
{
	QEpicsAcqLocal *ptr;

	ptr = (QEpicsAcqLocal *)m->user_mode_data;
	ptr->showMode( mode);
}

void
QEpicsAcqLocal::localShowProgress( acqMaster_t *m, const char *status, double completion)
{
	QEpicsAcqLocal *ptr;
	ptr = (QEpicsAcqLocal *)m->user_mode_data;
	ptr->showCompletion( status, (int)(completion*100) );
}

void
QEpicsAcqLocal::localMessageOut(acqMaster_t *master, const char *fmt, ...)
{
	va_list ap;
	char msg[100];
	QEpicsAcqLocal *ptr;
	ptr = (QEpicsAcqLocal *)master->user_mode_data;
	if( ptr == NULL)
		return;
	va_start( ap, fmt);
	vsnprintf( msg, sizeof msg, fmt, ap);
	ptr->outputMessage(QString(msg) );
	va_end(ap);
}

void
QEpicsAcqLocal::localStatusOut(acqMaster_t *master, const char *fmt, ...)
{
	va_list ap;
	char msg[100];
	QEpicsAcqLocal *ptr;
	ptr = (QEpicsAcqLocal *)master->user_mode_data;
	if( ptr == NULL)
		return;
	va_start( ap, fmt);
	vsnprintf( msg, sizeof msg, fmt, ap);
	ptr->outputOnelineMessage(QString(msg) );
	va_end(ap);
}

void
QEpicsAcqLocal::localMessageBoxClear( acqMaster_t *master)
{
	QEpicsAcqLocal *ptr;
	ptr = (QEpicsAcqLocal *)master->user_mode_data;
	if( ptr == NULL)
		return;
	ptr->clearMessageBox();
}

void
QEpicsAcqLocal::localOnStop( acqMaster_t *master)
{
	QEpicsAcqLocal *ptr;
	ptr = (QEpicsAcqLocal *)master->user_mode_data;
	if( ptr == NULL)
		return;
	ptr->signalOnStop = 1;
	ptr->running = 0;
}

//
// class QEpicsAcqLocal methods
//
// Removed the NULL argument, QString can't instantiate from NULL (David Chevrier, Oct 27 2011)
QEpicsAcqLocal::QEpicsAcqLocal( QWidget *parent) : QEpicsAcqClass(parent), running(0), config_file()
{
	signalOnNewConfig = 0;
	signalOnPause = 0;
	signalOnStop = 0;
	signalOnStart = 0;
	signalSendStatus = 0;
	signalSendCompletion = 0;
	signalNextOutputFile = 0;
	signalChangeRunNumber = 0;
	signalSendMessage = 0;
	signalSendOnelineMessage = 0;
	signalOnState = 0;
	signalSendMessageBoxClear = 0;

	signalOnHandlerSignal = 0;	//MB: Fixed initialization bug Nov. 2011. These two lines are in other versions of the dacq, but were missing here.
	signalOnNewProperty = 0;

	master = new_acqMaster();		// used for life of this widget
	//
	// circular reference: note that this is used by the putMode
	// function and also the ShowProgress function
	//
	master->user_mode_data = (void *) this;	// circular reference
	acqSetPutMode( localShowMode, master);
	acqSetStatusMessage( localStatusOut, master);
	acqSetMessageAdd( localMessageOut, master);
	acqSetShowProgress( localShowProgress, master);
	acqSetMessageBoxClear( localMessageBoxClear, master);
	acqSetOnStop( localOnStop, master);
	connect( &threadTimer, SIGNAL( timeout() ), this, SLOT( onThreadTimer() ) );
	connect( &monitorTimer, SIGNAL(timeout() ), this, SLOT( onMonitorTimer() ) );
	threadTimer.start(100);		// 10hz max callback rate.
}

//
// check which signals are requested for emit()
void
QEpicsAcqLocal::onThreadTimer()
{
	threadTimerLock.lock();
	if( signalOnNewConfig) {
		emit onNewConfig();
		signalOnNewConfig = FALSE;
	}
	if( signalOnPause) {
		emit onPause(argOnPause);
		signalOnPause = FALSE;
	}
	if( signalOnStart) {
		emit onStart();
		signalOnStart = FALSE;
	}
	if( signalOnStop) {
		emit onStop();
		signalOnStop = FALSE;
	}
	if( signalSendStatus) {
		emit sendStatus(argSendStatus);
		signalSendStatus = FALSE;
	}
	if( signalSendCompletion) {
		emit sendCompletion(argSendCompletion);
		signalSendCompletion = FALSE;
	}
	if( signalChangeRunNumber) {
		emit changeRunNumber(argChangeRunNumber);
		signalChangeRunNumber = FALSE;
	}
	if( signalSendMessageBoxClear) {
		emit sendMessageBoxClear();
		signalSendMessageBoxClear = FALSE;
	}
	if( signalSendMessage) {
		emit sendMessage(argSendMessage);
		argSendMessage = "";
		signalSendMessage = FALSE;
	}
	if( signalSendOnelineMessage) {
		emit sendOnelineMessage(argSendOnelineMessage);
		signalSendOnelineMessage = FALSE;
	}
	if( signalOnState) {
		emit onState(argOnState);
		signalOnState = FALSE;
	}
	if( signalOnHandlerSignal) {
		emit onHandlerSignal( argOnHandlerFrom, argOnHandlerSignal, NULL);
		signalOnHandlerSignal = FALSE;
	}

	if( signalOnNewProperty)
	{
		emit newHandlerProperty( argPropertyName);
		signalOnNewProperty = FALSE;
	}

	threadTimerLock.unlock();
}

void QEpicsAcqLocal::onMonitorTimer()
{
	if( acqMonitor_status(master) == 0)
		monitorTimer.stop();
}

void QEpicsAcqLocal::outputHandlerRegister( eventDataHandler *evh, const acqKey_t key)
{
	evh->regKey = (registrarKey_t) this;
	evh->statusMessage_cb = statusMessage;
	evh->sendHandlerSignal_cb = getHandlerSignal;
	acqRegisterOutputHandler( master, key, (eventDataHandler *)evh);
}

int QEpicsAcqLocal::statusMessage(registrarKey_t key, const char *message)
{
	QEpicsAcqLocal *qealp = (QEpicsAcqLocal *)key;
	if (qealp == NULL || qealp->master == NULL)
		return -1;
	qealp->master->messageAdd( qealp->master, message);
	return 0;
}

void
QEpicsAcqLocal::outputHandlerRemove(acqKey_t key)
{
	acqRemoveOutputHandler( master, key);
}

// PRIVATE

//
// display status
//
void
QEpicsAcqLocal::showCompletion( const QString status, int completion)
{
	if( status != lastStatus)
	{
		lastStatus = status;
		threadTimerLock.lock();
		// emit sendStatus(status);
		argSendStatus = status;
		signalSendStatus = TRUE;
		threadTimerLock.unlock();
	}
	if( completion != lastCompletion)
	{
		lastCompletion = completion;
		threadTimerLock.lock();

		// emit sendCompletion(completion);
		argSendCompletion = completion;
		signalSendCompletion = TRUE;

		threadTimerLock.unlock();
	}
}

// PROPERTY
// return the config file name
QString
QEpicsAcqLocal::ConfigFile() const
{
	return config_file;
}
//
// SLOT
// load up a configuration file
void
QEpicsAcqLocal::setConfigFile( const QString &cfname)
{
	if( running)		// software fails miserably if an attempt is made to rebuild the
		return;		// scan configuration while tasks are runnning!

	if( cfname.isEmpty() )	// can't delete by setting a null file name.
		return;
	char expand[512];
	config_file = cfname;
	macro_expand(cfname.toAscii().constData(), expand, NULL);
	acq_file_load( expand, master);
	showMode(AS_OFF);
	threadTimerLock.lock();
	// emit onNewConfig();
	signalOnNewConfig = TRUE;
	threadTimerLock.unlock();
}


// SLOT
// start an acquisition running
void
QEpicsAcqLocal::Start()
{
	if( running || master->globalState != AS_OFF)
		return;
	acqMonitor_init(master);
	running = TRUE;
	showMode(AS_STANDBY);
	if( Standby_mode( master) != 1)
	{
		running = FALSE;
		showMode(AS_FAULT);
		return;
	}
	if( Run_mode( master) != 0)	// note: Run_mode() currently has no error return!
	{
		running = FALSE;
		showMode(AS_FAULT);
		return;
	}
	threadTimerLock.lock();
	// emit onStart();
	signalOnStart = TRUE;
	threadTimerLock.unlock();

	showMode(AS_RUN);

	monitorTimer.start(master->MsecDelay);

}

//
// SLOT
// pause/resume the running acquisition
void
QEpicsAcqLocal::Pause( int mode)
{
	if( !running)
		return;
	master->globalPause = mode;
	if( mode)
		showMode( AS_STANDBY);
	else
		showMode( AS_RUN);

	threadTimerLock.lock();
	// emit onPause(mode);
	argOnPause = mode;
	signalOnPause = TRUE;
	threadTimerLock.unlock();
}
// SLOT
// stop the active acquisition
void
QEpicsAcqLocal::Stop()
{
	master->globalPause = 0;
	master->globalShutdown = 1;
	showMode(AS_OFF);
}

//
// SLOT
// start or stop a scan from a toogle signal
void
QEpicsAcqLocal::Toggle(bool state)
{
	if( state == TRUE)
		Start();
	else
		Stop();
}
//
// SLOT
// set a string variable to a value
void
QEpicsAcqLocal::setVariable( const QString &varName, const QString &value)
{
	setVariable(varName.toAscii().constData(), value.toAscii().constData() );
}

//
// SLOT
// set a double variable to a value
void
QEpicsAcqLocal::setVariable(  const QString &varName, const double value)
{
	QString str;
	str.sprintf("%.12g", value);
	setVariable(varName.toAscii().constData(), str.toAscii().constData() );
}

//
// SLOT
// Effect: change the macro expansion referenced by a control entry to the named variable.
//
void
QEpicsAcqLocal::defVariable(  const QString &varName, const QString &scan, const QString &type, int varIndex)
{
	acqScan_t *sc;
	char **nameAddr = NULL;
	if( master == NULL)
		return;


	sc = lookup_acqScan( scan.toAscii().constData(), master);
	if( sc == NULL)
		return;
	if( type == "first" || type == "start" )
		nameAddr = &sc->acqControlList[varIndex].startMacro;
	else if( type == "delta")
		nameAddr = &sc->acqControlList[varIndex].deltaMacro;
	else if( type == "final" || type == "last" || type == "end" )
		nameAddr = &sc->acqControlList[varIndex].finalMacro;
	if(nameAddr)
	{
		QString macro( "$(" + varName + ")");
		if( *nameAddr)
			free(*nameAddr);
		*nameAddr = strdup(macro.toAscii().data() );
	}

}

double
QEpicsAcqLocal::getValue(char *name, char *field, int index)
{
	acqScan_t *sc;

	sc = lookup_acqScan( name, master);
	if( sc == NULL || index < 0)
		return 0;

	if( index >= sc->numControlPV)
		return 0;

	if( strcmp(field, "first") == 0 || strcmp(field, "start") == 0)
		return sc->acqControlList[index].startVal;

	if( strcmp(field, "delta") == 0)
		return sc->acqControlList[index].deltaVal;

	if( strcmp(field, "end") == 0 || strcmp(field, "final") == 0)
		return sc->acqControlList[index].finalVal;

	return 0;
}

//
// SLOT
// send a message
void
QEpicsAcqLocal::outputMessage(const QString &str)
{
	threadTimerLock.lock();
	// emit sendMessage(str);
	if( argSendMessage.isEmpty() )
		argSendMessage = str;
	else
		argSendMessage += "\n" + str;
	signalSendMessage = TRUE;

	threadTimerLock.unlock();
}

// SLOT
// send a status string
void
QEpicsAcqLocal::outputOnelineMessage( const QString &str)
{
	threadTimerLock.lock();
	// emit sendOnelineMessage(str);
	argSendOnelineMessage = str;
	signalSendOnelineMessage = TRUE;
	threadTimerLock.unlock();
}

// SLOT
// send a request to clear
void
QEpicsAcqLocal::clearMessageBox()
{
	threadTimerLock.lock();
	// emit sendOnelineMessage(str);
	signalSendMessageBoxClear = TRUE;
	threadTimerLock.unlock();
}
//
// sets a named field for a named scan to a value. Returns 1 on success, 0 on failure
//
int
QEpicsAcqLocal::setValue(char *name, char*field, int index, double value)
{
	acqScan_t *sc;

	sc = lookup_acqScan( name, master);
	if( sc == NULL || index < 0)
		return 0;

	if( index >= sc->numControlPV)
		return 0;

	if( strcmp(field, "start") == 0)
		sc->acqControlList[index].startVal = value;

	else
	if( strcmp(field, "delta") == 0)
		sc->acqControlList[index].deltaVal = value;

	else
	if( strcmp(field, "end") == 0)
		sc->acqControlList[index].finalVal = value;
	else
		return 0;

	return 1;
}

int
QEpicsAcqLocal::setValue(char *name, char *field, double value)
{
	return setValue(name, field, 0, value);

}

void QEpicsAcqLocal::setRuns(int runCount)           // sets the number of runs
{
	master->acqRunMax = runCount;
}

int QEpicsAcqLocal::getRuns()                        // returns the current run when running, o.w. the number of runs
						// to be performed.
{
	if( running)
		return master->acqRuns;
	else
		return master->acqRunMax;

	return 0;
}

int QEpicsAcqLocal::pvIndex(const char *event, const char *pvname)
{
	// MB:Fixed memory bug Nov. 2011. Previously:
//	QString alternate = NULL;
//	displayAlias *dap = displayAlias::find(pvname);
//	if( dap)
//		alternate = dap->getPvName();
//	if( !alternate.isEmpty() )
//		pvname = alternate.toAscii().constData();
//	return getPVcolumn(event, pvname, master);
	// BUG: In case of alternate, pvname does not point to valid data anymore; the QByteArray returned by toAscii() is gone out of scope.

	QString alternate;
	displayAlias *dap = displayAlias::find(pvname);
	if( dap)
		alternate = dap->getPvName();
	if(!alternate.isEmpty() )
		return getPVcolumn(event, alternate.toAscii().constData(), master);
	else
		return getPVcolumn(event, pvname, master);
}

const char *
QEpicsAcqLocal::getVariable( const char *name)
{
	macroTable *mt = find_macro( name, NULL);
	if( name)
		return mt->macroVal;
	return NULL;
}

int QEpicsAcqLocal::setVariable( const char *name, const char *value)
{
	// MB: Fixing memory bug Nov. 2011. Previously:
//	const char *alternate = NULL;

//	if( name == NULL || value == NULL)
//		return -1;

//	//
//	// huge assumption: if we're setting a macro to a value that is also
//	// in the alias table, then we don't want the alias, we really want the
//	// pv name of the alias. Mostly true.
//	//
//	displayAlias *dap = displayAlias::find(value);
//	if( dap)
//		alternate = dap->getPvName().toAscii().constData(); // BUG: the string data pointed to by alternate is not valid after this line: the QByteArray returned by toAscii() goes out of scope.

//	// printf("setVariable(%s,%s,%s)\n", name, value, alternate?alternate:"(NULL)");
//	if( alternate)
//		value = alternate;

//	//
//	// if this variable exists, update the value
//	//
//	macroTable *ptr = find_macro( name, NULL);
//	if( ptr)
//		set_macroValue(ptr, value);

//	return 0;

	const char *alternate = NULL;
	QByteArray alternateByteArray;

	if( name == NULL || value == NULL)
		return -1;

	//
	// huge assumption: if we're setting a macro to a value that is also
	// in the alias table, then we don't want the alias, we really want the
	// pv name of the alias. Mostly true.
	//
	displayAlias *dap = displayAlias::find(value);
	if( dap) {
		alternateByteArray = dap->getPvName().toAscii();
		alternate = alternateByteArray.constData();
	}

	// printf("setVariable(%s,%s,%s)\n", name, value, alternate?alternate:"(NULL)");
	if(alternate)
		value = alternate;

	//
	// if this variable exists, update the value
	//
	macroTable *ptr = find_macro( name, NULL);
	if( ptr)
		set_macroValue(ptr, value);

	return 0;
}

void
QEpicsAcqLocal::showMode(acqState mode)
{
	threadTimerLock.lock();
	argOnState = curState(mode);
	signalOnState = TRUE;
	threadTimerLock.unlock();
}

acqState
QEpicsAcqLocal::getState()
{
	return AS_OFF;
}

int
QEpicsAcqLocal::addControlLine(char*)
{
	return 0;
}
bool
QEpicsAcqLocal::deleteControl(char*)
{
	return 0;
}
bool
QEpicsAcqLocal::deleteControlLine(char*, int)
{
	return 0;
}
int
QEpicsAcqLocal::getSeqNo()
{
	return 0;
}
void
QEpicsAcqLocal::setSeqNo(int)
{
}
void
QEpicsAcqLocal::outFileChanged()
{
}

int
QEpicsAcqLocal::getHandlerSignal( registrarKey_t key, acqKey_t from, unsigned int signal, const void *data)
{
	QEpicsAcqLocal *ptr = (QEpicsAcqLocal *) key;

	if( ptr->threadTimerLock.tryLock() == FALSE)
	{
		// the likelyhood of a deadlock situation is small. To be zero, we would have to ensure
		// that every thread that calls the lock would not wait for a lock here to return.
		return 0;
	}

	// propagate broadcast signals to other registered handlers.
	if( acqGetSignalBroadcast( signal ) != 0)
		acqHandlerSignal( ptr->master, from, signal, data);

	if( signal == acqBaseOutput_NewProperty)
	{
		ptr->signalOnNewProperty = TRUE;
		ptr->argPropertyName = (char *)data;
		ptr->threadTimerLock.unlock();
		return 0;
	}

	ptr->signalOnHandlerSignal = TRUE;
	ptr->argOnHandlerFrom = from;
	ptr->argOnHandlerSignal = signal;
	ptr->threadTimerLock.unlock();
	return 0;
}

QEpicsAcqLocal::~QEpicsAcqLocal()
{
}

bool QEpicsAcqLocal::setControlLine(const char* scan, int ctl_idx, const QString &varName, double first, double last, double increment)
{
	acqScan_t *scp;
	acqControl_t *ctlp;
	scp = lookup_acqScan(scan, master);
	if( scp == NULL)		// scan doesn't exist?
		return 0;

	ctlp = setupControlLine( scp, ctl_idx);

	tryCopy( &ctlp->controlPV, varName);
	trySet( &ctlp->startVal, first);
	trySet( &ctlp->deltaVal, increment);
	trySet( &ctlp->finalVal, last);

	return 0;
}

bool QEpicsAcqLocal::setControlLine(const QString&scan, int ctl_idx, const QString &varName, double first, double last, double increment)
{
	return setControlLine( scan.toAscii().data(), ctl_idx, varName, first, last, increment);
}

static bool tryCopy(char **c_ptr, const QString &str)
{
	if( str.isNull() )	// if a NULL entry, then don't do any work
		return 1;
	if( *c_ptr != NULL)
		free(*c_ptr);
	*c_ptr = NULL;
	if( str.isEmpty())
		return 1;
	*c_ptr = strdup( str.toAscii().data() );
	return 1;

}

static acqControl_t * setupControlLine(acqScan_t *scp, int ctl_idx)
{
	acqControl_t *ctlp;
	if( ctl_idx < 0)		// inserting a new control entry
	{
		memmove( &scp->acqControlList[1], &scp->acqControlList[0], sizeof( *ctlp) * scp->numControlPV);
		scp->numControlPV++;
		ctlp = &scp->acqControlList[0];
		memset( ctlp, '\0', sizeof *ctlp);
	}
	else
	if( ctl_idx >= scp->numControlPV)	// appending a new control entry
	{
		addScanControl(scp, NULL);
		ctlp = &scp->acqControlList[scp->numControlPV-1];
	}
	else
		ctlp = &scp->acqControlList[ctl_idx];

	return ctlp;
}

bool QEpicsAcqLocal::setControlLine(const char* scan, int ctl_idx, const QString &varName, const QString& first, const QString& last, const QString& increment)
{
	acqScan_t *scp;
	acqControl_t *ctlp;
	scp = lookup_acqScan(scan, master);
	if( scp == NULL)		// scan doesn't exist?
		return 0;

	ctlp = setupControlLine( scp, ctl_idx);

	tryCopy( &ctlp->controlPV, varName);
	tryCopy( &ctlp->startMacro, first);
	tryCopy( &ctlp->deltaMacro, increment);
	tryCopy( &ctlp->finalMacro, last);

	return 0;
}

static bool trySet( double *var, double val)
{

	if( val == NAN)
		return 0;
	*var = val;
	return 1;
}

bool QEpicsAcqLocal::setControlLine(const QString&scan, int ctl_idx, const QString &varName, const QString& first, const QString& last, const QString& increment)
{
	return setControlLine( scan.toAscii().data(), ctl_idx, varName, first, last, increment);
}

QStringList QEpicsAcqLocal::getScanID()
{
	return QStringList();
}
bool QEpicsAcqLocal::setEventLine(char*, int, const QString&)
{
	return 0;
}
bool QEpicsAcqLocal::setEventLine(const QString&, int, const QString&)
{
	return 0;
}
QStringList QEpicsAcqLocal::getEventID()
{
	return QStringList();
}
void QEpicsAcqLocal::outputHandlerRegister(const QString&, const QString&)
{
	return;
}

// $Log: qepicsacq.cpp  $
// Revision 1.5.1.1 2007/05/19 12:25:35CST Glen Wright (wrightg)
// Duplicate revision

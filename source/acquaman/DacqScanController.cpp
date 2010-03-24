#include "DacqScanController.h"
#include <qdebug.h>

DacqScanController::DacqScanController(QObject *parent)
{
    running_ = FALSE;
    paused_ = FALSE;
    cancelled_ = FALSE;
    QEpicsAcqLocal *lAcq = new QEpicsAcqLocal((QWidget*)parent);
    advAcq_ = new QEpicsAdvAcq(lAcq);
    connect(advAcq_, SIGNAL(onStart()), this, SLOT(onStart()));
    connect(advAcq_, SIGNAL(onStop()), this, SLOT(onStop()));
    connect(advAcq_, SIGNAL(onPause(int)), this, SLOT(onPause(int)));

    /*
    qDebug() << "Start of dacqscancontroller constructor";

    acqMaster_t *master;
    master = new_acqMaster();
    acq_file_load("myScan.cfg", master);
    acqScan_t *sp = first_acqScan(master);
//    getValue(sp->scanName, "start", x)
    qDebug() << "with values of " << sp->acqControlList[0].startVal << " " << sp->acqControlList[0].deltaVal << " " << sp->acqControlList[0].finalVal;
    if( Standby_mode(master) == 1 && Run_mode(master) == 0)
        startMonitorTask(master);

    QEpicsAcqLocal *lAcq = new QEpicsAcqLocal();
    QEpicsAdvAcq *myAcq = new QEpicsAdvAcq(lAcq);
    myAcq->setConfigFile("myScan.cfg");
    acqBaseOutput *abop = acqOutputHandlerFactory::new_acqOutput("SimpleText", "File");
    if( abop)
    {
        acqRegisterOutputHandler( myAcq->getMaster(), (acqKey_t) abop, &abop->handler);                // register the handler with the acquisition
        abop->setProperty( "File Template", "daveData.%03d.dat");                           // set the file name to be recorded to
    }

    for(int x = 0; x < myAcq->getNumRegions(); x++)
        qDebug() << "Start is " << myAcq->getStrStart(x) << " delta is " << myAcq->getStrDelta(x) << " end is " << myAcq->getStrEnd(x);
    myAcq->setStart(0, 27);
    myAcq->setDelta(0, 0.1);
    myAcq->setEnd(0, 28);
    for(int x = 0; x < myAcq->getNumRegions(); x++)
        qDebug() << "Start is " << myAcq->getStrStart(x) << " delta is " << myAcq->getStrDelta(x) << " end is " << myAcq->getStrEnd(x);
    myAcq->addRegion(1, 28.3, 0.3, 31, 1);
    for(int x = 0; x < myAcq->getNumRegions(); x++)
        qDebug() << "Start is " << myAcq->getStrStart(x) << " delta is " << myAcq->getStrDelta(x) << " end is " << myAcq->getStrEnd(x);
    myAcq->Start();

    qDebug() << "end of dacqscancontroller constructor";
    */
}

/// Sets a new scan configuration
void DacqScanController::newConfigurationLoad(ScanConfiguration &cfg)
{
}

/// Cancel scan if currently running or paused
void DacqScanController::cancel()
{
    advAcq_->Stop();
    cancelled_ = TRUE;
    emit cancelled();
}

void DacqScanController::onStart()
{
    running_ = TRUE;
    emit started();
}

void DacqScanController::onStop()
{
    running_ = FALSE;
    if(cancelled_)
        cancelled_ = FALSE;
    else
        emit finished();
}

void DacqScanController::onPause(int mode)
{
    if(mode == 0){
        paused_ = TRUE;
        emit paused();
    }
    else if(mode == 1){
        paused_ = FALSE;
        emit resumed();
    }
}

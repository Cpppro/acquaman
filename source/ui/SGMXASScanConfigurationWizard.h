/*
Copyright 2010, 2011 Mark Boots, David Chevrier.

This file is part of the Acquaman Data Acquisition and Management framework ("Acquaman").

Acquaman is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Acquaman is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Acquaman.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef SGMXASSCANCONFIGURATIONWIZARD_H
#define SGMXASSCANCONFIGURATIONWIZARD_H

#include <QWizard>
#include <QWizardPage>
#include "AMControlSetView.h"
#include "ui/AMControlOptimizationView.h"
#include "AMDetectorView.h"
#include "AMXASRegionsView.h"
#include "AMRegionsLineView.h"
#include "acquaman/SGM/SGMXASScanConfiguration.h"
#include "acquaman/SGM/SGMXASDacqScanController.h"

class SGMXASScanConfigurationIntroWizardPage;
class AMXASRegionsWizardPage;
class SGMFluxResolutionWizardPage;
class AMControlSetWizardPage;
class AMDetectorSetWizardPage;
class SGMXASScanConfigurationReviewWizardPage;

class SGMXASScanConfigurationWizard : public QWizard
{
Q_OBJECT
public:
	explicit SGMXASScanConfigurationWizard(SGMXASScanConfiguration *sxsc, AMOldDetectorInfoSet *cfgDetectorInfoSet, QWidget *parent = 0);

	void accept();

signals:
	void scanControllerReady(AMScanController *scanController);
	void scanStartRequested();

protected:
	SGMXASScanConfiguration *cfg_;
	AMOldDetectorInfoSet *cfgDetectorInfoSet_;

	SGMXASScanConfigurationIntroWizardPage *introPage;
	AMXASRegionsWizardPage *regionsPage;
	SGMFluxResolutionWizardPage *fluxResolutionPage;
	AMControlSetWizardPage *trackingPage;
	AMDetectorSetWizardPage *detectorsPage;
	SGMXASScanConfigurationReviewWizardPage *reviewPage;
};

class SGMXASScanConfigurationIntroWizardPage : public QWizardPage
{
	Q_OBJECT
public:
	SGMXASScanConfigurationIntroWizardPage(QString title, QString subTitle, QWidget *parent = 0);

protected slots:
	void resizeEvent(QResizeEvent *);

protected:
	QGridLayout *gl_;
	QLabel *textLabel_;
};

class AMXASRegionsWizardPage : public QWizardPage
{
	Q_OBJECT
public:
	AMXASRegionsWizardPage(AMXASRegionsList *regionsList, QString title, QString subTitle, QWidget *parent = 0);

protected slots:
	void resizeEvent(QResizeEvent *);

protected:
	AMXASRegionsList* regionsList_;

	AMXASRegionsView *regionsView_;
	AMRegionsLineView *regionsLineView_;
	QGridLayout *gl_;
	QLabel *textLabel_;
};

class SGMFluxResolutionWizardPage : public QWizardPage
{
	Q_OBJECT
public:
	SGMFluxResolutionWizardPage(AMControlOptimizationSet *fluxResolutionSet, AMXASRegionsList* regionsList, QString title, QString subTitle, QWidget *parent = 0);

public slots:
	void onRegionsUpdate();

protected slots:
	void resizeEvent(QResizeEvent *);

protected:
	AMControlOptimizationSet *fluxResolutionSet_;
	AMXASRegionsList* regionsList_;

	AMCompactControlOptimizationSetView *fluxResolutionView_;
	QGridLayout *gl_;
	QLabel *textLabel_;
};

class AMControlSetWizardPage : public QWizardPage
{
	Q_OBJECT
public:
	AMControlSetWizardPage(AMControlSet *trackingSet, QString title, QString subTitle, QWidget *parent = 0);

protected slots:
	void resizeEvent(QResizeEvent *);

protected:
	AMControlSet *trackingSet_;

	AMOldControlSetView *trackingView_;
	QGridLayout *gl_;
	QLabel *textLabel_;
};

class AMDetectorSetWizardPage : public QWizardPage
{
	Q_OBJECT
public:
	AMDetectorSetWizardPage(AMOldDetectorInfoSet *detectorSet, AMOldDetectorInfoSet *cfgDetectorInfoSet, QString title, QString subTitle, QWidget *parent = 0);

protected slots:
	void resizeEvent(QResizeEvent *);

protected:
	AMOldDetectorInfoSet *detectorSet_;
	AMOldDetectorInfoSet *cfgDetectorInfoSet_;

	AMDetectorInfoSetView *detectorView_;
	QGridLayout *gl_;
	QLabel *textLabel_;
};

class SGMXASScanConfigurationReviewWizardPage : public QWizardPage
{
	Q_OBJECT
public:
	SGMXASScanConfigurationReviewWizardPage(QString title, QString subTitle, QWidget *parent = 0);

protected slots:
	void resizeEvent(QResizeEvent *);

protected:
	QGridLayout *gl_;
	QLabel *textLabel_;
};

#endif // SGMXASSCANCONFIGURATIONWIZARD_H

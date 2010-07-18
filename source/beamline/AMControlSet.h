#ifndef ACQMAN_AMCONTROLSET_H
#define ACQMAN_AMCONTROLSET_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QVariant>
//#include "acquaman/AMScanConfiguration.h"
//#include "acquaman/AMRegion.h"
#include "acquaman/AMRegionsList.h"
#include "AMAbstractDetector.h"

/// An AMControlSet is designed to hold a logical group of controls.
/*!
  Controls that are heirarchically linked should be children of an AMControl. On the other hand, AMControls that are logically linked should be in an AMControlSet.
  For example, beamline energy might be related to several other controls (say mono angle, undulator gap, and exit slit position).
  These are heirarchically linked, so should be children of an energy AMControl.
  This way you could monitor the status of each child to get the status of the control as a whole.
  Conversely, the expected flux of the beamline at any given energy may be several parameters (say the mono grating choice, the exit slit gap, and the harmonic).
  These are logically linked, and should be in an AMControlSet. The convenience is that a large control object can list its logical groups, which can be displayed together or operated on as a group.
  As well, heirarchically linked controls should not likely wind up as a child of more than one control; however, logically grouped controls have no real problem overlapping.

  All that is required to make an AMControlSet is to give the set a name and to add controls by passing a pointer to the AMControl. A function to remove controls is also offered for convenience.
  */
class AMControlSet : public QObject
{
Q_OBJECT
public:
	/// Constructor, only needs a QObject to act as a parent.
	explicit AMControlSet(QObject *parent = 0);

	~AMControlSet(){
		ctrls_.clear();
	}

	/// Returns the name defined for the control set.
	QString name() const { return name_;}
	int count() { return ctrls_.count();}
	AMControl* controlAt(int index) { return ctrls_.at(index);}
	int indexOf(const QString &name);
	AMControl* controlByName(const QString &name);

signals:

public slots:
	/// Sets the name of the control set.
	void setName(const QString &name) { name_ = name;}
	/// Adds an AMControl to the control set. Returns true if the addition was successful. Failure could result from adding the same AMControl twice.
	bool addControl(AMControl* ctrl);
	/// Removes an AMControl from the control set. Returns true if the removal was successful. Failure could result from removing an AMControl not in the set.
	bool removeControl(AMControl* ctrl);

protected:
	/// Holds the name of the control set. Should be descriptive of the logical relationship.
	/// AMControlSetView will use this value as the title of the group box being displayed.
	QString name_;
	/// Local list of AMControl pointers, which represent the controls in the set.
	QList<AMControl*> ctrls_;
};

/// An AMControlOptimization is an abstract class that represents how a single output can vary across a given region for a given state of a system.
/*!
  This class is designed to be subclassed to achieve functionality.
  The curve function can be called, but it returns an empty map. The intention is for subclasses to define their own list of required state parameters.
  See SGMFluxOptimization and SGMResolutionOptimization for examples.
  A name can be defined to identify the optimization.
  The subclasses can implement the curve() function however they like, whether its through an fitting equation, or a predetermined value mapping.

  */
class AMControlOptimization : public QObject
{
	Q_OBJECT
public:
	/// Constructor, only requires a QObject pointer as a parent.
	AMControlOptimization(QObject *parent=0) : QObject(parent){;}

	/// Returns the name of the optimization, likely hardcoded in the subclass.
	QString name() const { return name_;}
	QString description() const { return description_;}
	/// Returns a QMap to represent the output to optimize. Can be thought of as x-y pairs for a graph.
	/// The context parameters allow only the necessary region to be returned.
	virtual QMap<double, double> curve(QList<QVariant> stateParameters, AMRegionsList* contextParameters);
	virtual QMap< QString, QMap<double, double> > collapse(AMRegionsList* contextParameters);

public slots:
	/// Sets the name of the optimization.
	void setName(const QString &name) { name_ = name;}
	void setDescription(const QString &description) { description_ = description;}

protected:
	/// Holds the name of the optimization.
	QString name_;
	QString description_;
};

/// An AMControlOptimizationSet is a combination of an AMControlSet (its parent class) and a list of AMControlOptimization.
/*!
  The class is designed to hold a list of AMControl (like AMControlSet), as well as the parameter, or parameters, these controls can be used to optimize.
  The true power of this class is to relate multiple optimizable parameters that compete with eachother.
  For example, the beamline flux and the beamline resolution often compete with eachother as increasing one likely decreases the other.
  */
class AMControlOptimizationSet : public AMControlSet
{
	Q_OBJECT
public:
	/// Constructor, only requires a QObject pointer as a parent.
	explicit AMControlOptimizationSet(QObject *parent=0) : AMControlSet(parent){;}

	~AMControlOptimizationSet(){
		outputs_.clear();
	}

	/// Adds an AMControlOptimization to the set.
	void addOptimization(AMControlOptimization *optimization){ outputs_.append(optimization) ;}
	const AMControlOptimization* optimizationAt(int index) const { return outputs_.at(index) ;}
	int optimizationCount() const { return outputs_.count() ;}
	QMap<double, double> curveAt(size_t index, QList<QVariant> stateParameters, AMRegionsList* contextParameters){
		return outputs_.at(index)->curve(stateParameters, contextParameters);
	}
	QMap<QString, QMap<double, double> > collapseAt(size_t index, AMRegionsList* contextParameters){
		return outputs_.at(index)->collapse(contextParameters);
	}
	QMap<QString, QMap<double, double> > plotAgainst(AMRegionsList* contextParameters){
		QMap<QString, QMap<double, double> > fluxes, resolutions;
		QMap<double, double> LEG, MEG, HEG1, HEG3;
		fluxes = collapseAt(0, contextParameters);
		resolutions = collapseAt(1, contextParameters);
		QMap<double, double>::const_iterator i;
		i = fluxes.value("LEG1").constBegin();
		while(i != fluxes.value("LEG1").constEnd()){
			LEG.insert(resolutions.value("LEG1").value(i.key()), fluxes.value("LEG1").value(i.key()));
			MEG.insert(resolutions.value("MEG1").value(i.key()), fluxes.value("MEG1").value(i.key()));
			HEG1.insert(resolutions.value("HEG1").value(i.key()), fluxes.value("HEG1").value(i.key()));
			HEG3.insert(resolutions.value("HEG3").value(i.key()), fluxes.value("HEG3").value(i.key()));
			++i;
		}
		QMap<QString, QMap<double, double> > rVal;
		rVal.insert("LEG1", LEG);
		rVal.insert("MEG1", MEG);
		rVal.insert("HEG1", HEG1);
		rVal.insert("HEG3", HEG3);
		return rVal;
	}
	/**/
	QMap<QString, QPair<double, double> > onePlot(AMRegionsList* contextParameters){
		QMap<QString, QMap<double, double> > allPlots = plotAgainst(contextParameters);
		QMap<double, double>::const_iterator l, m, h, hh;
		l = allPlots.value("LEG1").constBegin();
		m = allPlots.value("MEG1").constBegin();
		h = allPlots.value("HEG1").constBegin();
		hh = allPlots.value("HEG3").constBegin();
		QPair<double, double> fMaxLEG, fMaxMEG, fMaxHEG1, fMaxHEG3, rMaxLEG, rMaxMEG, rMaxHEG1, rMaxHEG3;
		rMaxLEG = QPair<double, double>(l.key(), l.value());
		rMaxMEG = QPair<double, double>(m.key(), m.value());
		rMaxHEG1 = QPair<double, double>(h.key(), h.value());
		rMaxHEG3 = QPair<double, double>(hh.key(), hh.value());
		fMaxLEG = QPair<double, double>(l.value(), l.key());
		fMaxMEG = QPair<double, double>(m.value(), m.key());
		fMaxHEG1 = QPair<double, double>(h.value(), h.key());
		fMaxHEG3 = QPair<double, double>(hh.value(), hh.key());
		++l;
		++m;
		++h;
		++hh;
		while(l != allPlots.value("LEG1").constEnd() ){
			if(rMaxLEG.first < l.key()){
				rMaxLEG.first = l.key();
				rMaxLEG.second = l.value();
			}
			if(fMaxLEG.first < l.value()){
				fMaxLEG.first = l.value();
				fMaxLEG.second = l.key();
			}
			++l;
		}
		while(m != allPlots.value("MEG1").constEnd() ){
			if(rMaxMEG.first < m.key()){
				rMaxMEG.first = m.key();
				rMaxMEG.second = m.value();
			}
			if(fMaxMEG.first < m.value()){
				fMaxMEG.first = m.value();
				fMaxMEG.second = m.key();
			}
			++m;
		}
		while(h != allPlots.value("HEG1").constEnd() ){
			if(rMaxHEG1.first < h.key()){
				rMaxHEG1.first = h.key();
				rMaxHEG1.second = h.value();
			}
			if(fMaxHEG1.first < h.value()){
				fMaxHEG1.first = h.value();
				fMaxHEG1.second = h.key();
			}
			++h;
		}
		while(hh != allPlots.value("HEG3").constEnd() ){
			if(rMaxHEG3.first < hh.key()){
				rMaxHEG3.first = hh.key();
				rMaxHEG3.second = hh.value();
			}
			if(fMaxHEG3.first < hh.value()){
				fMaxHEG3.first = hh.value();
				fMaxHEG3.second = hh.key();
			}
			++hh;
		}
		if(rMaxLEG.first < 1e-100)
			rMaxLEG.first = 0;
		if(rMaxMEG.first < 1e-100)
			rMaxMEG.first = 0;
		if(rMaxHEG1.first < 1e-100)
			rMaxHEG1.first = 0;
		if(rMaxHEG3.first < 1e-100)
			rMaxHEG3.first = 0;
		if(fMaxLEG.first < 1e-100)
			fMaxLEG.first = 0;
		if(fMaxMEG.first < 1e-100)
			fMaxMEG.first = 0;
		if(fMaxHEG1.first < 1e-100)
			fMaxHEG1.first = 0;
		if(fMaxHEG3.first < 1e-100)
			fMaxHEG3.first = 0;
		qDebug() << "Flux Maxes: LEG - " << fMaxLEG.first << "(" << fMaxLEG.second << ") MEG - " << fMaxMEG.first << "(" << fMaxMEG.second
				<< ") HEG1 - " << fMaxHEG1.first << "(" << fMaxHEG1.second << ") HEG3 - " << fMaxHEG3.first << "(" << fMaxHEG3.second << ")\n"
				<< "Resolution Maxes: LEG - " << rMaxLEG.first << "(" << rMaxLEG.second << ") MEG - " << rMaxMEG.first << "(" << rMaxMEG.second
				<< ") HEG1 - " << rMaxHEG1.first << "(" << rMaxHEG1.second << ") HEG3 - " << rMaxHEG3.first << "(" << rMaxHEG3.second << ")";
		QMap<QString, QPair<double, double> > rVal;
		rVal.insert("Something", QPair<double, double>(100, 100));
		return rVal;
	}
	/**/

protected:
	/// Internal list of AMControlOptimization.
	QList<AMControlOptimization*> outputs_;
};


class AMAbstractDetectorSet : public QObject
{
Q_OBJECT
public:
	/// Constructor, only needs a QObject to act as a parent.
	explicit AMAbstractDetectorSet(QObject *parent = 0);

	~AMAbstractDetectorSet(){
		detectors_.clear();
	}

	/// Returns the name defined for the control set.
	QString name() const { return name_;}
	int count() { return detectors_.count();}
	AMAbstractDetector* detectorAt(int index) { return detectors_.at(index);}
	int indexOf(const QString &name);
	AMAbstractDetector* detectorByName(const QString &name);
	bool isDefaultAt(int index) { return defaultDetectors_.at(index);}
	bool isDefaultByName(const QString &name);

signals:

public slots:
	/// Sets the name of the control set.
	void setName(const QString &name) { name_ = name;}
	/// Adds an AMControl to the control set. Returns true if the addition was successful. Failure could result from adding the same AMControl twice.
	bool addDetector(AMAbstractDetector* detector, bool defaultDetector = false);
	/// Removes an AMControl from the control set. Returns true if the removal was successful. Failure could result from removing an AMControl not in the set.
	bool removeDetector(AMAbstractDetector* detector);

protected:
	/// Holds the name of the control set. Should be descriptive of the logical relationship.
	/// AMControlSetView will use this value as the title of the group box being displayed.
	QString name_;
	/// Local list of AMControl pointers, which represent the controls in the set.
	QList<AMAbstractDetector*> detectors_;
	QList<bool> defaultDetectors_;
};

#endif // ACQMAN_AMCONTROLSET_H

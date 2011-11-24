#include "AMAppControllerSupport.h"

#include <QMetaObject>

#include "acquaman/AMScanConfiguration.h"
#include "dataman/export/AMExporter.h"
#include "dataman/export/AMExporterOption.h"

AMScanConfigurationObjectInfo::AMScanConfigurationObjectInfo(AMScanConfiguration *prototypeScanConfiguration, AMExporter *prototypeExporter, AMExporterOption *prototypeExporterOption, int useExporterOptionId){
	initWithMetaObject(prototypeScanConfiguration->metaObject(), prototypeExporter->metaObject(), prototypeExporterOption->metaObject(), useExporterOptionId);
}

AMScanConfigurationObjectInfo::AMScanConfigurationObjectInfo(const QMetaObject *useScanConfigurationMetaObject, const QMetaObject *useExporterMetaObject, const QMetaObject *useExporterOptionMetaObject, int useExporterOptionId){
	if(inheritsScanConfiguration(useScanConfigurationMetaObject) && inheritsExporter(useExporterMetaObject) && inheritsExporterOption(useExporterOptionMetaObject) && (useExporterOptionId >= 1))
		initWithMetaObject(useScanConfigurationMetaObject, useExporterMetaObject, useExporterOptionMetaObject, useExporterOptionId);
	else{
		scanConfigurationMetaObject = 0;
		exporterMetaObject = 0;
		exporterOptionMetaObject = 0;
		exporterOptionId = 0;
	}
}

void AMScanConfigurationObjectInfo::initWithMetaObject(const QMetaObject *useScanConfigurationMetaObject, const QMetaObject *useExporterMetaObject, const QMetaObject *useExporterOptionMetaObject, int useExporterOptionId){
	scanConfigurationMetaObject = useScanConfigurationMetaObject;
	exporterMetaObject = useExporterMetaObject;
	exporterOptionMetaObject = useExporterOptionMetaObject;
	exporterOptionId = useExporterOptionId;
	scanConfigurationClassName = useScanConfigurationMetaObject->className();
	exporterClassName = useExporterMetaObject->className();
	exporterOptionClassName = useExporterOptionMetaObject->className();
}

bool AMScanConfigurationObjectInfo::inheritsScanConfiguration(const QMetaObject *metaObject) const{
	// is this a subclass of AMScanConfiguration? (Or an AMScanConfiguration itself?)
	const QMetaObject* superClass = metaObject;
	bool inheritsScanConfiguration;
	do {
		inheritsScanConfiguration = (superClass->className() == QString("AMScanConfiguration"));
	}
	while( (superClass=superClass->superClass()) && inheritsScanConfiguration == false );
	return inheritsScanConfiguration;
}

bool AMScanConfigurationObjectInfo::inheritsExporter(const QMetaObject *metaObject) const{
	// is this a subclass of AMExporter? (Or an AMExporter itself?)
	const QMetaObject* superClass = metaObject;
	bool inheritsExporter;
	do {
		inheritsExporter = (superClass->className() == QString("AMExporter"));
	}
	while( (superClass=superClass->superClass()) && inheritsExporter == false );
	return inheritsExporter;
}

bool AMScanConfigurationObjectInfo::inheritsExporterOption(const QMetaObject *metaObject) const{
	// is this a subclass of AMExporterOption? (Or an AMExporterOption itself?)
	const QMetaObject* superClass = metaObject;
	bool inheritsExporterOption;
	do {
		inheritsExporterOption = (superClass->className() == QString("AMExporterOption"));
	}
	while( (superClass=superClass->superClass()) && inheritsExporterOption == false );
	return inheritsExporterOption;
}

namespace AMAppControllerSupport{
	QHash<QString, AMScanConfigurationObjectInfo> registeredClasses_;

	const QHash<QString, AMScanConfigurationObjectInfo>* registeredClasses() {
		return &registeredClasses_;
	}

	AMExporter* createExporter(AMScanConfiguration *scanConfiguration){
		if(!scanConfiguration)
			return 0;
		QHash<QString, AMScanConfigurationObjectInfo>::const_iterator i = registeredClasses_.constBegin();
		while( i != registeredClasses_.constEnd()){
			if(i.value().scanConfigurationClassName == scanConfiguration->getMetaObject()->className()){
				AMExporter *exporter = qobject_cast<AMExporter*>(i.value().exporterMetaObject->newInstance());
				if(exporter)
					return exporter;
			}
		}
		return 0;
	}

	AMExporterOption* createExporterOption(AMScanConfiguration *scanConfiguration){
		if(!scanConfiguration)
			return 0;
		QHash<QString, AMScanConfigurationObjectInfo>::const_iterator i = registeredClasses_.constBegin();
		while( i != registeredClasses_.constEnd()){
			if(i.value().scanConfigurationClassName == scanConfiguration->getMetaObject()->className()){
				AMExporterOption *exporterOption = qobject_cast<AMExporterOption*>(i.value().exporterOptionMetaObject->newInstance());
				if(exporterOption && exporterOption->loadFromDb(AMDatabase::database("user"), i.value().exporterOptionId))
					return exporterOption;
			}
		}
		return 0;
	}
}

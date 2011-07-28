include ( acquamanCommon.pri )

TARGET = XRFSpectraViewer

FORMS +=

HEADERS += \
	source/ui/VESPERS/XRFViewer.h \
	source/ui/VESPERS/DeadTimeButton.h

SOURCES += \
	source/application/VESPERS/XRFMain.cpp \
	source/ui/VESPERS/XRFViewer.cpp \
	source/ui/VESPERS/DeadTimeButton.cpp

RESOURCES += \
	source/ui/VESPERS/EndstationPictures.qrc \
	source/ui/VESPERS/vespersIcons.qrc \
	source/ui/StopButton.qrc
#ifndef GratingResolution_H
#define GratingResolution_H

#include <QtGui>
#include "ui_GratingResolution.h"

class GratingResolution : public QWidget, private Ui::GratingResolution {
	
	Q_OBJECT
	
public:
	GratingResolution(QWidget* parent = 0) : QWidget(parent) {
		
		setupUi(this);
	}
	
};

#endif
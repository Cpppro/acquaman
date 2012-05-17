#ifndef AM1DNORMALIZATIONABEDITOR_H
#define AM1DNORMALIZATIONABEDITOR_H

#include <QWidget>
#include <QComboBox>

#include "analysis/AM1DNormalizationAB.h"

/*!
  This is a very basic view that contains a drop box that holds all of the available data sources for normalization.

  \todo Because of the dynamic nature of setting various data sources, the AMDataSource API will need to have a general capability for choosing data sources like this (or in some other way).  That way some of these views can be generalized rather than making a specific one for every analysis block.
  */
class AM1DNormalizationABEditor : public QWidget
{
	Q_OBJECT
public:
	/// Constructor.  Takes in an AM1DNormalizationAB pointer.
	explicit AM1DNormalizationABEditor(AM1DNormalizationAB *analysisBlock, QWidget *parent = 0);

signals:

public slots:

protected slots:
	/// Handles setting the data name for the analysis block based on the new choice of input data source.
	void onDataNameChoiceChanged(int index);
	/// Handles setting the normalization name for the analysis block based on the new choice of input data source.
	void onNormalizationNameChoiceChanged(int index);
	/// Helper slot.  Populates the names combo box.
	void populateComboBox();

protected:
	/// Pointer to the derivative analysis block.
	AM1DNormalizationAB *analysisBlock_;

	/// The combo box that contains all of the names.  Used for the data input source.
	QComboBox *dataNames_;
	/// The combo box that contains all of the names.  Used for the normalization data source.
	QComboBox *normalizationNames_;

};

#endif // AM1DNORMALIZATIONABEDITOR_H
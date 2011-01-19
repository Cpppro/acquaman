#ifndef AMDATASOURCESEDITOR_H
#define AMDATASOURCESEDITOR_H

#include <QWidget>
#include <QMenu>
#include "dataman/AMScanSetModel.h"
#include "ui_AMDataSourcesEditor.h"


/// This widget is used inside AMGenericScanEditor to let users create, delete, and modify the analysis chains / analysis blocks for a set of scans.  It's a quick first prototype; the eventual interface should let users create custom analysis chains and edit the parameters at each level within the chain.  This version only displays the current analysis blocks.
class AMDataSourcesEditor : public QWidget
{
Q_OBJECT
public:
	explicit AMDataSourcesEditor(AMScanSetModel* model, QWidget *parent = 0);


signals:

public slots:
	/// Call this to set the current scan that we edit data sources for
	void setCurrentScan(AMScan* scan);
	/// Call this to set the current scan that we edit data sources for. (The index refers to the scan index in our AMScanSetModel).
	/*! This is an overloaded function.*/
	void setCurrentScan(int scanIndex);

protected slots:
	/// Called when the currently-selected scan or data source (in our list view) is changed
	void onSetViewIndexChanged(const QModelIndex& selected, const QModelIndex& deselected);
	/// Called when a user clicks the 'close' button on a data source.
	void onCloseButtonClicked(const QModelIndex& index);

	/// Called when the 'add data source' button is clicked
	void onAddDataSourceButtonClicked();

	/// Called when the user comes up with a name for their new data source
	void onNewDataSourceNamed();

	/// Called when the user finishes editing a data source description
	void descriptionEditingFinished();


protected:
	/// UI components
	Ui::AMDataSourcesEditor ui_;

	/// This is a model of scans and their data sources. Inside an AMGenericScanEditor, we share this between ourselves, the main editor, and the AMScanView.
	AMScanSetModel* model_;

	/// Helper function: returns the currently selected scan index.  If a data source is selected, this is the index of its parent scan. If a scan is selected, this is the index of that scan.  If nothing is selected, returns -1;
	int currentScanIndex() const;
	/// Helper function: returns the currently selected data source index.  If a scan (or nothing) is currently selected, returns -1;
	int currentDataSourceIndex() const;

	/// Helper function: removes and deletes any current 'detail editors' (data source-specific parameter editors)
	void removeDetailEditor() {
		if(detailEditor_) {
			delete detailEditor_;
			detailEditor_ = 0;
		}
	}

	/// Helper function: install a 'detail editor' (data source-specific parameter editor). If there is an existing detail editor, it will be removed first.  The \c newDetailEditor should be initialized, and already connected to the data source it's intended to edit.  Alternatively, if \c newDetailEditor = 0, the widget is left blank.
	/*! \todo Future performance optimization: If the existing detail editor is for the same type of data source as the new one, devise a way to re-use the existing widget, rather than deleting and re-creating.)*/
	void installDetailEditor(QWidget* newDetailEditor) {
		if(detailEditor_)
			removeDetailEditor();

		detailEditor_ = newDetailEditor;

		if(detailEditor_)
			ui_.detailEditorLayout->addWidget(detailEditor_);
	}


	/// Flag to indicate that we're currently editing the name of a new (not-yet created) data source.
	bool editingNewDataSourceName_;


	/// A data source-specific editor widget, to edit the unique parameters of a data source.  Returned by AMDataSource::createDetailEditor().
	QWidget* detailEditor_;

};

#endif // AMDATASOURCESEDITOR_H
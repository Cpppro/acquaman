#ifndef AMGENERICSCANEDITOR_H
#define AMGENERICSCANEDITOR_H

#include <QWidget>
#include <QMetaType>
#include "ui_AMGenericScanEditor.h"

#include "acquaman.h"
#include "dataman/AMScan.h"
#include "dataman/AMScanSetModel.h"
#include "ui/AMScanView.h"
#include "ui/AMVerticalStackWidget.h"
#include "ui/AMRunSelector.h"
#include "ui/AMSampleEditor.h"
#include "ui/AMChannelEditor.h"

class AMGenericScanEditor : public QWidget
{
Q_OBJECT
public:
	/// Construct an empty editor:
	explicit AMGenericScanEditor(QWidget *parent = 0);

	/// Deletes self and all scan objects that were added
	virtual ~AMGenericScanEditor();

	/// Add a scan to an editor. The editor takes ownership of the scan, and will delete it when closed or the scan is removed.
	void addScan(AMScan* newScan) { /// \todo
		scanSetModel_->addScan(newScan);
		ui_.scanListView->setCurrentIndex(scanSetModel_->indexForScan(newScan));
		if(scanSetModel_->exclusiveChannel().isEmpty() && newScan->numChannels() > 0)
			scanSetModel_->setExclusiveChannel(newScan->channel(0)->name());
	}


	/// Remove a scan from the editor and delete the scan.
	void deleteScan(AMScan* scan) {
		scanSetModel_->removeScan(scan);
		delete scan;
	}

	/// Remove a scan and delete it, but ask the user for confirmation if it's been modified.
	void deleteScanWithModifiedCheck(AMScan* scan);

	/// Remove a scan from the editor, but don't delete the scan. Ownership becomes the responsibility of the caller.
	void removeScan(AMScan* scan) {
		scanSetModel_->removeScan(scan);
	}

	/// Returns the number of scans open in the editor.
	int numScans() const {
		return scanSetModel_->numScans();
	}

	/// Returns a particular scan pointer, by index:
	AMScan* scanAt(int index) const {
		if(index<0 || index>=numScans())
			return 0;
		return scanSetModel_->scanAt(index);
	}





signals:
	/// Internal signal to forward the textChanged() from ui_.notesEdit
	void notesChanged(const QString&);

public slots:

protected slots:
	///  This catches changes in the scan that is currently selected, and hooks it up to the editor widgets. \todo Ultimately, we might handle more than one scan being "selected" at once.
	void onCurrentChanged ( const QModelIndex & selected, const QModelIndex & deselected );

	/// This slot responds to meta-data changes in the current scan
	void onScanMetaDataChanged(const QString& key);

	/// internal signal to forward the textChanged() signal from ui_.notesEdit
	void onNotesTextChanged() {
		emit notesChanged(ui_.notesEdit->toPlainText());
	}

	/// called when the close buttons in the list of scans are clicked
	void onScanModelCloseClicked(const QModelIndex& index);


protected:
	/// This is a model containing the current open scans
	AMScanSetModel* scanSetModel_;
	/// This is the currently-selected scan, or 0 non-existent
	AMScan* currentScan_;

	// UI Components

	/// UI object container
	Ui::AMGenericScanEditor ui_;

	/// stack-widget holder for right-column editors
	AMVerticalStackWidget* stackWidget_;

	/// Run selector
	AMRunSelector* runSelector_;

	/// Channel editor:
	AMChannelEditor* channelEditor_;

	/// Plot view capable of holding multiple scans.
	AMScanView* scanView_;

	/// Sample editor
	AMSampleEditor* sampleEditor_;

	/// Overloaded to enable drag-dropping scans (when Drag Action = Qt::CopyAction and mime-type = "text/uri-list" with the proper format.)
	void dragEnterEvent(QDragEnterEvent *event);
	/// Overloaded to enable drag-dropping scans.
	/*! The Drag is accepted when:
	  - Drag Action = Qt::CopyAction
	  - One of the MIME types is "text/uri-list"... format is "amd://databaseConnectionName/tableName/id"
	  - There is at least one URL in the uri-list
	  - The URL scheme is "amd://"
	  - The database connection name returns a valid database, according to AMDatabase::dbByName(connectionName)
	  - The table is the main Objects table
	  - The id of the item can be found in the table
	  */
	void dropEvent(QDropEvent * event);

	/// This helper function refreshes the editor widgets with the values from a given scan
	void updateEditor(AMScan* scan);

};

Q_DECLARE_METATYPE(AMGenericScanEditor*)

#endif // AMGENERICSCANEDITOR_H
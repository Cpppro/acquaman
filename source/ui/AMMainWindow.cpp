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


#include "AMMainWindow.h"
#include <QDebug>
#include "ui/AMCloseItemDelegate.h"

// Default constructor
AMMainWindow::AMMainWindow(QWidget *parent) : QWidget(parent) {

	model_ = new AMWindowPaneModel(this);

	stackWidget_ = new QStackedWidget();

	sidebar_ = new QTreeView();
	sidebar_->setModel(model_);
	sidebar_->setHeaderHidden(true);
	sidebar_->setRootIsDecorated(true);
	sidebar_->setMouseTracking(true);
	sidebar_->setAttribute(Qt::WA_MacShowFocusRect, false);
	sidebar_->setWordWrap(false);
	sidebar_->setIndentation(10);
	sidebar_->setAnimated(true);
	sidebar_->setEditTriggers(QAbstractItemView::SelectedClicked);
	sidebar_->setSelectionMode(QAbstractItemView::SingleSelection);
	// accept drops onto items (specfically, experiment items, which are supposed to accept dropped scans)
	sidebar_->setDragDropMode(QAbstractItemView::DropOnly);
	sidebar_->setAcceptDrops(true);
	sidebar_->setDropIndicatorShown(true);
	sidebar_->setAutoExpandDelay(300);
	sidebar_->setMinimumWidth(200);
	sidebar_->setMaximumWidth(200);
	sidebar_->setStyleSheet("QTreeView { font: 500 10pt \"Lucida Grande\"; border-width: 1px;   border-style: solid;   border-color: rgb(221, 227, 234);  border-right-color: rgb(64, 64, 64); background-color: rgb(221, 227, 234); show-decoration-selected: 1; selection-background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(91, 146, 213, 255), stop:1 rgba(22, 84, 170, 255)); }"
							" QTreeView::item { height: 30; } "
							" QTreeView::item::selected { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(91, 146, 213, 255), stop:1 rgba(22, 84, 170, 255)); } ");
	AMCloseItemDelegate* del = new AMCloseItemDelegate();
	del->setCloseButtonsEnabled(true);
	sidebar_->setItemDelegate(del);


	model_->invisibleRootItem()->setFlags( Qt::NoItemFlags );


	// setup layouts
	vlayout_ = new QVBoxLayout(this);
	vlayout_->setContentsMargins(0, 0, 0, 0);
	vlayout_->setSpacing(0);
	hlayout_ = new QHBoxLayout();
	hlayout_->setContentsMargins(0, 0, 0, 0);
	hlayout_->setSpacing(0);
	vlayout_->addLayout(hlayout_);

	hlayout_->addWidget(sidebar_);
	hlayout_->addWidget(stackWidget_);

	// connect signals from the model:
	connect(model_, SIGNAL(dockStateChanged(QWidget*,bool)), this, SLOT(onDockStateChanged(QWidget*,bool)));
	connect(model_, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(onModelRowsInserted(QModelIndex,int,int)));
	connect(model_, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(onModelRowsAboutToBeRemoved(QModelIndex,int,int)));

	// connect click and double-click signals from the sidebar:
	connect(sidebar_->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(onSidebarItemActivated(QModelIndex,QModelIndex)));
	connect(sidebar_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onSidebarItemDoubleClicked(QModelIndex)));

	// connect the stackWidget_'s currentWidgetChanged signal, to ensure we stay in sync with the currentIndex in the sidebar:
	connect(stackWidget_, SIGNAL(currentChanged(int)), this, SLOT(onFwdCurrentWidgetChanged(int)));

	// connect 'close' signal from AMCloseItemDelegate
	connect(del, SIGNAL(closeButtonClicked(QModelIndex)), this, SLOT(onItemCloseButtonClicked(QModelIndex)));
}

AMMainWindow::~AMMainWindow() {
	// determine all children widgets:
	QList<QWidget*> panes = model_->allPanes();

	// get rid of the sidebar, so that it's not emitting signals and causing changes while we delete the widgets themselves.
	delete sidebar_;

	disconnect(stackWidget_, SIGNAL(currentChanged(int)), this, SLOT(onFwdCurrentWidgetChanged(int)));
	// delete the model.
	delete model_;

	foreach(QWidget* pane, panes) {
		delete pane;
	}
}



QStandardItem* AMMainWindow::addPane(QWidget* pane, const QString& categoryName, const QString& title, const QString& iconFileName) {

	return model_->addPane(pane, categoryName, title, QIcon(iconFileName));
}




// Removes and deletes a pane widget (whether docked or undocked)
void AMMainWindow::deletePane(QWidget* pane) {

	removePane(pane);

	// delete actual widget:
	delete pane;
}



// Removes a pane widget but does not delete it.  Ownership is now the responsibility of the caller. The pane becomes a top-level window.
void AMMainWindow::removePane(QWidget* pane) {

	QModelIndex i = model_->indexForPane(pane);

	if(i.isValid())
		model_->removeRow(i.row(), i.parent());
}


// Insert a new heading item at a given index.  This can be used in situations where you want a pane added using addPane() to appear at a given \c position (from top to bottom).  Call insertHeading() first, with the top-level \c position and \c title of the heading, and then call addPane() with the same heading title.
/* If \c position is less than 0 or > than the number of existing headings, the heading will be inserted at the bottom.  Returns the newly-created heading item.  */
QStandardItem* AMMainWindow::insertHeading(const QString& title, int position) {
	return model_->headingItem(title, QModelIndex(), position);
}


void AMMainWindow::onModelRowsInserted(const QModelIndex &parent, int start, int end) {

	for(int row=start; row<=end; row++) {
		QModelIndex i = model_->index(row, 0, parent);

		// qDebug() << "Inserting new item. isAlias() = " << model_->isAlias(i) << "isHeading = " << model_->isHeading(i);

		// for "real" item entries. (Nothing to do for aliases or headings)
		if(!model_->isAlias(i) && !model_->isHeading(i)) {

			QWidget* pane = model_->pane(i);
			// qDebug() << "  Still inserting new item. pane is:" << pane;

			if(model_->isDocked(i) && pane) {
				stackWidget_->addWidget(pane);
			}
			else if(pane) {
				pane->setParent(0);
				pane->show();
			}
		}
	}

	sidebar_->expand(parent);
}

void AMMainWindow::onModelRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {


	for(int row=start; row<=end; row++) {
		QModelIndex i = model_->index(row, 0, parent);

		// for "real" items only. Don't care about removing headings or aliases.
		if(!model_->isAlias(i) && !model_->isHeading(i)) {

			if(model_->isDocked(i)) {
				QWidget* pane = model_->pane(i);
				if(pane) {

					QSize oldSize = pane->size();
					QPoint oldPos = pane->mapToGlobal(pane->geometry().topLeft());
					stackWidget_->removeWidget(pane);
					pane->setParent(0);
					pane->setGeometry(QRect(oldPos + QPoint(20, 20), oldSize));
					pane->show();

					if(previousPane_ == pane)
						previousPane_ = 0;
				}
			}
		}
	}
}

void AMMainWindow::onItemCloseButtonClicked(const QModelIndex &index) {
	emit itemCloseButtonClicked(index);
}

void AMMainWindow::onDockStateChanged(QWidget* pane, bool isDocked) {
	// dock it
	if(isDocked) {
		stackWidget_->addWidget(pane);
	}

	// undock it
	else {
		QSize oldSize = pane->size();
		QPoint oldPos = pane->mapToGlobal(pane->geometry().topLeft());

		// the most intuitive thing for users would be: when undocking this pane, take them back in the main-window to the previously-visited pane.
		// caveats: need to make sure that the previously-visited pane still exists, and is actually docked itself.
		QModelIndex previousPaneIndex = model_->indexForPane(previousPane_);
		if(previousPaneIndex.isValid() && model_->isDocked(previousPaneIndex) && previousPane_ != pane)
			stackWidget_->setCurrentWidget(previousPane_);

		stackWidget_->removeWidget(pane);
		pane->setParent(0);
		pane->setGeometry(QRect(oldPos + QPoint(20, 20), oldSize));
		pane->show();
	}
}



void AMMainWindow::goToPane(QWidget* pane){

	// widget doesn't exist
	QModelIndex i = model_->indexForPane(pane);
	if(!i.isValid())
		return;

	// if its a docked widget, set as current widget
	if(model_->isDocked(i)) {
		previousPane_ = stackWidget_->currentWidget();
		stackWidget_->setCurrentWidget(pane);
	}

	// if it's undocked, bring it to the front
	else {
		pane->activateWindow();
		// unnecessary, since pane is a top-level window with no siblings: pane->raise();
	}
}


void AMMainWindow::onSidebarItemActivated(const QModelIndex& index, const QModelIndex& oldIndex) {

	Q_UNUSED(oldIndex)

	QWidget* pane = model_->pane(index);

	// qDebug() << "Calling onSidebarItemActivated. pane = " << pane << "index = " << index;

	if(pane == 0)
		return;

	// if this item is an "alias" item for another widget, pane() will correctly access the real widget. However, we should emit the 'aliasActivated' signal to notify
	if(model_->isAlias(index)) {
		emit aliasItemActivated(pane, model_->aliasKey(index), model_->aliasValue(index));
	}

	// already done
	if(pane == stackWidget_->currentWidget())
		return;

	// If it's floating free, need to grab it back
	if(!model_->isDocked(index)) {
		model_->dock(index);
	}

	previousPane_ = stackWidget_->currentWidget();
	stackWidget_->setCurrentWidget(pane);
}

void AMMainWindow::onSidebarItemDoubleClicked(const QModelIndex& index) {
	model_->undock(index);
}

// We intercept and forward the currentChanged(int) signal from the QStackedWidget, to keep the sidebar's highlighted link consistent with the current widget.
void AMMainWindow::onFwdCurrentWidgetChanged(int currentIndex) {

	if(currentIndex < 0)
		return;

	QWidget* currentPane = stackWidget_->widget(currentIndex);
	// Normally, we want to ensure that the highlighted selection in the tree(sidebar) stays consistent with the selected pane.
	// However, if there are multiple links for a single pane, we don't know which one to choose, so its better to not do anything at all. (For now... this could be improved with a more detailed internal model.)
	// ??????????
	QModelIndex i = model_->indexForPane(currentPane);
	// sidebar_->setCurrentIndex(i);

	emit currentPaneChanged(currentPane);
}


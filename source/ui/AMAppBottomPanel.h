#ifndef AMAPPBOTTOMPANEL_H
#define AMAPPBOTTOMPANEL_H

#include "ui/AMDatamanAppBottomPanel.h"
#include "ui/actions3/AMActionRunnerBottomBarCurrentView3.h"
#include "actions3/AMActionRunner3.h"

/// This class implements the bottom panel used in the standard App for AM.  It contains a mini-view of the workflow and adds it between the add experiment button and the status view.
class AMAppBottomPanel : public AMDatamanAppBottomPanel
{
	Q_OBJECT

public:
	/// Constructor.  Passes in the action runner to appropriately build the mini-workflow view.
	AMAppBottomPanel(AMActionRunner3 *actionRunner, QWidget *parent = 0);

protected:
	/// The current workflow view.
	AMActionRunnerBottomBarCurrentView3 *workflowView_;
};

#endif // AMAPPBOTTOMPANEL_H

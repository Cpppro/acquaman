#include "AMNestedAction.h"

bool AMNestedAction::insertSubAction(AMAction *action, int index)
{
	if(!action)
		return false;

	if(state() != Constructed)
		return false;

	if(index < 0 || index > subActionCount())
		index = subActionCount();

	emit subActionAboutToBeAdded(index);
	action->internalSetParentAction(this);
	insertSubActionImplementation(action, index);
	emit subActionAdded(index);

	return true;
}

bool AMNestedAction::deleteSubAction(int index)
{

	AMAction* action = takeSubAction(index);
	delete action;	// harmless if 0

	return (action != 0);
}

bool AMNestedAction::duplicateSubActions(const QList<int> &indexesToCopy)
{
	if(state() != Constructed)
		return false;

	if(indexesToCopy.isEmpty())
		return true;	// done

	// sort the list, so we know the highest index in it.
	QList<int> sIndexesToCopy = indexesToCopy;
	qSort(sIndexesToCopy);

	// any indexes out of range?
	if(sIndexesToCopy.first() < 0)
		return false;
	if(sIndexesToCopy.last() >= subActionCount())
		return false;

	// insert at the position after last existing subAction to copy
	int insertionIndex = sIndexesToCopy.last() + 1;

	// insert copies of all, using regular insertSubAction().
	foreach(int i, sIndexesToCopy)
		insertSubAction(subActionAt(i)->createCopy(), insertionIndex++);

	return true;
}

int AMNestedAction::indexOfSubAction(const AMAction *action) const
{
	for(int i=0,cc=subActionCount(); i<cc; i++) {
		if(action == subActionAt(i))
			return i;
	}
	return -1;
}

AMAction * AMNestedAction::takeSubAction(int index)
{
	if(state() != Constructed)
		return 0;

	if(index < 0 || index >= subActionCount())
		return 0;

	emit subActionAboutToBeRemoved(index);
	AMAction* action = takeSubActionImplementation(index);
	action->internalSetParentAction(0);
	emit subActionRemoved(index);

	return action;
}

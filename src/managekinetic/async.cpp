#include <managekinetic/async.h>

// -----------------------------------------------------------------------------
// Routine Handler
// -----------------------------------------------------------------------------

operation* routine_handler::
getOperationFromIdentifier(ui32 identifier)
{
	for (operation* currentOperation : this->_operations)
		if (currentOperation->operationIdentifier == identifier)
			return currentOperation;
	return nullptr;
}

ui32 routine_handler::
insertOperation(operation* operationPtr)
{
	this->_operations.push_back(operationPtr);
	ui32 operationIndex = ++this->_operationIndexCounter;
	operationPtr->setIdentifier(operationIndex);
	return operationIndex;
}

void routine_handler::
removeOperation(ui32 identifier)
{
	for (ui32 operationIndex = 0; operationIndex < this->_operations.size(); ++operationIndex)
	{
		if (this->_operations[operationIndex] != nullptr &&
			this->_operations[operationIndex]->operationIdentifier == identifier)
		{
			delete this->_operations[operationIndex];
			this->_operations[operationIndex] = nullptr;
			this->_operations.erase(this->_operations.begin() + operationIndex);
		}
	}
}

void routine_handler::
executeOperations()
{
	for (operation* currentOperation : this->_operations)
		if (!currentOperation->operationStarted)
		{
			currentOperation->operationStarted = true;
			currentOperation->run();
		}
}

routine_handler::
~routine_handler()
{
	for (operation* currentOperation : this->_operations)
		delete currentOperation;
}

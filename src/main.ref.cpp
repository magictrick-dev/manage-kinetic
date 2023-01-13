#if 0
class worker_thread
{
	public:

		worker_thread(ui64 id, shared_thread_context* t_context);
		~worker_thread();

		void main();
		bool getDocumentFromQueue(ui64* queueIndex, RecordsTable** currentTableVector);

	protected:
		ui64 id;
		shared_thread_context* tContext;
};

worker_thread::
worker_thread(ui64 id, shared_thread_context* t_context) :
	id(id), tContext(t_context)
{
	printf("[ Thread ID #%lld ] : Thread successfully launched.\n", this->id);
}

worker_thread::
~worker_thread()
{
	printf("[ Thread ID #%lld ] : Thread is exitting.\n", this->id);
}

bool worker_thread::
getDocumentFromQueue(ui64* queueIndex, RecordsTable** currentTableVector)
{

	// Lock the queue.
	this->tContext->m_workqueue_lock.lock();

	// Ensure we are within the workqueue bounds.
	ui64 currentDocQueueIndex = this->tContext->queueIndex;
	if (currentDocQueueIndex >= this->tContext->documentQueue->size())
	{
		// If the queue index is out of bounds, there is no more work to do.
		this->tContext->m_workqueue_lock.unlock();
		return false;
	}

	// There is work, set the necessary output parameters and bump the queue index.
	*queueIndex = currentDocQueueIndex;
	*currentTableVector = this->tContext->documentQueue->operator[](currentDocQueueIndex);
	this->tContext->queueIndex += 1;

	// Unlock the work queue.
	this->tContext->m_workqueue_lock.unlock();

	return true;

}

/**
 * The main execution point for a worker thread. Once this function exits, the
 * thread should shut down.
 */
void worker_thread::
main()
{

	// If document from queue returns true, it is guaranteed we have a valid document
	// queue index value and can process further.
	ui64 documentQueueIndex;
	RecordsTable* currentTableVector;
	while (this->getDocumentFromQueue(&documentQueueIndex, &currentTableVector))
	{
		printf("[ Thread ID #%lld ] : Processing work queue index %lld.\n", this->id, documentQueueIndex);

		// Create a mapping table to store our values in.
		MappingTable mtable = {};

		// Go through each record in the table and parse it out.
		for (ui64 rIndex = 0; rIndex < currentTableVector->size(); ++rIndex)
		{

			// Get the raw record string, ensure it's not empty.
			std::string* currentRecord = &currentTableVector->operator[](rIndex);
			if (currentRecord->empty()) continue;

			// Create a record to store the fields in.
			MappingRecord& mrecord = mtable.createRecord();
			MappingField* mfield = &mrecord.createField();
			
			// Parse out the string.
			std::string parseString = {};
			for (ui64 characterIndex = 0; characterIndex < currentRecord->length(); ++characterIndex)
			{
				
				// Get the character.
				char currentCharacter = currentRecord->operator[](characterIndex);
				
				// If we reached a field delimiter marker, we can store it in the mapping record.
				if (currentCharacter == ROW_FIELD_DELIMITER)
				{
					mfield->insertValue(parseString);
					mfield = &mrecord.createField();
					parseString.clear();
				}

				// If we reached a multi-value marker, insert it and clear the parsing string.
				else if ((ui8)currentCharacter == MULTIVALUE_SEPERATOR_UNSIGNED)
				{
					mfield->insertValue(parseString);
					parseString.clear();
				}

				// Otherwise, push the character.
				else
				{
					if (currentCharacter >= 32 && currentCharacter <= 126)
						parseString += currentCharacter;
					else
					{
						std::string alpha = {};
						alpha += currentCharacter;
						parseString += CP1252_to_UTF8(alpha);
					}
				}

			}

		}

		// Once the mtable is built, we can dump it straight to excel.
		// Create XLSX document.
		OpenXLSX::XLDocument exportDocument;
		std::stringstream exportDocumentName;
		exportDocumentName << "cmaster_" << documentQueueIndex << ".xlsx";
		exportDocument.create(exportDocumentName.str());

		// Get the first sheet for modification.
		OpenXLSX::XLWorksheet worksheet = exportDocument.workbook().worksheet("Sheet1");

		// Go through each record.
		for (ui64 rIndex = 0; rIndex < mtable.count(); ++rIndex)
		{

			// Get the current record we are working with from the table.
			MappingRecord& currentRecord = mtable[rIndex];

			// Now begin to dump the data into the worksheet.
			for (ui64 fIndex = 0; fIndex < currentRecord.count(); ++fIndex)
			{

				// Collect the row and column of where the data goes, OpenXLSX
				// uses non-zero based indexing.
				ui64 row = rIndex + 1;
				ui64 col = fIndex + 1;

				// Get the field we are working with.
				MappingField& currentField = currentRecord[fIndex];

				// Since some fields are multivalue, we need to account for that.
				std::stringstream outputss = {};
				for (ui64 vIndex = 0; vIndex < currentField.count(); ++vIndex)
				{
					outputss << currentField[vIndex];
					if (vIndex < currentField.count()-1)
						outputss << MULTIVALUE_FIELD_DELIMITER;
				}

				// Insert into the document.
				worksheet.cell(row, col).value() = outputss.str();

				outputss.clear();

			}

		}

		exportDocument.save();
		exportDocument.close();

	}

}

// ---------------------------------------------------------------------------------------------------------------------
// Worker Thread Procedures
// ---------------------------------------------------------------------------------------------------------------------

void
tProcedureMain(ui64 id, shared_thread_context* t_context)
{
	// Create a worker_thread instance and then execute worker_thread.main().
	worker_thread wThread = { id, t_context };
	wThread.main();
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Functions
// ---------------------------------------------------------------------------------------------------------------------

RecordsTableQueue
createDocumentQueueFromSource(char* rawSourceFilePtr, runtime_constants* rtConstants)
{

	RecordsTableQueue docQueue = {};

	std::vector<std::string>* currentQueue = new std::vector<std::string>;
	docQueue.push_back(currentQueue);

	ui64 characterIndex = 0;
	ui64 currentRowIndex = 0;
	std::string currentRowString = {};
	while (rawSourceFilePtr[characterIndex] != '\0')
	{
		// Pull the character out and then check for a new line.
		char currentCharacter = rawSourceFilePtr[characterIndex++];
		if (currentCharacter == '\n')
		{
			currentQueue->push_back(currentRowString);
			currentRowString.clear();
			currentRowIndex++;

			if (currentRowIndex >= rtConstants->recordsPerDocument)
			{
				currentQueue = new std::vector<std::string>;
				docQueue.push_back(currentQueue);
				currentRowIndex = 0;
			}

			continue;
		}

		// Otherwise, push the character onto the currentRowString.
		currentRowString += currentCharacter;
	}

	// Push back the dangling row and return.
	if (!currentRowString.empty())
	{
		currentQueue->push_back(currentRowString);
	}

	currentRowString.clear();

	return docQueue;

}
#endif

#include <string>
#include <sstream>
#include <codecvt>
#include <locale>

#include <managekinetic/application.h>

#include <platform/fileio_win32.h>
#include <platform/memory_win32.h>

#include <vendor/OpenXLSX/OpenXLSX.hpp>

inline static std::string
CP1252_to_UTF8(const std::string &byte_array)
{
	static const wchar_t CP1252_UNICODE_TABLE[] =
		L"\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007"
		L"\u0008\u0009\u000A\u000B\u000C\u000D\u000E\u000F"
		L"\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017"
		L"\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F"
		L"\u0020\u0021\u0022\u0023\u0024\u0025\u0026\u0027"
		L"\u0028\u0029\u002A\u002B\u002C\u002D\u002E\u002F"
		L"\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037"
		L"\u0038\u0039\u003A\u003B\u003C\u003D\u003E\u003F"
		L"\u0040\u0041\u0042\u0043\u0044\u0045\u0046\u0047"
		L"\u0048\u0049\u004A\u004B\u004C\u004D\u004E\u004F"
		L"\u0050\u0051\u0052\u0053\u0054\u0055\u0056\u0057"
		L"\u0058\u0059\u005A\u005B\u005C\u005D\u005E\u005F"
		L"\u0060\u0061\u0062\u0063\u0064\u0065\u0066\u0067"
		L"\u0068\u0069\u006A\u006B\u006C\u006D\u006E\u006F"
		L"\u0070\u0071\u0072\u0073\u0074\u0075\u0076\u0077"
		L"\u0078\u0079\u007A\u007B\u007C\u007D\u007E\u007F"
		L"\u20AC\u0020\u201A\u0192\u201E\u2026\u2020\u2021"
		L"\u02C6\u2030\u0160\u2039\u0152\u0020\u017D\u0020"
		L"\u0020\u2018\u2019\u201C\u201D\u2022\u2013\u2014"
		L"\u02DC\u2122\u0161\u203A\u0153\u0020\u017E\u0178"
		L"\u00A0\u00A1\u00A2\u00A3\u00A4\u00A5\u00A6\u00A7"
		L"\u00A8\u00A9\u00AA\u00AB\u00AC\u00AD\u00AE\u00AF"
		L"\u00B0\u00B1\u00B2\u00B3\u00B4\u00B5\u00B6\u00B7"
		L"\u00B8\u00B9\u00BA\u00BB\u00BC\u00BD\u00BE\u00BF"
		L"\u00C0\u00C1\u00C2\u00C3\u00C4\u00C5\u00C6\u00C7"
		L"\u00C8\u00C9\u00CA\u00CB\u00CC\u00CD\u00CE\u00CF"
		L"\u00D0\u00D1\u00D2\u00D3\u00D4\u00D5\u00D6\u00D7"
		L"\u00D8\u00D9\u00DA\u00DB\u00DC\u00DD\u00DE\u00DF"
		L"\u00E0\u00E1\u00E2\u00E3\u00E4\u00E5\u00E6\u00E7"
		L"\u00E8\u00E9\u00EA\u00EB\u00EC\u00ED\u00EE\u00EF"
		L"\u00F0\u00F1\u00F2\u00F3\u00F4\u00F5\u00F6\u00F7"
		L"\u00F8\u00F9\u00FA\u00FB\u00FC\u00FD\u00FE\u00FF";

	std::wstring unicode(byte_array.size(), ' ');
	for (size_t i = 0; i < unicode.size(); ++i)
		unicode[i] = CP1252_UNICODE_TABLE[(uint8_t)byte_array[i]];

	std::wstring_convert<std::codecvt_utf8<wchar_t>> unicode_to_utf8;
	return unicode_to_utf8.to_bytes(unicode);

}

// ---------------------------------------------------------------------------------------------------------------------
// Document Definitions
// ---------------------------------------------------------------------------------------------------------------------

document::
document(std::filesystem::path filepath, ui32 maxRecordsPerDocument)
	: table(maxRecordsPerDocument)
{
	this->filepath = filepath;
	this->documentLoaderThread = std::thread(&document::parse, this);
	this->documentLoaderThread.detach();
}

std::vector<parseprogress> document::
getParsingProgress()
{
	std::vector<parseprogress> prog;
	this->parsingProgressLock.lock();
	prog = this->parsingProgress;
	this->parsingProgressLock.unlock();
	return prog;
}

void document::
beginExport(std::string filepath)
{
	this->exportpath = filepath;
	this->documentExportThread = std::thread(&document::exportSubroutine, this);
	this->exportStartFlag = true;
	this->documentExportThread.detach();
}

void document::
exportSubroutine()
{

	printf("[ Export Subroutine Thread ] : Exporting to %s\n", this->exportpath.string().c_str());

	// Setup the export process.
	tablemap& table = application::get().currentDocument->getTable();
	this->exportProgressLock.lock();
	for (ui64 p = 0; p < table.subtableCount(); ++p)
		this->exportProgress.push_back({});
	this->exportProgressLock.unlock();

	std::vector<std::thread> worker_threads;
	for (ui64 t = 0; t < table.subtableCount(); ++t)
		worker_threads.push_back(std::thread(&document::exportSubroutineCreateExcelDocument,
			this, t));

	for (ui64 t = 0; t < worker_threads.size(); ++t)
		if (worker_threads[t].joinable())
			worker_threads[t].join();

	printf("[ Export Subroutine Thread ] : Export complete.\n");
	this->exportCompleteFlag = true;

}

void document::
exportSubroutineCreateExcelDocument(ui64 workIndex)
{

	printf("[ Export Subroutine Worker #%lld ] : Working subtable index %lld\n",
		workIndex, workIndex);

	// -------------------------------------------------------------------------
	// Get the path.
	// -------------------------------------------------------------------------

	std::filesystem::path root_path = this->exportpath;
	std::stringstream fpath = {};
	std::filesystem::path base_name = root_path.filename().replace_extension("");
	std::stringstream fname = {};
	fname << base_name.string() << "_" << workIndex + 1 << root_path.extension().string();
	root_path.replace_filename(fname.str());
	printf("[ Export Subroutine Worker %lld ] : Exporting to %s\n", workIndex, root_path.string().c_str());

	// -------------------------------------------------------------------------
	// Once we have the path, we can create the excel document.
	// -------------------------------------------------------------------------

	OpenXLSX::XLDocument exceldoc;
	exceldoc.create(root_path.string());
	if (!exceldoc.isOpen())
	{
		printf("[ Export Subroutine Worker %lld ] : Unable to expor to %s\n", workIndex,
			root_path.string().c_str());
		return;
	}
	OpenXLSX::XLWorksheet excelsheet = exceldoc.workbook().worksheet("Sheet1");

	// -------------------------------------------------------------------------
	// Dump the contents of the subtable to the sheet.
	// -------------------------------------------------------------------------


	// Get the subtable. Lock to ensure non-compete with other threads.
	application::get().currentDocument->lockTable();
	tablemap& table = application::get().currentDocument->getTable();
	std::shared_ptr<subtable> currentSubtable = application::get().currentDocument->getTable()
		.getSubtables()[workIndex];
	std::vector<record>& subrecords = currentSubtable->getRecords();
	std::vector<column_map> maps = table.getMappedColumns();
	application::get().currentDocument->unlockTable();

	// Label each of the column headers.
	for (ui64 colh = 0; colh < maps.size(); ++colh)
	{
		excelsheet.cell(1, colh + 1).value() = maps[colh].export_alias;
	}

	// Dump each row. Offset the row by 1 when inserting values to accomodate the
	// headers.
	for (ui64 row = 0; row < currentSubtable->size(); ++row)
	{
		record& currentRecord = subrecords[row];
		for (ui64 col = 0; col < maps.size(); ++col)
		{
			field* currentField = currentRecord[maps[col].column_index];
			if (currentField != nullptr)
			{
				if (maps[col].all_multivalues == true)
				{
					excelsheet.cell(row + 2, col + 1).value() = 
						currentField->get(maps[col].multivalue_delim);
				}
				else
				{
					excelsheet.cell(row + 2, col + 1).value() =
						currentField->get(maps[col].multivalue_index - 1);
				}
			}
		}
	}

	// Once dumped, save, and exit.
	exceldoc.save();

	printf("[ Export Subroutine Worker #%lld ] : Worker export complete.\n", workIndex);

}

void document::
parse()
{

	// -------------------------------------------------------------------------
	// Start the initialization procedure.
	// -------------------------------------------------------------------------

	this->initializingFlag = true;

	// -------------------------------------------------------------------------
	// Retrieve the application parameters.
	// -------------------------------------------------------------------------

	ui32 recordsPerDocument;
	ui32 workerThreads;

	application::get().parmetersLock.lock();

	recordsPerDocument = application::get().recordsPerTable;
	workerThreads = application::get().parseingThreads;

	application::get().parmetersLock.unlock();

	// -------------------------------------------------------------------------
	// Load the file.
	// -------------------------------------------------------------------------

	ui64 	filesize = sourceFileSize(this->filepath.string());
	void* 	filebuffer = virtualHeapAllocate(filesize + 1);
	ui64 	bytesread = sourceFileOpen(filebuffer, filesize+1,	this->filepath.string(), filesize);
	char* 	rawSourceFilePtr = (char*)filebuffer;

	// -------------------------------------------------------------------------
	// Create the work queue.
	// -------------------------------------------------------------------------

	this->workQueue = new TableWorkQueue;
	UnprocessedTable* currentQueue = nullptr;
	this->workQueue->emplace_back();
	currentQueue = &this->workQueue->back();

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

			if (currentRowIndex >= recordsPerDocument)
			{
				this->workQueue->emplace_back();
				currentQueue = &this->workQueue->back();
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

	// -------------------------------------------------------------------------
	// Unload the file from memory.
	// -------------------------------------------------------------------------

	virtualHeapFree(filebuffer);

	// -------------------------------------------------------------------------
	// Create parsing progress markers for the front-end.
	// -------------------------------------------------------------------------

	this->parsingProgressLock.lock();

	for (ui32 i = 0; i < this->workQueue->size(); ++i)
	{
		this->parsingProgress.push_back({0, (ui32)this->workQueue->operator[](i).size() });
	}

	this->parsingProgressLock.unlock();

	// -------------------------------------------------------------------------
	// Start the parse procedure.
	// -------------------------------------------------------------------------

	this->initializingFlag = false;
	this->parsingFlag = true;

	// -------------------------------------------------------------------------
	// Start the parse subroutine workers.
	// -------------------------------------------------------------------------

	std::vector<std::thread> w_threads = {};
	for (ui32 workerIndex = 0; workerIndex < workerThreads; ++workerIndex)
	{
		w_threads.emplace_back(std::thread(&document::parseSubroutine, this, workerIndex));
	}

	// Join the threads and wait for completion.
	for (ui32 workerIndex = 0; workerIndex < workerThreads; ++workerIndex)
	{
		if (w_threads[workerIndex].joinable())
			w_threads[workerIndex].join();
	}

	// -------------------------------------------------------------------------
	// Clean up, ready, and exit.
	// -------------------------------------------------------------------------

	delete this->workQueue;

	this->readyFlag = true;
	this->parsingFlag = false;


}

void document::
parseSubroutine(ui32 workerIdentifier)
{

	printf("[ Subroutine Worker #%d ] : Worker has initialized.\n", workerIdentifier);

	// -------------------------------------------------------------------------
	// Create a runtime loop.
	// -------------------------------------------------------------------------

	bool runtimeFlag = true;
	while (runtimeFlag == true)
	{

		ui32 w_index;
		UnprocessedTable* currentTable = nullptr;
		parseprogress* prog = nullptr;

		// ---------------------------------------------------------------------
		// Retrieve work from the work queue.
		// ---------------------------------------------------------------------

		this->workQueueLock.lock();
		
		// Ensure there is work to do.
		w_index = this->workQueueIndex;
		if (workQueueIndex >= this->workQueue->size())
		{
			this->workQueueLock.unlock();
			runtimeFlag = false;
			continue;
		}

		// If there is, grab it.
		this->workQueueIndex++;
		currentTable = &this->workQueue->operator[](w_index);

		this->workQueueLock.unlock();

		// ---------------------------------------------------------------------
		// Get the progress for front-end.
		// ---------------------------------------------------------------------

		this->parsingProgressLock.lock();
		prog = &this->parsingProgress[w_index];
		this->parsingProgressLock.unlock();

		// ---------------------------------------------------------------------
		// Prepare the new mapping table.
		// ---------------------------------------------------------------------

		printf("[ Subroutine Worker #%d ] : Processing work queue index %d.\n",
			workerIdentifier, w_index);

		parseSubroutineCreateMappingTable(workerIdentifier, prog, currentTable);

	}

	printf("[ Subroutine Worker #%d ] : Worker is shutting down.\n", workerIdentifier);

}

void document::
parseSubroutineCreateMappingTable(ui32 workerIdentifier, parseprogress* prog, UnprocessedTable* uTable)
{

	// -------------------------------------------------------------------------
	// Create a subtable.
	// -------------------------------------------------------------------------

	std::shared_ptr<subtable> currentSubtable = nullptr;
	this->tableLock.lock();
	currentSubtable = this->table.createSubtable();
	this->tableLock.unlock();

	// -------------------------------------------------------------------------
	// Loop through all the values.
	// -------------------------------------------------------------------------
	for (ui64 stringIndex = 0; stringIndex < uTable->size(); ++stringIndex)
	{

		record* currentRecord = &currentSubtable->create();

		std::vector<std::string> currentFields = {};
		std::string fieldstring = {};

		std::string& uString = uTable->operator[](stringIndex);

		for (ui64 characterIndex = 0; characterIndex < uString.length(); ++characterIndex)
		{

			// Get the character.
			char currentCharacter = uString[characterIndex];
			
			// If we reached a field delimiter marker, we can store it in the mapping record.
			if (currentCharacter == ROW_FIELD_DELIMITER)
			{

				currentFields.push_back(fieldstring);
				fieldstring.clear();

				currentRecord->insertMultivalueField(currentFields);
				currentFields.clear();

			}

			// If we reached a multi-value marker, insert it and clear the parsing string.
			else if ((ui8)currentCharacter == MULTIVALUE_SEPERATOR_UNSIGNED)
			{
				currentFields.push_back(fieldstring);
				fieldstring.clear();
			}

			// Otherwise, push the character.
			else
			{
				if (currentCharacter >= 32 && currentCharacter <= 126)
					fieldstring += currentCharacter;
				else
				{
					std::string alpha = {};
					alpha += currentCharacter;
					fieldstring += CP1252_to_UTF8(alpha);
				}
			}

		}

		// ---------------------------------------------------------------------
		// Insert the hanging data as well.
		// ---------------------------------------------------------------------

		currentFields.push_back(fieldstring);
		fieldstring.clear();

		currentRecord->insertMultivalueField(currentFields);
		currentFields.clear();

		// ---------------------------------------------------------------------
		// Once we have processed that current record, update the progress.
		// ---------------------------------------------------------------------

		prog->recordsProcessed = stringIndex + 1;

	}

}

document::
~document()
{ }

std::string document::
getFilename() const
{
	return this->filepath.filename().string();
}


/**
 * -----------------------------------------------------------------------------
 * Managekinetic
 * 		A unidata table processing tool which formats multi-value fields and
 * 		exports the data to Excel. Uses multi-threading to enhance performance
 * 		and allows for concurrent processing of records.
 * 
 * By Chris DeJong, 12/28/2022
 * -----------------------------------------------------------------------------
 */
#include <windows.h>
#include <GL/GL.h>
#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <codecvt>
#include <locale>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>
#include <cassert>

#include <main.h>
#include <managekinetic/commandline.h>
#include <managekinetic/async.h>
#include <managekinetic/uicomponent.h>
#include <managekinetic/tableviewer.h>

#include <platform/benchmark_win32.h>
#include <platform/memory_win32.h>
#include <platform/fileio_win32.h>

#include <vendor/OpenXLSX/OpenXLSX.hpp>

#include <vendor/DearImGUI/imgui.h>
#include <vendor/DearImGUI/misc/cpp/imgui_stdlib.h>
#include <vendor/DearImGUI/backends/imgui_impl_win32.h>
#include <vendor/DearImGUI/backends/imgui_impl_opengl3.h>

inline std::string
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

void
performCloseFile()
{
	bool closeDocument = true;

	if (application::get().currentDocument != nullptr)
	{

#if 0
		// If the document is saveable and the document has pending changes
		if (application::get().currentDocument->isDocumentSaveable() &&
			!application::get().currentDocument->isDocumentUnchanged())
		{
			INT userCloseChoice = MessageBoxA(application::get().internal.windowHandle,
				"Are you sure you want to close this document?"
				" You may lose your current progress."
				, "Close Document Confirmation", MB_YESNO);
			if (userCloseChoice == IDNO) closeDocument = false;
		}
#endif

		if (closeDocument == true)
		{
			application::get().mappingWindows.clear();
			delete application::get().currentDocument;
			application::get().currentDocument = nullptr;
		}

	}
}

void
performSaveFile()
{

	std::string savepath = sourceSaveFileDialogue(&application::get().internal.windowHandle,
		".xlsx", "Excel XLSX Document .xlsx\0*.xlsx\0\0");
	if (!savepath.empty())
	{
		printf("[ Main Thread ] : Saving %s.\n", savepath.c_str());

		if (application::get().currentDocument != nullptr && application::get().currentDocument->isReady())
		{
			application::get().currentDocument->beginExport(savepath);
			application::get().exportWindow->open();
		}

	}

}

void
performOpenFile()
{
	std::string filepath = sourceOpenFileDialogue(&application::get().internal.windowHandle);
	if (!filepath.empty())
	{
		printf("[ Main Thread ] : Opening %s.\n", filepath.c_str());
		
		// If there is a document open, ask the user if they want to close it.
		if (application::get().currentDocument != nullptr)
			performCloseFile();

		// If the document was closed or not set, the pointer will be nullptr.
		if (application::get().currentDocument == nullptr)
		{
			ui32 maxRecordsParameter = 0;

			application::get().parmetersLock.lock();
			maxRecordsParameter = application::get().recordsPerTable;
			application::get().parmetersLock.unlock();

			application::get().currentDocument = new document(filepath, maxRecordsParameter);

			application::get().parseWindow->open();
		}

	}
}

// -----------------------------------------------------------------------------
// Custom Export Preview Table
// -----------------------------------------------------------------------------

class ExportPreviewTable : public tableviewer
{

	public:
		ExportPreviewTable(std::string name) : tableviewer(name)
		{

		}

	protected:
		inline virtual void
		headerProcedure(ui32 columnOffset, ui32 colCount) override
		{
			// Get the current document and table.
			document& currentDocument = *application::get().currentDocument;
			tablemap& table = currentDocument.getTable();

			if (this->isRowNumbersShown())
				ImGui::TableSetupColumn("");

			ui32 columnsShown = 0;

			for (ui32 cIndex = columnOffset; cIndex < table.getMappedColumns().size(); ++cIndex)
			{
				ImGui::TableSetupColumn(table.getMappedColumns()[cIndex].export_alias.c_str());
				if (++columnsShown >= colCount - 1) break;
			}

			ImGui::TableHeadersRow();
		}

		inline virtual void
		iterationProcedure(ui64 index, ui32 columnOffset, ui32 colCount) override
		{
			// Get the current document and table.
			document& currentDocument = *application::get().currentDocument;
			tablemap& table = currentDocument.getTable();
			std::vector<column_map>& exportMap = table.getMappedColumns();

			record& currentRecord = table[index];

			// Show the y-axis header as needed.
			if (this->isRowNumbersShown())
			{
				ImGui::TableSetColumnIndex(0);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
					ImGui::ColorConvertFloat4ToU32({.20f, .20f, .20f, 1.0f}));
				ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", index + 1);
			}

			ui32 cc = (this->isRowNumbersShown()) ? colCount - 1 : colCount;
			for (ui32 cIndex = 0; cIndex < cc; ++cIndex)
			{

				ImGui::TableSetColumnIndex(cIndex + 1);
				field* f = currentRecord[exportMap[cIndex].column_index];
				if (exportMap[cIndex].all_multivalues)
				{
					ImGui::Text(f->get(exportMap[cIndex].multivalue_delim).c_str());
				}
				else
				{
					ImGui::Text(f->get(exportMap[cIndex].multivalue_index - 1).c_str());
				}

			}

#if 0
			ui32 nextColumn = (this->isRowNumbersShown()) ? 1 : 0;
			ImGui::TableSetColumnIndex(nextColumn);
			ImGui::Text(currentRecord[0]->get(0).c_str());
			ImGui::TableSetColumnIndex(nextColumn+1);
			field* currentField = currentRecord[colIndex];
			if (useAllMulti == false)
			{
				if (currentField != nullptr)
				{
					ImGui::Text(currentField->get(this->multiIndex).c_str());
				}
			}
			else
			{
				if (currentField != nullptr)
				{
					ImGui::Text(currentField->get(delim).c_str());
				}
			}
#endif

#if 0
			ui32 cc = (this->isRowNumbersShown()) ? colCount - 1 : colCount;
			for (ui32 cIndex = 0; cIndex < cc; ++cIndex)
			{

				// Since we are iterating based on the number of columns, we need to
				// manually calculate the column.
				field* currentField = currentRecord[columnOffset + cIndex];


				if (ImGui::TableSetColumnIndex(cIndex + 1)
					&& currentField != nullptr)
				{
					std::string outputString = "";

					if (currentField != nullptr)
						outputString = currentField->get("; ");
					ImGui::Text("%s", outputString.c_str());
				}

			}
#endif
		}
};

// -----------------------------------------------------------------------------
// Custom Mapping Window Table
// -----------------------------------------------------------------------------

class MappingWindowPreviewTable : public tableviewer
{

	public:
		MappingWindowPreviewTable(std::string name) : tableviewer(name)
		{

		}

		ui32 colIndex = -1;
		std::string mapHeaderString = "";
		bool useAllMulti = false;
		ui32 multiIndex = 0;
		std::string delim = ",";
	protected:
		inline virtual void
		headerProcedure(ui32 columnOffset, ui32 colCount) override
		{
			// Get the current document and table.
			document& currentDocument = *application::get().currentDocument;
			tablemap& table = currentDocument.getTable();

			if (this->isRowNumbersShown())
				ImGui::TableSetupColumn("");

			ImGui::TableSetupColumn("Column F0 / Identifying Key");
			ImGui::TableSetupColumn(this->mapHeaderString.c_str());

			ImGui::TableHeadersRow();
		}

		inline virtual void
		iterationProcedure(ui64 index, ui32 columnOffset, ui32 colCount) override
		{
			// Get the current document and table.
			document& currentDocument = *application::get().currentDocument;
			tablemap& table = currentDocument.getTable();

			record& currentRecord = table[index];

			// Show the y-axis header as needed.
			if (this->isRowNumbersShown())
			{
				ImGui::TableSetColumnIndex(0);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
					ImGui::ColorConvertFloat4ToU32({.20f, .20f, .20f, 1.0f}));
				ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", index + 1);
			}

			ui32 nextColumn = (this->isRowNumbersShown()) ? 1 : 0;
			ImGui::TableSetColumnIndex(nextColumn);
			ImGui::Text(currentRecord[0]->get(0).c_str());
			
			ImGui::TableSetColumnIndex(nextColumn+1);
			field* currentField = currentRecord[colIndex];
			if (useAllMulti == false)
			{
				if (currentField != nullptr)
				{
					ImGui::Text(currentField->get(this->multiIndex).c_str());
				}
			}
			else
			{
				if (currentField != nullptr)
				{
					ImGui::Text(currentField->get(delim).c_str());
				}
			}


#if 0
			ui32 cc = (this->isRowNumbersShown()) ? colCount - 1 : colCount;
			for (ui32 cIndex = 0; cIndex < cc; ++cIndex)
			{

				// Since we are iterating based on the number of columns, we need to
				// manually calculate the column.
				field* currentField = currentRecord[columnOffset + cIndex];


				if (ImGui::TableSetColumnIndex(cIndex + 1)
					&& currentField != nullptr)
				{
					std::string outputString = "";

					if (currentField != nullptr)
						outputString = currentField->get("; ");
					ImGui::Text("%s", outputString.c_str());
				}

			}
#endif
		}
};

// -----------------------------------------------------------------------------
// Mapping Window
// -----------------------------------------------------------------------------

class MappingWindow : public GUIWindow
{
	public:
		MappingWindow();
		MappingWindow(ui64 selectedIndex);
		~MappingWindow();

	protected:
		virtual bool onDisplay() 	override;
		virtual bool onOpen() 		override;
		virtual bool onClose() 		override;

	private:
		column_map map;
		MappingWindowPreviewTable previewTable;
};

MappingWindow::
MappingWindow()
	: GUIWindow(true), previewTable("Preview Table")
{
	map.column_index = -1;
	map.all_multivalues = false;
	map.multivalue_index = 1;
}

MappingWindow::
MappingWindow(ui64 selectedIndex)
	: GUIWindow(true), previewTable("Preview Table")
{
	map.column_index = selectedIndex;
	map.all_multivalues = false;
	map.multivalue_index = 1;
}

MappingWindow::
~MappingWindow()
{
	printf("[ Main Thread ] : Mapping window #%d is deconstructing.\n", this->getID());
}

bool MappingWindow::
onOpen()
{
	// Allow the window to open.
	return true;
}

bool MappingWindow::
onClose()
{
	// Allow the window to close.
	return true;
}

bool MappingWindow::
onDisplay()
{

	// Ensure there is a document opened and it is ready.
	if (application::get().currentDocument == nullptr &&
		!application::get().currentDocument->isReady())
			return false;

	document& currentDocument = *application::get().currentDocument;

	std::stringstream mappingtoolname = {};
	mappingtoolname << "Mapping Tool##" << this->getID();

	bool stayOpen = true;

	ImGui::SetNextWindowSize({600.0f, 400.0f}, ImGuiCond_Appearing);
	ImGui::Begin(mappingtoolname.str().c_str(), &stayOpen);

	// -------------------------------------------------------------------------
	// Show Unmapped Columns
	// -------------------------------------------------------------------------
#if 0
	std::stringstream selcol = {};
	selcol << "Selected Column: Column F" << currentMapper.columnSelected;
	ImGui::Text("%s", selcol.str().c_str());

	if (ImGui::BeginListBox("Unmapped Columns", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
	{
		for (ui32 list_index = 0; list_index < 30; ++list_index)
		{
			std::stringstream columnname = {};
			columnname << "Column F" << list_index;

			bool selected = (currentMapper.columnSelected == list_index);
			if (ImGui::Selectable(columnname.str().c_str(), selected))
				currentMapper.columnSelected = list_index;

		}
		ImGui::EndListBox();
	}
#endif

	std::string selectedColumnName = "None Selected";
	if (this->map.column_index != -1)
		selectedColumnName = currentDocument.getTable()
			.getColumnNames()[this->map.column_index];

	std::stringstream listboxHeader = {};
	listboxHeader << "Currently Selected Column: " << selectedColumnName;
	ImGui::Text(listboxHeader.str().c_str());

	if (ImGui::BeginListBox("Columns", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
	{

		// Create a selectable list of columns that can be selected.
		std::vector<std::string>& columnNames = currentDocument.getTable().getColumnNames();
		for (ui32 mapIndex = 0; mapIndex < columnNames.size(); ++mapIndex)
		{
			bool currentlySelected = (this->map.column_index == mapIndex);
			if (ImGui::Selectable(columnNames[mapIndex].c_str(), currentlySelected))
				this->map.column_index = mapIndex;
		}

		ImGui::EndListBox();

	}

	ImGui::Spacing();

	// -------------------------------------------------------------------------
	// Table Mapping Controls
	// -------------------------------------------------------------------------

	ImGui::InputText("Alias Name", &this->map.export_alias);
	ImGui::Spacing();

	ImGui::Checkbox("Use all Multivalues?", &this->map.all_multivalues);
	ImGui::Spacing();

	if (this->map.all_multivalues == false) ImGui::BeginDisabled();
	ImGui::InputText("Multivalue Delimiter", &this->map.multivalue_delim);
	if (this->map.all_multivalues == false) ImGui::EndDisabled();
	

	if (this->map.all_multivalues == true) ImGui::BeginDisabled();
	ImGui::InputInt("Multivalue Index", &this->map.multivalue_index, 1, 1);
	if (this->map.all_multivalues == true) ImGui::EndDisabled();
	ImGui::Spacing();

	ImGui::Separator();
	ImGui::Spacing();

	// Clamp the multi value.
	if (this->map.multivalue_index < 1)
		this->map.multivalue_index = 1;

	// -------------------------------------------------------------------------
	// Show Data Preview
	// -------------------------------------------------------------------------

#if 0
		ui32 maxShown = 32;
		if (currentTable.count() < 32)
			maxShown = currentTable.count();

		ImGui::Text("Preview Values w/in Selected Column");

		if (ImGui::BeginTable("Preview Values", 1, ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
		{
			for (ui32 rowIndex = 0; rowIndex < maxShown; ++rowIndex)
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				std::string str = currentTable[rowIndex][currentMapper.columnSelected][0];
				ImGui::Text("%s", str.c_str());
			}

			ImGui::EndTable();
		}



		// Display the headers.
		ImGui::TableSetupColumn(""); // Top left, 0,0, null cell.
		std::vector<std::string>& headings = table.getColumnNames();
		for (ui32 cnIndex = 0; cnIndex < maxColumns-1; ++cnIndex)
			ImGui::TableSetupColumn(headings[cnIndex].c_str());
		ImGui::TableHeadersRow();

		// Show each record.
		ui32 currentlyShown = 0;
		for (ui32 rowIndex = startingOffset; rowIndex < table.count()
			&& currentlyShown < rowsShown; ++rowIndex)
		{
			ImGui::TableNextRow();

			// Get the record we are showing.
			record& currentRecord = table[rowIndex];

			// Show the y-axis header.
			ImGui::TableSetColumnIndex(0);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
				ImGui::ColorConvertFloat4ToU32({.25f, .25f, .25f, 1.0f}));
			ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", rowIndex + 1);

#endif

	if (this->map.multivalue_index != -1)
	{
		this->previewTable.setColumnStep(2);
		this->previewTable.setPaginationStep(20);

		this->previewTable.mapHeaderString = this->map.export_alias;
		this->previewTable.colIndex = this->map.column_index;
		this->previewTable.delim = this->map.multivalue_delim;
		this->previewTable.multiIndex = this->map.multivalue_index - 1;
		this->previewTable.useAllMulti = this->map.all_multivalues;

		this->previewTable.render(currentDocument.getTable().count(), 2);

#if 0
		ui32 startingOffset = 0;

		ImGui::Text("Preview Values w/in Selected Column");

		tablemap& table = currentDocument.getTable();

		if (ImGui::BeginTable("Preview Values", maxColumns, ImGuiTableFlags_Borders |
				ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
		{

			// Setup the column names.
			ImGui::TableSetupColumn(""); // Top left, 0,0, null cell.
			ImGui::TableSetupColumn("Column F0 / ID");
			ImGui::TableSetupColumn(this->mapName.c_str());
			ImGui::TableHeadersRow();

			ui32 recordsShowned = 0;
			for (ui32 rowIndex = startingOffset; rowIndex < table.count(); ++rowIndex)
			{

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
					ImGui::ColorConvertFloat4ToU32({.25f, .25f, .25f, 1.0f}));
				ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", rowIndex + 1);

				//std::string str = currentTable[rowIndex][currentMapper.columnSelected][0];
				//ImGui::Text("%s", str.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text(table[rowIndex][0]->get(0).c_str());

				ImGui::TableSetColumnIndex(2);
				ImGui::Text(table[rowIndex][this->columnIndexSelected]->get(this->multivalueSelected - 1).c_str());

				if (recordsShowned++ >= recordsPerPage) break;
			
			}

			ImGui::EndTable();
		}
#endif
	}

	// -------------------------------------------------------------------------
	// Save only if a column is mapped.
	// -------------------------------------------------------------------------

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Please examine preview before saving.");
	ImGui::Spacing();
	ImGui::Text("Once saved, check the export preview tab.");
	ImGui::Spacing();

	bool saveDisabled = false;
	if (this->map.export_alias.empty())
	{
		saveDisabled = true;
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Save Map"))
	{
		currentDocument.lockTable();
		currentDocument.getTable().getMappedColumns().push_back(this->map);
		currentDocument.unlockTable();
		stayOpen = false; // Saved, we can close this window.
	}
	if (saveDisabled == true) ImGui::EndDisabled();

	ImGui::End();

	return stayOpen;

}

// -----------------------------------------------------------------------------
// Export Window
// -----------------------------------------------------------------------------

class ExportWindow : public GUIWindow
{
	public:
		ExportWindow();
		~ExportWindow();

	protected:
		virtual bool onDisplay() 	override;
		virtual bool onOpen() 		override;
		virtual bool onClose() 		override;
		virtual void onExit(bool) 	override;

	private:
		bool popupOpened = false;
};

ExportWindow::
ExportWindow()
{ }

ExportWindow::
~ExportWindow()
{ }

bool ExportWindow::
onOpen()
{
	// Allow the window to open.
	return true;
}

bool ExportWindow::
onClose()
{
	// Allow the window to close.
	popupOpened = false;
	return true;
}

void ExportWindow::
onExit(bool destroyed)
{

}

bool ExportWindow::
onDisplay()
{

	// -------------------------------------------------------------------------
	// Ensure that there is a valid document.
	// -------------------------------------------------------------------------
	
	if (application::get().currentDocument == nullptr)
		return false;

	document& currentDocument = *application::get().currentDocument;


	// -------------------------------------------------------------------------
	// Parsing Progress Display
	// -------------------------------------------------------------------------
	
	// Used to determine if the window should stay open or closed.
	bool stayOpen = true;

	// Disallow the closing of the window if the document hasn't finished loading.
	bool* windowCloseButton = nullptr;
	if (currentDocument.isExportComplete()) windowCloseButton = &stayOpen;

	if (this->popupOpened == false)
	{
		ImGui::OpenPopup("Export Progress");
		this->popupOpened = true;
	}

	//ImGui::SetNextWindowSize({0, 0}, ImGuiCond_Always);
	if (ImGui::BeginPopupModal("Export Progress", windowCloseButton, ImGuiWindowFlags_AlwaysAutoResize))
	{
#if 0
		ImGui::Text("Parsing %s, please wait as this may take awhile.",
			currentDocument.getFilename().c_str());
		ImGui::Spacing();
		ImGui::Separator();

		auto pprog = currentDocument.getParsingProgress();
		for (ui32 progressIndex = 0; progressIndex < pprog.size(); ++progressIndex)
		{
			parseprogress& cprog = pprog[progressIndex];
			r32 pfloat = (r32)cprog.recordsProcessed / (r32)cprog.recordsTotal;
			r32 perc = (pfloat * 100.0f);

			std::stringstream percstr = {};
			percstr << std::fixed << std::setprecision(0);
			if (pfloat > .98)
				percstr << "Done!";
			else
				percstr << perc << "%";

			ImGui::Spacing();
			ImGui::Text("Parsing Subtable: %d", progressIndex + 1);
			ImGui::Spacing();
			ImGui::ProgressBar(pfloat, {}, percstr.str().c_str());
			ImGui::SameLine();
			ImGui::Text("%d / %d \n", cprog.recordsProcessed, cprog.recordsTotal);
			ImGui::Spacing();
		}
#endif

		ImGui::Separator();

		// -------------------------------------------------------------------------
		// Close Button
		// -------------------------------------------------------------------------

		// Disallow the use of the vlose button if we aren't allowed to close the
		// window.
		if (windowCloseButton == nullptr) ImGui::BeginDisabled();
		ImGui::Spacing();
		if (ImGui::Button("Close Window"))
			stayOpen = false;

		if (windowCloseButton == nullptr)
		{
			ImGui::SameLine();
			ImGui::Text("Please wait...");
			ImGui::EndDisabled();
		}

		ImGui::EndPopup();
	}


	

	return stayOpen;

}

// -----------------------------------------------------------------------------
// Parsing Window
// -----------------------------------------------------------------------------

class ParsingWindow : public GUIWindow
{
	public:
		ParsingWindow();
		~ParsingWindow();

	protected:
		virtual bool onDisplay() 	override;
		virtual bool onOpen() 		override;
		virtual bool onClose() 		override;
		virtual void onExit(bool) 	override;

	private:
		bool popupOpened = false;
};

ParsingWindow::
ParsingWindow()
{ }

ParsingWindow::
~ParsingWindow()
{ }

bool ParsingWindow::
onOpen()
{
	// Allow the window to open.
	return true;
}

bool ParsingWindow::
onClose()
{
	// Allow the window to close.
	popupOpened = false;
	return true;
}

void ParsingWindow::
onExit(bool destroyed)
{

}

bool ParsingWindow::
onDisplay()
{

	// -------------------------------------------------------------------------
	// Ensure that there is a valid document.
	// -------------------------------------------------------------------------
	
	if (application::get().currentDocument == nullptr)
		return false;

	document& currentDocument = *application::get().currentDocument;


	// -------------------------------------------------------------------------
	// Parsing Progress Display
	// -------------------------------------------------------------------------
	
	// Used to determine if the window should stay open or closed.
	bool stayOpen = true;

	// Disallow the closing of the window if the document hasn't finished loading.
	bool* windowCloseButton = nullptr;
	if (currentDocument.isReady()) windowCloseButton = &stayOpen;

	if (this->popupOpened == false)
	{
		ImGui::OpenPopup("Document Parse Progress");
		this->popupOpened = true;
	}

	ImGui::SetNextWindowSize({0, 0}, ImGuiCond_Always);
	if (ImGui::BeginPopupModal("Document Parse Progress", windowCloseButton, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Parsing %s, please wait as this may take awhile.",
			currentDocument.getFilename().c_str());
		ImGui::Spacing();
		ImGui::Separator();

		auto pprog = currentDocument.getParsingProgress();
		for (ui32 progressIndex = 0; progressIndex < pprog.size(); ++progressIndex)
		{
			parseprogress& cprog = pprog[progressIndex];
			r32 pfloat = (r32)cprog.recordsProcessed / (r32)cprog.recordsTotal;
			r32 perc = (pfloat * 100.0f);

			std::stringstream percstr = {};
			percstr << std::fixed << std::setprecision(0);
			if (pfloat > .98)
				percstr << "Done!";
			else
				percstr << perc << "%";

			ImGui::Spacing();
			ImGui::Text("Parsing Subtable: %d", progressIndex + 1);
			ImGui::Spacing();
			ImGui::ProgressBar(pfloat, {}, percstr.str().c_str());
			ImGui::SameLine();
			ImGui::Text("%d / %d \n", cprog.recordsProcessed, cprog.recordsTotal);
			ImGui::Spacing();
		}


		ImGui::Separator();

		// -------------------------------------------------------------------------
		// Close Button
		// -------------------------------------------------------------------------

		// Disallow the use of the vlose button if we aren't allowed to close the
		// window.
		if (windowCloseButton == nullptr) ImGui::BeginDisabled();
		ImGui::Spacing();
		if (ImGui::Button("Close Window"))
			stayOpen = false;

		if (windowCloseButton == nullptr)
		{
			ImGui::SameLine();
			ImGui::Text("Please wait...");
			ImGui::EndDisabled();
		}

		ImGui::EndPopup();
	}


	

	return stayOpen;

}

// ---------------------------------------------------------------------------------------------------------------------
// Main Window
// ---------------------------------------------------------------------------------------------------------------------

class MainWindow : public GUIWindow
{

	public:
		MainWindow();
		~MainWindow();

	protected:
		virtual bool onDisplay() 	override;
		virtual bool onClose() 		override;
		virtual bool onOpen() 		override;

	protected:
		void displayMenubar();
		void displaySubmenuFile();
		void displaySubmenuTableMapping();

		void displayExportPreview();

		void saveMappingSchema();
		void loadMappingSchema();

		tableviewer rawViewTable;
		ExportPreviewTable exportPreview;

};

MainWindow::
MainWindow() : GUIWindow(true), rawViewTable("Raw Import Data View"), exportPreview("Export Preview Table")
{ }

MainWindow::
~MainWindow()
{ }

bool MainWindow::
onOpen()
{
	return true;
}

bool MainWindow::
onClose()
{
	return true;
}

bool MainWindow::
onDisplay()
{

	// This section defines the main work area.
	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos({0,0});
	ImGui::SetNextWindowSize(mainViewport->WorkSize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("Manage Kinetic", nullptr, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|
		ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar|
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	// We are always displaying the main menubar.
	this->displayMenubar();

	if (application::get().currentDocument != nullptr)
	{

		document& currentDocument = *application::get().currentDocument;

		if (ImGui::BeginTabBar("Table Display"))
		{

			if (ImGui::BeginTabItem("Imported Table Data View"))
			{
				ImGui::Text("Imported Table Data View Reference");
				ImGui::Spacing();

				if (currentDocument.isReady())
					this->rawViewTable.render(currentDocument.getTable().count(),
						currentDocument.getTable().columnsCount());
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Export Preview"))
			{
				if (currentDocument.isReady())
					this->exportPreview.render(currentDocument.getTable().count(),
						currentDocument.getTable().getMappedColumns().size());
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	//this->displayExportPreview();

	// Close the main work area.
	ImGui::End();
	ImGui::PopStyleVar();

	return true;
}

void MainWindow::
displayExportPreview()
{
	if (application::get().currentDocument != nullptr)
	{

		// Get the current document since it is valid.
		document& currentDocument = *application::get().currentDocument;

		// Show the table for the data.
		if (currentDocument.isReady())
		{
#if 0
			// -----------------------------------------------------------------
			// Setup the export table data.
			// -----------------------------------------------------------------

			tablemap& table = currentDocument.getTable();

			// Determine how many columns this record supports.
			ui64 maxColumns = 64;
			if (table.columnsCount() < maxColumns)
				maxColumns = table.columnsCount();

			const ui64 rowsShown = 100;

			ui64 totalPages = (table.count() / rowsShown);
			if (table.count() % rowsShown != 0) totalPages++;
			i32& currentPage = currentDocument.currentPreviewPage;

			ui64 startingOffset = (currentPage * rowsShown);
			ui64 endingOffset = (currentPage * rowsShown) + rowsShown;

			// -----------------------------------------------------------------
			// Pagination Controls
			// -----------------------------------------------------------------

			ImGui::Text("Pagination Controls");
			ImGui::Spacing();

			bool leftArrowDisabled = false;
			if (currentPage <= 0)
			{
				ImGui::BeginDisabled();
				leftArrowDisabled = true;
			}
			if (ImGui::ArrowButton("##page-left", ImGuiDir_Left))
				currentPage--;
			if (leftArrowDisabled == true) ImGui::EndDisabled();

			ImGui::SameLine();

			bool rightArrowDisabled = false;
			if (currentPage >= totalPages - 1)
			{
				ImGui::BeginDisabled();
				rightArrowDisabled = true;
			}
			if (ImGui::ArrowButton("##page-right", ImGuiDir_Right))
				currentPage++;
			if (rightArrowDisabled == true) ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::Text("Page %i / %i", currentPage+1, totalPages);

			// -----------------------------------------------------------------
			// Display the table.
			// -----------------------------------------------------------------

			if (ImGui::BeginTable("Mapping Table", maxColumns, ImGuiTableFlags_Borders|ImGuiTableFlags_ScrollX|ImGuiTableFlags_ScrollY
				|ImGuiTableFlags_Resizable|ImGuiTableFlags_RowBg,
				ImGui::GetContentRegionAvail()))
			{

				// Display the headers.
				ImGui::TableSetupColumn(""); // Top left, 0,0, null cell.
				std::vector<std::string>& headings = table.getColumnNames();
				for (ui32 cnIndex = 0; cnIndex < maxColumns-1; ++cnIndex)
					ImGui::TableSetupColumn(headings[cnIndex].c_str());
				ImGui::TableHeadersRow();

				// Show each record.
				ui32 currentlyShown = 0;
				for (ui32 rowIndex = startingOffset; rowIndex < table.count()
					&& currentlyShown < rowsShown; ++rowIndex)
				{
					ImGui::TableNextRow();

					// Get the record we are showing.
					record& currentRecord = table[rowIndex];

					// Show the y-axis header.
					ImGui::TableSetColumnIndex(0);
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
						ImGui::ColorConvertFloat4ToU32({.25f, .25f, .25f, 1.0f}));
					ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", rowIndex + 1);

					for (ui32 colIndex = 0; colIndex < maxColumns - 1; ++colIndex)
					{
						field* currentField = currentRecord[colIndex];
						if (ImGui::TableSetColumnIndex(colIndex + 1)
							&& currentField != nullptr)
						{
							std::string outputString = "";
							if (currentField != nullptr)
								outputString = currentField->get("; ");
							ImGui::Text("%s", outputString.c_str());
						}
					}

					currentlyShown++;
				}

				ImGui::EndTable();
			}
#endif

			this->rawViewTable.render(currentDocument.getTable().count(),
				currentDocument.getTable().getColumnNames().size());

		}

	}
}

void MainWindow::
displaySubmenuTableMapping()
{

	bool mappingMenuDisabled = false;
	if (application::get().currentDocument == nullptr ||
		!application::get().currentDocument->isReady())
	{
		mappingMenuDisabled = true;
		ImGui::BeginDisabled();
	}

	if (ImGui::BeginMenu("Table Mapping"))
	{

		if (ImGui::MenuItem("New Column Map"))
			application::get().mappingWindows.push_back(std::make_unique<MappingWindow>());

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::MenuItem("Save Tablemap Schema"))
			this->saveMappingSchema();

		if (ImGui::MenuItem("Load Tablemap Schema"))
			this->loadMappingSchema();

		ImGui::EndMenu();
	}

	if (mappingMenuDisabled == true) ImGui::EndDisabled();

}

void MainWindow::
loadMappingSchema()
{

	printf("[ Main Thread ] : Loading mapping schema.\n");



}

void MainWindow::
saveMappingSchema()
{

	printf("[ Main Thread ] : Saving mapping schema.\n");



}

void MainWindow::
displaySubmenuFile()
{

	if (ImGui::BeginMenu("File", true))
	{

		if (ImGui::MenuItem("Open File")) performOpenFile();

		if (ImGui::MenuItem("Close File")) performCloseFile();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		bool saveDisabled = false;
		if (application::get().currentDocument == nullptr || (application::get().currentDocument != nullptr &&
			application::get().currentDocument->isReady() &&
			application::get().currentDocument->getTable().getMappedColumns().size() == 0))
				saveDisabled = true;

		if (saveDisabled) ImGui::BeginDisabled();
		if (ImGui::MenuItem("Export As")) performSaveFile();
		if (saveDisabled) ImGui::EndDisabled();

		// -----------------------------------------------------------------
		// About window which displays information about the program.
		// -----------------------------------------------------------------

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::SetNextWindowSize({300.0f, 300.0f}, ImGuiCond_Appearing);
		if (ImGui::BeginMenu("About"))
		{

			std::stringstream authorString = {};
			authorString << "Created by Chris DeJong, Aircraft Gear Corp," << " 2022-23,"
				<< " " << application::get().currentVersion;
			ImGui::BeginGroup();
			ImGui::TextWrapped("About Manage Kinetic");
			ImGui::Separator();
			ImGui::TextWrapped(authorString.str().c_str());
			ImGui::Separator();
			ImGui::TextWrapped("A data conversion utility which translates table data from"
				" Manage 2000 and allows them to be mapped to tables within Epicor Kinetic."
				" The data is then exported to Microsoft Excel .xlsx files for import into"
				" Epicor Kinetic."
				" See the README document for more information on how to use Manage Kinetic.");
			ImGui::EndGroup();

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}

}

void MainWindow::
displayMenubar()
{

	// Get the application state.
	application& applicationState = application::get();

	// Menu Bar Window
	if (ImGui::BeginMenuBar())
	{
		// Mode name.
		ImGui::TextColored({.96f, .45f, .3f, 1.0f}, "Manage Kinetic");
		ImGui::Separator();

		// Display the filename in the header.
		std::string defaultHeaderString = applicationState.currentVersion;
		std::string viewDocumentString 	= {};
		if (application::get().currentDocument)
			viewDocumentString = application::get().currentDocument->getFilename();
		else
			viewDocumentString = defaultHeaderString;

		ImGui::TextColored({.66f, .66f, .66f, 1.0f}, viewDocumentString.c_str());
		ImGui::Separator();

		// ---------------------------------------------------------------------
		// Determine if the menu needs to disabled.
		// ---------------------------------------------------------------------
		bool menuDisabled = false;
		if (application::get().currentDocument != nullptr)
		{
			if (application::get().currentDocument->isInitializing() ||
				application::get().currentDocument->isParsing())
			{
				ImGui::BeginDisabled();
				menuDisabled = true;
			}
		}


		this->displaySubmenuFile();
		this->displaySubmenuTableMapping();

		ImGui::EndMenuBar();

		if (menuDisabled == true) ImGui::EndDisabled();

	}

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

// ---------------------------------------------------------------------------------------------------------------------
// Application Main & Main Thread Procedures
// ---------------------------------------------------------------------------------------------------------------------

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void HelpMarker(const char* desc);

void showFileMenu()
{

	if (ImGui::BeginMenu("File", true))
	{

		// -----------------------------------------------------------------
		// Open file dialogue will create a new document to work with.
		// -----------------------------------------------------------------

		if (ImGui::MenuItem("Open File")) performOpenFile();
		ImGui::Spacing();

		// -----------------------------------------------------------------
		// Close file dialogue will delete the document.
		// -----------------------------------------------------------------

		if (ImGui::MenuItem("Close File")) performCloseFile();
		ImGui::Spacing();

		// -----------------------------------------------------------------
		// About window which displays information about the program.
		// -----------------------------------------------------------------

		ImGui::Separator();
		ImGui::Spacing();
		ImGui::SetNextWindowSize({300.0f, 300.0f}, ImGuiCond_Appearing);
		if (ImGui::BeginMenu("About"))
		{
			ImGui::BeginGroup();
			ImGui::TextWrapped("About Manage Kinetic");
			ImGui::Separator();
			ImGui::TextWrapped("Created by Chris DeJong, Aircraft Gear Corp,"
				" 2022-23, Version 0.2.1i");
			ImGui::Separator();
			ImGui::TextWrapped("A data conversion utility which translates table data from"
				" Manage 2000 and allows them to be mapped to tables within Epicor Kinetic."
				" The data is then exported to Microsoft Excel .xlsx files for import into"
				" Epicor Kinetic."
				" See the README document for more information on how to use Manage Kinetic.");
			ImGui::EndGroup();

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}

}

void
showMappingMenu()
{

	bool mappingMenuDisabled = false;
	if (application::get().currentDocument == nullptr ||
		!application::get().currentDocument->isReady())
	{
		mappingMenuDisabled = true;
		ImGui::BeginDisabled();
	}

	if (ImGui::BeginMenu("Table Mapping"))
	{

		if (ImGui::MenuItem("New Column Map"))
			application::get().mappingWindows.push_back(std::make_unique<MappingWindow>());

		ImGui::EndMenu();
	}

	if (mappingMenuDisabled == true) ImGui::EndDisabled();

}
#if 0
void
displayMainMenubar()
{

	// Get the application state.
	application& applicationState = application::get();

	// Menu Bar Window
	if (ImGui::BeginMenuBar())
	{
		// Mode name.
		ImGui::TextColored({.96f, .96f, .13f, 1.0f}, "Manage Kinetic");
		ImGui::Separator();

		// Display the filename in the header.
		std::string defaultHeaderString = "Version 0.2.1i";
		std::string viewDocumentString 	= {};
		if (application::get().currentDocument)
			viewDocumentString = application::get().currentDocument->getFilename();
		else
			viewDocumentString = defaultHeaderString;

		ImGui::TextColored({.1f, .96f, .96f, .66f}, viewDocumentString.c_str());
		ImGui::Separator();

		// ---------------------------------------------------------------------
		// Determine if the menu needs to disabled.
		// ---------------------------------------------------------------------
		bool menuDisabled = false;
		if (application::get().currentDocument != nullptr)
		{
			if (application::get().currentDocument->isInitializing() ||
				application::get().currentDocument->isParsing())
			{
				ImGui::BeginDisabled();
				menuDisabled = true;
			}
		}

		// ---------------------------------------------------------------------
		// File menu.
		// ---------------------------------------------------------------------

		showFileMenu();

		// ---------------------------------------------------------------------
		// Table map menu.
		// ---------------------------------------------------------------------

		showMappingMenu();

		ImGui::EndMenuBar();

		if (menuDisabled == true) ImGui::EndDisabled();

	}

}
#endif
#if 0
void
showMappingTool()
{

	if (application::get().currentMapper != nullptr)
	{

		// Get the current mapper.
		mapper& currentMapper = *application::get().currentMapper;

		// Get a table to examime.
		//MappingTable& currentTable = *application::get().currentDocument->operator[](0);

		// Create the mapper window.
		ImGui::SetNextWindowSize({400.0f, 500.0f}, ImGuiCond_Once);
		ImGui::Begin("Table Mapper Tool", &currentMapper.mapperOpen, ImGuiWindowFlags_NoCollapse);

		// -----------------------------------------------------------------
		// Offer a list of available columns to map.
		// -----------------------------------------------------------------

		std::stringstream selcol = {};
		selcol << "Selected Column: Column F" << currentMapper.columnSelected;
		ImGui::Text("%s", selcol.str().c_str());

		if (ImGui::BeginListBox("Unmapped Columns", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (ui32 list_index = 0; list_index < 30; ++list_index)
			{
				std::stringstream columnname = {};
				columnname << "Column F" << list_index;

				bool selected = (currentMapper.columnSelected == list_index);
				if (ImGui::Selectable(columnname.str().c_str(), selected))
					currentMapper.columnSelected = list_index;

			}
			ImGui::EndListBox();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// -----------------------------------------------------------------
		// Preview the data to the user.
		// -----------------------------------------------------------------
#if 0
		ui32 maxShown = 32;
		if (currentTable.count() < 32)
			maxShown = currentTable.count();

		ImGui::Text("Preview Values w/in Selected Column");

		if (ImGui::BeginTable("Preview Values", 1, ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
		{
			for (ui32 rowIndex = 0; rowIndex < maxShown; ++rowIndex)
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				std::string str = currentTable[rowIndex][currentMapper.columnSelected][0];
				ImGui::Text("%s", str.c_str());
			}

			ImGui::EndTable();
		}
#endif
		ImGui::End();

		if (currentMapper.mapperOpen != true)
			if (closeColumnMapper() == false) currentMapper.mapperOpen = true;

	}

}
#endif
void
showExportPreview()
{
	if (application::get().currentDocument != nullptr)
	{

		// Get the current document since it is valid.
		document& currentDocument = *application::get().currentDocument;

		// Show the table for the data.
		if (currentDocument.isReady())
		{

			// -----------------------------------------------------------------
			// Setup the export table data.
			// -----------------------------------------------------------------

			tablemap& table = currentDocument.getTable();

			// Determine how many columns this record supports.
			ui64 maxColumns = 64;
			if (table.columnsCount() < maxColumns)
				maxColumns = table.columnsCount();

			const ui64 rowsShown = 100;

			ui64 totalPages = (table.count() / rowsShown);
			if (table.count() % rowsShown != 0) totalPages++;
			i32& currentPage = currentDocument.currentPreviewPage;

			ui64 startingOffset = (currentPage * rowsShown);
			ui64 endingOffset = (currentPage * rowsShown) + rowsShown;

			// -----------------------------------------------------------------
			// Pagination Controls
			// -----------------------------------------------------------------

			ImGui::Text("Pagination Controls");
			ImGui::Spacing();

			bool leftArrowDisabled = false;
			if (currentPage <= 0)
			{
				ImGui::BeginDisabled();
				leftArrowDisabled = true;
			}
			if (ImGui::ArrowButton("##page-left", ImGuiDir_Left))
				currentPage--;
			if (leftArrowDisabled == true) ImGui::EndDisabled();

			ImGui::SameLine();

			bool rightArrowDisabled = false;
			if (currentPage >= totalPages - 1)
			{
				ImGui::BeginDisabled();
				rightArrowDisabled = true;
			}
			if (ImGui::ArrowButton("##page-right", ImGuiDir_Right))
				currentPage++;
			if (rightArrowDisabled == true) ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::Text("Page %i / %i", currentPage+1, totalPages);

			// -----------------------------------------------------------------
			// Display the table.
			// -----------------------------------------------------------------

			if (ImGui::BeginTable("Mapping Table", maxColumns, ImGuiTableFlags_Borders|ImGuiTableFlags_ScrollX|ImGuiTableFlags_ScrollY
				|ImGuiTableFlags_Resizable|ImGuiTableFlags_RowBg,
				ImGui::GetContentRegionAvail()))
			{

				// Display the headers.
				ImGui::TableSetupColumn(""); // Top left, 0,0, null cell.
				std::vector<std::string>& headings = table.getColumnNames();
				for (ui32 cnIndex = 0; cnIndex < maxColumns-1; ++cnIndex)
					ImGui::TableSetupColumn(headings[cnIndex].c_str());
				ImGui::TableHeadersRow();

				// Show each record.
				ui32 currentlyShown = 0;
				for (ui32 rowIndex = startingOffset; rowIndex < table.count()
					&& currentlyShown < rowsShown; ++rowIndex)
				{
					ImGui::TableNextRow();

					// Get the record we are showing.
					record& currentRecord = table[rowIndex];

					// Show the y-axis header.
					ImGui::TableSetColumnIndex(0);
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
						ImGui::ColorConvertFloat4ToU32({.25f, .25f, .25f, 1.0f}));
					ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", rowIndex + 1);

					for (ui32 colIndex = 0; colIndex < maxColumns - 1; ++colIndex)
					{
						field* currentField = currentRecord[colIndex];
						if (ImGui::TableSetColumnIndex(colIndex + 1)
							&& currentField != nullptr)
						{
							std::string outputString = "";
							if (currentField != nullptr)
								outputString = currentField->get("; ");
							ImGui::Text("%s", outputString.c_str());
						}
					}

					currentlyShown++;
				}

				ImGui::EndTable();
			}

		}

	}
}
#if 0
void
displayMainWindow()
{
	// This section defines the main work area.
	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos({0,0});
	ImGui::SetNextWindowSize(mainViewport->WorkSize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("Manage Kinetic", nullptr, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|
		ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar|
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	// We are always displaying the main menubar.
	displayMainMenubar();

	// Close the main work area.
	ImGui::End();
	ImGui::PopStyleVar();
}
#endif
// ---------------------------------------------------------------------------------------------------------------------
// Windows Main
// ---------------------------------------------------------------------------------------------------------------------

void
RenderFrame(HWND windowHandle, HDC OpenGLDeviceContext, HGLRC OpenGLRenderContext, bool disableAll = false)
{

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// -------------------------------------------------------------------------
	// Main Window Render Area
	// -------------------------------------------------------------------------

#if 0
	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos({0,0});
	ImGui::SetNextWindowSize(mainViewport->WorkSize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (disableAll == true) ImGui::BeginDisabled();
	ImGui::Begin("Manage Kinetic", nullptr, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|
		ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar|
		ImGuiWindowFlags_NoBringToFrontOnFocus);


	displayMainMenubar();	// The main menu bar for the main window.
	showExportPreview(); 	// The export preview for the main window.

	ImGui::End();
	ImGui::PopStyleVar();
	if (disableAll == true) ImGui::EndDisabled();
#endif

	if (disableAll == true) ImGui::BeginDisabled();

	application::get().mainWindow->display();

	if (disableAll == true) ImGui::EndDisabled();

	// -------------------------------------------------------------------------
	// Client Interface Windows
	// -------------------------------------------------------------------------

	if (disableAll == true) ImGui::BeginDisabled();
	
	// Show the parsing window.
	application::get().parseWindow->display();

	// Show the export window.
	application::get().exportWindow->display();

	// Display all the mapping windows currently open.
	for (std::unique_ptr<GUIWindow>& currentMappingWindow : application::get().mappingWindows)
		currentMappingWindow->display();
	
	if (disableAll == true) ImGui::EndDisabled();


	// ---------------------------------------------------------------------
	// ImGUI Demo Window
	// ---------------------------------------------------------------------

#define MANAGEKINETIC_DEBUG
#if defined(MANAGEKINETIC_DEBUG)
	if (disableAll == true) ImGui::BeginDisabled();
	// Show the demo window.
	static bool showDemoWindowFlag = false;
	if (showDemoWindowFlag)
		ImGui::ShowDemoWindow(&showDemoWindowFlag);
	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_B))
		showDemoWindowFlag = showDemoWindowFlag ? false : true;
	if (disableAll == true) ImGui::EndDisabled();
#endif

	// ---------------------------------------------------------------------
	// Post the Frame to the GPU
	// ---------------------------------------------------------------------

	// Render ImGui components.
	ImGui::Render();

	// Retrieve the current size of the window.
	RECT windowClientSize = {};
	GetClientRect(windowHandle, &windowClientSize);
	i32 windowWidth = windowClientSize.right - windowClientSize.left;
	i32 windowHeight = windowClientSize.top - windowClientSize.bottom;

	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(.2f, .2f, .2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	wglMakeCurrent(OpenGLDeviceContext, OpenGLRenderContext);
	SwapBuffers(OpenGLDeviceContext);

}

LRESULT CALLBACK
Win32WindowProcedure(HWND windowHandle, UINT message, WPARAM wideParam, LPARAM lowParam)
{

	switch (message)
	{
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_SIZING:
		{
			// We need to render a new frame since the window is resizing.
			// We disable all elements for interaction.
			RenderFrame(windowHandle, GetDC(application::get().internal.windowHandle),
				application::get().internal.OpenGLRenderContext, true);
			return 0;
		}
		case WM_SETCURSOR:
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			break;
		}
		case WM_PAINT:
		{
			break;
		}
	}
	// For the messages we don't collect.
	return DefWindowProc(windowHandle, message, wideParam, lowParam);
}

i32 WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR commandline, i32 commandShow)
{

	// -------------------------------------------------------------------------
	// Allocate a Console for Output
	// -------------------------------------------------------------------------
	FreeConsole();
	if (AllocConsole())
	{
		SetConsoleTitleA("Manage Kinetic Status Console");
		typedef struct { char* _ptr; int _cnt; char* _base; int _flag; int _file; int _charbuf; int _bufsiz; char* _tmpfname; } FILE_COMPLETE;
#pragma warning(push)
#pragma warning(disable: 4311)
		*(FILE_COMPLETE*)stdout = *(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT), "w");
		*(FILE_COMPLETE*)stderr = *(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT), "w");
		*(FILE_COMPLETE*)stdin = *(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT), "r");
#pragma warning(pop)
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
		setvbuf(stdin, NULL, _IONBF, 0);
		printf("[ Main Thread ] : Console has been initialized.\n");
	}
	else
	{
		// Ensure that we are running a console at startup.
		return -1;
	}

#if 0
	// -------------------------------------------------------------------------
	// Startup procedures, runtime benchmarks, and other initialization procs.
	// -------------------------------------------------------------------------

	shared_thread_context sharedThreadContext = {};
	runtime_constants rtConstants = {};
	sharedThreadContext.rtConsts = &rtConstants;

	// Parse the command line.
	std::string operationFilepath = {};
	b32 commandlineParseResults = parseCommandlineArguments(argc, argv,
		operationFilepath, &rtConstants);

	// Handle the errors.
	if (commandlineParseResults != CLPARSE_RESULTS_VALID)
	{
		if (commandlineParseResults == CLPARSE_RESULTS_NOARGS)
			std::cerr << "Please provide a path to a file containing the unidata table to process.\n";
		else if (commandlineParseResults == CLPARSE_RESULTS_NOEXIST)
			std::cerr << "The provided filepath doesn't exist.\n";
		else if (commandlineParseResults == CLPARSE_RESULTS_INVALIDTHREADS_PARAM)
			std::cerr << "Invalid thread count parameter. See help for more information.\n";
		else if (commandlineParseResults == CLPARSE_RESULTS_MAXRECORDSDOC_PARAM)
			std::cerr << "Invalid max records per document parameter. See help for more information.\n";
		showHelpDialogue();
		return -1;
	}


	// -------------------------------------------------------------------------
	// Load the Unidata table into memory.
	// -------------------------------------------------------------------------

	// Determine the size of the file and then allocate enough memory to load it into memory.
	ui64 operationFilesize = sourceFileSize(operationFilepath); // Memory is automatically init'd to zero.
	void* sourceFileBuffer = virtualHeapAllocate(operationFilesize + 1);

	// Load the source file into memory.
	ui64 bytesRead = sourceFileOpen(sourceFileBuffer, operationFilesize, operationFilepath, operationFilesize);
	if (bytesRead != operationFilesize)
	{
		std::cerr << "Unable to read " << operationFilepath << " into memory.\n";
		return -1;
	}

	// Once the file is read, cast down to a character array.
	char* rawSourceFilePtr = (char*)sourceFileBuffer;

	// -------------------------------------------------------------------------
	// Prepare the work queue, deploy the workers, and pray.
	// -------------------------------------------------------------------------

	// Prepare the document process queue for the worker threads.
	RecordsTableQueue documentProcessQueue = createDocumentQueueFromSource(rawSourceFilePtr, sharedThreadContext.rtConsts);
	sharedThreadContext.documentQueue = &documentProcessQueue;
	sharedThreadContext.queueIndex = 0;

	// Flog the hamsters.
	std::vector<std::thread> workers = {};
	for (ui64 i = 1; i < sharedThreadContext.rtConsts->concurrentThreadsCount; ++i)
	{
		std::thread currentThread(tProcedureMain, i+1, &sharedThreadContext);
		workers.push_back(std::move(currentThread));
	}

	// Main thread join, then join the rest after.
	tProcedureMain(0, &sharedThreadContext);
	for (std::thread& currentThread : workers)
	{
		if (currentThread.joinable())
			currentThread.join();
	}

	return 0;
#endif

	// -------------------------------------------------------------------------
	// Initialize the Win32 Window
	// -------------------------------------------------------------------------

	WNDCLASSA windowClass = {};
	windowClass.lpfnWndProc = Win32WindowProcedure;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "ManageKinetic";
	windowClass.hbrBackground = NULL;

	RegisterClassA(&windowClass);

	HWND windowHandle = CreateWindowExA(NULL, "ManageKinetic", "Manage Kinetic Conversion Utility",
		WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
		hInstance, NULL);

	if (windowHandle == NULL)
	{
		printf("[ Main Thread ] : Unable to create the window.\n");
		printf("[ Main Thread ] : ");
		system("pause");
		return -1;
	}

	ShowWindow(windowHandle, commandShow);

	// -------------------------------------------------------------------------
	// Initialize the OpenGL Render Context
	// -------------------------------------------------------------------------

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	HDC OpenGLDeviceContext = GetDC(windowHandle);

	int pixelFormal = ChoosePixelFormat(OpenGLDeviceContext, &pfd); 
	SetPixelFormat(OpenGLDeviceContext, pixelFormal, &pfd);
	HGLRC OpenGLRenderContext = wglCreateContext(OpenGLDeviceContext);
	wglMakeCurrent(OpenGLDeviceContext, OpenGLRenderContext);

	((BOOL(WINAPI*)(int))wglGetProcAddress("wglSwapIntervalEXT"))(1);

	// -------------------------------------------------------------------------
	// Initialize ImGUI Interface
	// -------------------------------------------------------------------------

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsRCV();

	// Setup Platform/Renderer backends
	if (!ImGui_ImplWin32_Init(windowHandle))
	{
		printf("[ Main Thread ] : Win32 ImGUI initialization failed.\n");
		printf("[ Main Thread ] : ");
		system("pause");
		return -1;
	}

	if (!ImGui_ImplOpenGL3_Init("#version 410 core"))
	{
		printf("[ Main Thread ] : OpenGL3 ImGUI initialization failed.\n");
		printf("[ Main Thread ] : ");
		system("pause");
		return -1;
	}

	// -------------------------------------------------------------------------
	// Initialize Application
	// -------------------------------------------------------------------------

	application& appRuntime 	= application::get();
	appRuntime.internal.runtimeFlag 	= true;
	appRuntime.internal.windowHandle 	= windowHandle;
	appRuntime.internal.OpenGLRenderContext = OpenGLRenderContext;

	// -------------------------------------------------------------------------
	// Create Windows
	// -------------------------------------------------------------------------

	application::get().parseWindow = std::make_shared<ParsingWindow>();
	application::get().exportWindow = std::make_shared<ExportWindow>();
	application::get().mainWindow = std::make_shared<MainWindow>();

	// -------------------------------------------------------------------------
	// Main-Loop
	// -------------------------------------------------------------------------

	MSG currentMessage = {};
	while (appRuntime.internal.runtimeFlag)
	{

		// ---------------------------------------------------------------------
		// Process Window Messages
		// ---------------------------------------------------------------------

		// Process incoming window messages.
		while (PeekMessageA(&currentMessage, NULL, 0, 0, PM_REMOVE))
		{

			LRESULT pStatus = ImGui_ImplWin32_WndProcHandler(windowHandle, currentMessage.message,
				currentMessage.wParam, currentMessage.lParam);
			if (pStatus == TRUE) break;

			// Exit the loop if WM_QUIT was received.
			if (currentMessage.message == WM_QUIT)
			{
				appRuntime.internal.runtimeFlag = false;
				break;
			}

			TranslateMessage(&currentMessage);
			DispatchMessageA(&currentMessage);
		}

		// ---------------------------------------------------------------------
		// Render the Frame
		// ---------------------------------------------------------------------

		RenderFrame(windowHandle, OpenGLDeviceContext, OpenGLRenderContext);

		// ---------------------------------------------------------------------
		// Remove Abandoned Windows
		// ---------------------------------------------------------------------

		// Find orphaned mapping windows.
		std::vector<GUIWindow*> mappingWindowOrphans = {};
		for (ui64 m_index = 0; m_index < application::get().mappingWindows.size(); ++m_index)
			if (!application::get().mappingWindows[m_index]->isOpen())
				mappingWindowOrphans.push_back(application::get().mappingWindows[m_index].get());

		// Delete the orphaned mapping windows.
		for (GUIWindow* orphan : mappingWindowOrphans)
		{
			for (ui64 i = 0; i < application::get().mappingWindows.size(); ++i)
			{
				if (application::get().mappingWindows[i].get() == orphan)
				{
					application::get().mappingWindows.erase(application::get().mappingWindows.begin() + i);
					break;
				}
			}
		}
		

	}

	// -------------------------------------------------------------------------
	// Clean-up & Exit Process
	// -------------------------------------------------------------------------

	return 0;
}


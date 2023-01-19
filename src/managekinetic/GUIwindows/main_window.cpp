#include <managekinetic/GUIwindows/main_window.h>
#include <managekinetic/GUIwindows/mapping_window.h>
#include <platform/fileio_win32.h>

void
performCloseFile()
{
	bool closeDocument = true;

	if (application::get().currentDocument != nullptr)
	{
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
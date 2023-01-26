#include <managekinetic/GUIwindows/mapping_window.h>

MappingWindow::
MappingWindow()
	: GUIWindow(true), previewTable("Preview Table")
{
	this->column_map_edit_index = -1; // A new map.
	map.column_index = -1;
	map.all_multivalues = false;
	map.multivalue_index = 1;
}

MappingWindow::
MappingWindow(column_map map, ui64 edit_index)
	: GUIWindow(true), previewTable("Preview Table")
{
	this->column_map_edit_index = edit_index; // A new map.
	this->map = map;
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
		if (this->column_map_edit_index == -1)
		{
			currentDocument.getTable().getMappedColumns().push_back(this->map);
		}
		else
		{
			currentDocument.getTable().getMappedColumns()[this->column_map_edit_index] = this->map;
		}
		currentDocument.unlockTable();
		stayOpen = false; // Saved, we can close this window.
	}
	if (saveDisabled == true) ImGui::EndDisabled();

	ImGui::End();

	return stayOpen;

}
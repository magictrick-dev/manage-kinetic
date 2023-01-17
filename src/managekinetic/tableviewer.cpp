#include <managekinetic/tableviewer.h>
#include <string>
#include <sstream>

tableviewer::
tableviewer(std::string tablename)
	: tableName(tablename)
{
	static ui32 instanceIdentifier = 1;
	this->instanceIdentifer = instanceIdentifer++;
}

tableviewer::
~tableviewer()
{

}

std::string tableviewer::
getUniqueElementName(std::string elementName)
{
	std::stringstream el = {};
	el << elementName << "##_tblevwr_" << this->instanceIdentifer;
	return el.str();
}

/**
 * Renders a table to the screen. The default behavior is to show the table data.
 * The table total refers to the maximum number of rows within the dataset such that
 * the loop knows when to reach an exit condition.
 */
void tableviewer::
render(ui64 tableTotal, ui64 columnTotal)
{

	/*
		total columns = 7

		cOffset = 0, step = 3, step_p_off = 3, total - step_p_off = 4
		4 > 0 OKAY
		[a][a][a][ ][ ][ ][ ]

		cOffset = 3, step = 3, step_p_off = 6, total - step_p_off = 1
		1 > 0 OKAY
		[ ][ ][ ][a][a][a][ ]
	
		cOffset = 6, step = 3, step_p_off = 9, total - step_p_off = -2
		-2 > 0 BAD
		[ ][ ][ ][ ][ ][ ][a] b  b

		min = step + t_s_p_o = 1
		[ ][ ][ ][ ][ ][ ][a]

	*/
	// -------------------------------------------------------------------------
	// Calculute the number of columns needed to be shown.
	// -------------------------------------------------------------------------

	i32 columnsShown = this->columnStep;
	i32 deltaCol = columnTotal - (cOffset + columnStep);
	if (deltaCol <= 0)
		columnsShown = this->columnStep + deltaCol;

	if (this->showRowNumbers == true)
		columnsShown += 1;

	ImGui::BeginGroup(); // controls group

	// -------------------------------------------------------------------------
	// Column Pagination
	// -------------------------------------------------------------------------

		if (ImGui::Button(this->getUniqueElementName("Reset##c_clear").c_str()))
			this->cOffset = 0;

		ImGui::SameLine();

		bool lcpDisable = false;
		if (this->cOffset - this->columnStep < 0)
		{
			ImGui::BeginDisabled();
			lcpDisable = true;
		}
		if (ImGui::ArrowButton(this->getUniqueElementName("col_left").c_str(), ImGuiDir_Left))
		{
			this->cOffset -= this->columnStep;
		}
		if (lcpDisable == true) ImGui::EndDisabled();

		ImGui::SameLine();

		bool rcpDisable = false;
		if (this->cOffset + this->columnStep >= columnTotal)
		{	
			ImGui::BeginDisabled();
			rcpDisable = true;
		}
		if (ImGui::ArrowButton(this->getUniqueElementName("col_right").c_str(), ImGuiDir_Right))
		{
			this->cOffset += this->columnStep;
		}
		if (rcpDisable == true) ImGui::EndDisabled();

		ImGui::SameLine();

		i32 dcolOffset = (this->showRowNumbers) ? 2 : 1;
		std::stringstream ccolss = {};
		ccolss << "F" << this->cOffset << " - F" << ((columnsShown <= 1) ? 0 : (this->cOffset + columnsShown - dcolOffset))
			<< " / " << ((columnTotal == 0) ? 0 : columnTotal - 1);
		ImGui::Text(ccolss.str().c_str());

	// -------------------------------------------------------------------------
	// Records Pagination
	// -------------------------------------------------------------------------

		if (ImGui::Button(this->getUniqueElementName("Reset##r_clear").c_str()))
			this->vOffset = 0;

		ImGui::SameLine();

		bool lrpDisable = false;
		if (this->vOffset - this->paginationStep < 0)
		{
			ImGui::BeginDisabled();
			lrpDisable = true;
		}
		if (ImGui::ArrowButton(this->getUniqueElementName("row_up").c_str(), ImGuiDir_Up))
		{
			this->vOffset -= this->paginationStep;
		}
		if (lrpDisable == true) ImGui::EndDisabled();

		ImGui::SameLine();

		bool rrpDisable = false;
		if (this->vOffset + this->paginationStep >= tableTotal)
		{	
			ImGui::BeginDisabled();
			rrpDisable = true;
		}
		if (ImGui::ArrowButton(this->getUniqueElementName("row_down").c_str(), ImGuiDir_Down))
		{
			this->vOffset += this->paginationStep;
		}
		if (rrpDisable == true) ImGui::EndDisabled();

		ImGui::SameLine();
		
		i32 totalPages = (i32)ceilf(((r32)tableTotal / (r32)this->paginationStep));
		i32 currentPage = (this->vOffset / paginationStep) + 1;
		std::stringstream pstrss = {};
		pstrss << currentPage << " / " << totalPages << " Pages";
		ImGui::Text(pstrss.str().c_str());

		if (ImGui::Button(this->getUniqueElementName("Reset All Offset Positions").c_str()))
		{
			this->cOffset = 0;
			this->vOffset = 0;
		}

	ImGui::EndGroup();

	ImGui::Spacing();

	// -------------------------------------------------------------------------
	// Table Setup & Loop
	// -------------------------------------------------------------------------

	if (ImGui::BeginTable(this->getUniqueElementName(this->tableName).c_str(), columnsShown,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX |
		ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
	{
		ui32 showCount = 0;
		this->headerProcedure(this->cOffset, columnsShown);
		for (ui64 idx = this->vOffset; idx < tableTotal; ++idx)
		{
			ImGui::TableNextRow();
			this->iterationProcedure(idx, this->cOffset, columnsShown);
			if (++showCount >= this->paginationStep) break;
		}

		ImGui::EndTable();
	}

#if 0
	// Show the table.
	if (ImGui::BeginTable(this->getUniqueElementName(this->tableName).c_str(), columnTotal,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
	{

		ui64 showCount = 0;
		this->headerProcedure(this->yOffset);
		for (ui64 idx = this->vOffset; idx < tableTotal; ++idx)
		{
			ImGui::TableNextRow();
			this->iterationProcedure(idx, columnTotal);
			if (showCount >= this->paginationStep) break;
		}

		ImGui::EndTable();

	}
#endif
}

void tableviewer::
iterationProcedure(ui64 index, ui32 columnOffset, ui32 colCount)
{
	// Get the current document and table.
	document& currentDocument = *application::get().currentDocument;
	tablemap& table = currentDocument.getTable();

	record& currentRecord = table[index];

	// Show the y-axis header as needed.
	if (this->showRowNumbers)
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
			ImGui::ColorConvertFloat4ToU32({.20f, .20f, .20f, 1.0f}));
		ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", index + 1);
	}

	bool isHovered = false;
	ui32 cc = (this->showRowNumbers) ? colCount - 1 : colCount;
	for (ui32 cIndex = 0; cIndex < cc; ++cIndex)
	{

		// Since we are iterating based on the number of columns, we need to
		// manually calculate the column.
		field* currentField = currentRecord[columnOffset + cIndex];

		if (ImGui::TableGetColumnFlags(cIndex) & ImGuiTableColumnFlags_IsHovered)
			isHovered = true;

		if (ImGui::TableSetColumnIndex(cIndex + 1)
			&& currentField != nullptr)
		{
			for (ui32 mvi = 0; mvi < currentField->valueCount(); ++mvi)
				ImGui::Text(currentField->get(mvi).c_str());
		}

	}

#if 0

	// Get the record we are showing.
	record& currentRecord = table[index];

	// Show the y-axis header.
	ImGui::TableSetColumnIndex(0);
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
		ImGui::ColorConvertFloat4ToU32({.25f, .25f, .25f, 1.0f}));
	ImGui::TextColored({.8f, .8f, .8f, 1.0f}, "Row #%d", index + 1);

	ui32 columnsShown = 1;
	for (ui32 colIndex = columnOffset; colIndex < table.columnsCount(); ++colIndex)
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

		if (columnsShown >= this->columnStep - 1) break;
	}
#endif
}


void tableviewer::
headerProcedure(ui32 columnOffset, ui32 colCount)
{

	// Get the current document and table.
	document& currentDocument = *application::get().currentDocument;
	tablemap& table = currentDocument.getTable();

	if (this->isRowNumbersShown())
		ImGui::TableSetupColumn("");

	std::vector<std::string>& headings = table.getColumnNames();
	ui32 columnsShown = 0;

	for (ui32 cIndex = columnOffset; cIndex < headings.size(); ++cIndex)
	{
		if (++columnsShown >= colCount - 1) break;
		ImGui::TableSetupColumn(headings[cIndex].c_str());
	}

	ImGui::TableHeadersRow();

#if 0

	ImGui::TableSetupColumn(""); // Top left, 0,0, null cell.

	std::vector<std::string>& headings = table.getColumnNames();

	ui32 columnsShown = 1; // We used one column to show row numbers.
	for (ui32 cIndex = columnOffset; cIndex < headings.size(); ++cIndex)
	{
		ImGui::TableSetupColumn(headings[cIndex].c_str());
		if (columnsShown >= this->getColumnStep() - 1) break;
	}

	ImGui::TableHeadersRow();
#endif



}

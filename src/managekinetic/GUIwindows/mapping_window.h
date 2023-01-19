#ifndef MANAGEKINETIC_GUIWIN_MAPPINGWINDOW_H
#define MANAGEKINETIC_GUIWIN_MAPPINGWINDOW_H
#include <managekinetic/primitives.h>
#include <managekinetic/guiwindow.h>
#include <managekinetic/mappingtable.h>
#include <managekinetic/tableviewer.h>

#include <vendor/DearImGUI/imgui.h>
#include <vendor/DearImGUI/misc/cpp/imgui_stdlib.h>

#include <sstream>
#include <string>

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


		}
};

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

#endif
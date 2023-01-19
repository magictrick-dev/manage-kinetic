#ifndef MANAGEKINETIC_GUIWINDOW_MAINWINDOW_H
#define MANAGEKINETIC_GUIWINDOW_MAINWINDOW_H
#include <managekinetic/guiwindow.h>
#include <managekinetic/tableviewer.h>

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
		}
};

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

#endif
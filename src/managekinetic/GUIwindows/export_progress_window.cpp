#include <managekinetic/GUIwindows/export_progress_window.h>
#include <managekinetic/application.h>

#include <string>
#include <sstream>

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

		ImGui::Text("Exporting to excel, please wait.");
		ImGui::Spacing();
		ImGui::Separator();

		ui64 recordsProcessed = 0;
		ui64 recordsTotal = 0;

		std::vector<exportprogress> prog = currentDocument.getExportProgress();
		for (ui64 i = 0; i < prog.size(); ++i)
		{
			recordsProcessed += prog[i].recordsStored;
			recordsTotal += prog[i].recordsTotal;
		}

		r32 delta = (r32)((r32)recordsProcessed / (r32)recordsTotal);

		std::stringstream exportString = {};
		exportString << recordsProcessed << " / " << recordsTotal << " records processed.";
		
		std::stringstream percentage = {};
		percentage << std::fixed << std::setprecision(0) << delta * 100.0f << "%";
		std::string visual = (delta < .99) ? percentage.str() : "Done!";

		ImGui::ProgressBar(delta, {}, visual.c_str());

		ImGui::Spacing();
		ImGui::Text("%s", exportString.str().c_str());

		ImGui::Spacing();
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
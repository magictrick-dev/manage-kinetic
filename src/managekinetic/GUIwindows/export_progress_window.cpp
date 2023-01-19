#include <managekinetic/GUIwindows/export_progress_window.h>
#include <managekinetic/application.h>

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
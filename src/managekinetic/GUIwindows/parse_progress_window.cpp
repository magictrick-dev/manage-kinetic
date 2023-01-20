#include <managekinetic/GUIwindows/parse_progress_window.h>
#include <managekinetic/application.h>

#include <string>
#include <sstream>

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
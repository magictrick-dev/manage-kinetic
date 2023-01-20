#ifndef MANAGEKINETIC_GUIWIN_EXPORTPROGRESSWINDOW_H
#define MANAGEKINETIC_GUIWIN_EXPORTPROGRESSWINDOW_H
#include <managekinetic/guiwindow.h>

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

#endif
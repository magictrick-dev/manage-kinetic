#ifndef MANAGEKINETIC_GUIWIN_PARSEPROGRESSWINDOW_H
#define MANAGEKINETIC_GUIWIN_PARSEPROGRESSWINDOW_H
#include <managekinetic/guiwindow.h>

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

#endif
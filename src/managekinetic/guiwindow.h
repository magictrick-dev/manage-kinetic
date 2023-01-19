#ifndef MANAGEKINETIC_GUIWINDOW_H
#define MANAGEKINETIC_GUIWINDOW_H

#include <managekinetic/primitives.h>
#include <vendor/DearImGUI/imgui.h>

#include <thread>
#include <mutex>
#include <vector>

class GUIWindow
{
	public:
		GUIWindow();
		GUIWindow(bool startOpened);
		virtual ~GUIWindow();

		bool isOpen() 	const { return this->openFlag; }
		ui32 getID() 	const { return this->identifier; }

		void open();
		void close();
		void display();

	public:
		static void DisplayWindow();

	protected:
		virtual bool onDisplay() 	= 0;
		virtual bool onClose() 		= 0;
		virtual bool onOpen() 		= 0;
		virtual void onExit(bool destroyed);

	private:
		ui32 identifier;
		std::atomic<bool> openFlag = false;
};

#endif
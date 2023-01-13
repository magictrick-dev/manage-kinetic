#include <managekinetic/uicomponent.h>

GUIWindow::
GUIWindow()
{
	this->openFlag = false;
	static ui32 identifierStack = 1;
	this->identifier = identifierStack++;
}

GUIWindow::
GUIWindow(bool startOpened)
{
	this->openFlag = startOpened;
	static ui32 identifierStack = 1;
	this->identifier = identifierStack++;
}

GUIWindow::
~GUIWindow()
{
	this->onExit(true);
}

/**
 * The onExit is an optional cleanup function should a window forceably be closed.
 * This is always invoked, either by the destructor or when the window is closed.
 * The onExit can differentiate between the "close" behavior and the destructor using
 * the "destroyed" parameter.
 */
void GUIWindow::
onExit(bool destroyed = false)
{ }

/**
 * Close will set the open flag to false, barring the display onDisplay function
 * from being invoked. Close will invoke the onClose() event. If the onClose emits
 * true, the window will be closed.
 */
void GUIWindow::
close()
{

	if (this->openFlag != false
		&& this->onClose() == true)
	{
		this->openFlag = false;
		this->onExit();
	}

}

/**
 * Open will set the open flag to true, allowing the display onDisplay function
 * being able to run. Open will invoke the onOpen() event. If the onOpen emits
 * false, the window will not be opened and the open flag will remain false.
 */
void GUIWindow::
open()
{
	if (this->openFlag != true
		&& this->onOpen() == true)
	{
		this->openFlag = true;
	}
}

/**
 * The display function should be invoked every frame, regardless if the window
 * should be shown or not. The window will ensure that the window should be displayed
 * and then invokes the onDisplay function which should contain the necessary display
 * routine behaviors.
 * 
 * The onDisplay event should always return true if the window should remain open,
 * or it could return false if it should close. In this event, the window will invoke
 * close immediately.
 */
void GUIWindow::
display()
{

	if (this->openFlag == true)
	{
		if (this->onDisplay() == false)
			this->close();
	}

}

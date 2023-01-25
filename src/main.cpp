/**
 * -----------------------------------------------------------------------------
 * Managekinetic
 * 		A unidata table processing tool which formats multi-value fields and
 * 		exports the data to Excel. Uses multi-threading to enhance performance
 * 		and allows for concurrent processing of records.
 * 
 * By Chris DeJong, 12/28/2022
 * -----------------------------------------------------------------------------
 * 
 * 
 * 
 * 
 * -----------------------------------------------------------------------------
 * Todos   &   Bugs
 * -----------------------------------------------------------------------------
 * 		//* Implement saving/loading maps. Serialize the structure.
 * 		* Implement editor of current maps.
 * 		* Prevent mapping of duplicated names.
 * 		
 * 		* MAYBE FIXED? Fix race condition bug on columns being appended. (Happens rarely)
 * 
 * 		* Optionally allow for exclusion rules to exclude records from export.
 * 		* Optionally implement procedures to remap field data during export.
 * -----------------------------------------------------------------------------
 */

// Bring in platform system headers.
#include <windows.h>	// System Definitions
#include <GL/GL.h>		// OpenGL
#include <io.h>			// For console
#include <fcntl.h>		// For console

// Bring in standard library headers as needed.
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>
#include <cassert>

// Bring in the necessary runtime headers.
#include <main.h>
#include <managekinetic/tableviewer.h>

// Bring in the windows.
#include <managekinetic/GUIwindows/main_window.h>
#include <managekinetic/GUIwindows/mapping_window.h>
#include <managekinetic/GUIwindows/export_progress_window.h>
#include <managekinetic/GUIwindows/parse_progress_window.h>

// Bring in the platform functions.
#include <platform/benchmark_win32.h>
#include <platform/memory_win32.h>
#include <platform/fileio_win32.h>

// Bring in required vendor library headers.
#include <vendor/OpenXLSX/OpenXLSX.hpp>
#include <vendor/DearImGUI/imgui.h>
#include <vendor/DearImGUI/misc/cpp/imgui_stdlib.h>
#include <vendor/DearImGUI/backends/imgui_impl_win32.h>
#include <vendor/DearImGUI/backends/imgui_impl_opengl3.h>

// ---------------------------------------------------------------------------------------------------------------------
// Load External ImGUI Functions
// ---------------------------------------------------------------------------------------------------------------------

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void HelpMarker(const char* desc);

// ---------------------------------------------------------------------------------------------------------------------
// Windows Main
// ---------------------------------------------------------------------------------------------------------------------

void
RenderFrame(HWND windowHandle, HDC OpenGLDeviceContext, HGLRC OpenGLRenderContext, bool disableAll = false)
{

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// -------------------------------------------------------------------------
	// Main Window Render Area
	// -------------------------------------------------------------------------

	if (disableAll == true) ImGui::BeginDisabled();

	application::get().mainWindow->display();

	if (disableAll == true) ImGui::EndDisabled();

	// -------------------------------------------------------------------------
	// Client Interface Windows
	// -------------------------------------------------------------------------

	if (disableAll == true) ImGui::BeginDisabled();
	
	// Show the parsing window.
	application::get().parseWindow->display();

	// Show the export window.
	application::get().exportWindow->display();

	// Display all the mapping windows currently open.
	for (std::unique_ptr<GUIWindow>& currentMappingWindow : application::get().mappingWindows)
		currentMappingWindow->display();
	
	if (disableAll == true) ImGui::EndDisabled();


	// ---------------------------------------------------------------------
	// ImGUI Demo Window
	// ---------------------------------------------------------------------

#define MANAGEKINETIC_DEBUG
#if defined(MANAGEKINETIC_DEBUG)
	if (disableAll == true) ImGui::BeginDisabled();
	// Show the demo window.
	static bool showDemoWindowFlag = false;
	if (showDemoWindowFlag)
		ImGui::ShowDemoWindow(&showDemoWindowFlag);
	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_B))
		showDemoWindowFlag = showDemoWindowFlag ? false : true;
	if (disableAll == true) ImGui::EndDisabled();
#endif

	// ---------------------------------------------------------------------
	// Post the Frame to the GPU
	// ---------------------------------------------------------------------

	// Render ImGui components.
	ImGui::Render();

	// Retrieve the current size of the window.
	RECT windowClientSize = {};
	GetClientRect(windowHandle, &windowClientSize);
	i32 windowWidth = windowClientSize.right - windowClientSize.left;
	i32 windowHeight = windowClientSize.top - windowClientSize.bottom;

	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(.2f, .2f, .2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	wglMakeCurrent(OpenGLDeviceContext, OpenGLRenderContext);
	SwapBuffers(OpenGLDeviceContext);

}

LRESULT CALLBACK
Win32WindowProcedure(HWND windowHandle, UINT message, WPARAM wideParam, LPARAM lowParam)
{

	switch (message)
	{
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_SIZING:
		{
			// We need to render a new frame since the window is resizing.
			// We disable all elements for interaction.
			RenderFrame(windowHandle, GetDC(application::get().internal.windowHandle),
				application::get().internal.OpenGLRenderContext, true);
			return 0;
		}
		case WM_SETCURSOR:
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			break;
		}
		case WM_PAINT:
		{
			break;
		}
	}
	// For the messages we don't collect.
	return DefWindowProc(windowHandle, message, wideParam, lowParam);
}

i32 WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR commandline, i32 commandShow)
{

	// -------------------------------------------------------------------------
	// Allocate a Console for Output
	// -------------------------------------------------------------------------
	
	// Console allocation is only useful for debugging and otherwise removed when
	// building for release.
#if defined(BTYPEDEBUG)
	FreeConsole();
	if (AllocConsole())
	{
		SetConsoleTitleA("Manage Kinetic Status Console");
		typedef struct { char* _ptr; int _cnt; char* _base; int _flag; int _file; int _charbuf; int _bufsiz; char* _tmpfname; } FILE_COMPLETE;
#pragma warning(push)
#pragma warning(disable: 4311)
		*(FILE_COMPLETE*)stdout = *(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT), "w");
		*(FILE_COMPLETE*)stderr = *(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT), "w");
		*(FILE_COMPLETE*)stdin = *(FILE_COMPLETE*)_fdopen(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT), "r");
#pragma warning(pop)
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
		setvbuf(stdin, NULL, _IONBF, 0);
		printf("[ Main Thread ] : Console has been initialized.\n");
	}
	else
	{
		// Ensure that we are running a console at startup.
		return -1;
	}
#endif

	// -------------------------------------------------------------------------
	// Initialize the Win32 Window
	// -------------------------------------------------------------------------

	WNDCLASSA windowClass = {};
	windowClass.lpfnWndProc = Win32WindowProcedure;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "ManageKinetic";
	windowClass.hbrBackground = NULL;

	RegisterClassA(&windowClass);

	HWND windowHandle = CreateWindowExA(NULL, "ManageKinetic", "Manage Kinetic Conversion Utility",
		WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
		hInstance, NULL);

	if (windowHandle == NULL)
	{
		printf("[ Main Thread ] : Unable to create the window.\n");
		printf("[ Main Thread ] : ");
		system("pause");
		return -1;
	}

	// -------------------------------------------------------------------------
	// Initialize the OpenGL Render Context
	// -------------------------------------------------------------------------

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	HDC OpenGLDeviceContext = GetDC(windowHandle);

	int pixelFormal = ChoosePixelFormat(OpenGLDeviceContext, &pfd); 
	SetPixelFormat(OpenGLDeviceContext, pixelFormal, &pfd);
	HGLRC OpenGLRenderContext = wglCreateContext(OpenGLDeviceContext);
	wglMakeCurrent(OpenGLDeviceContext, OpenGLRenderContext);

	((BOOL(WINAPI*)(int))wglGetProcAddress("wglSwapIntervalEXT"))(1);

	// -------------------------------------------------------------------------
	// Initialize ImGUI Interface
	// -------------------------------------------------------------------------

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsRCV();

	// Setup Platform/Renderer backends
	if (!ImGui_ImplWin32_Init(windowHandle))
	{
		printf("[ Main Thread ] : Win32 ImGUI initialization failed.\n");
		printf("[ Main Thread ] : ");
		system("pause");
		return -1;
	}

	if (!ImGui_ImplOpenGL3_Init("#version 410 core"))
	{
		printf("[ Main Thread ] : OpenGL3 ImGUI initialization failed.\n");
		printf("[ Main Thread ] : ");
		system("pause");
		return -1;
	}

	// -------------------------------------------------------------------------
	// Initialize Application
	// -------------------------------------------------------------------------

	application& appRuntime 	= application::get();
	appRuntime.internal.runtimeFlag 	= true;
	appRuntime.internal.windowHandle 	= windowHandle;
	appRuntime.internal.OpenGLRenderContext = OpenGLRenderContext;

	// -------------------------------------------------------------------------
	// Create Windows
	// -------------------------------------------------------------------------

	application::get().parseWindow = std::make_shared<ParsingWindow>();
	application::get().exportWindow = std::make_shared<ExportWindow>();
	application::get().mainWindow = std::make_shared<MainWindow>();

	// -------------------------------------------------------------------------
	// Main-Loop
	// -------------------------------------------------------------------------

	ShowWindow(windowHandle, commandShow); // Show once init is complete.

	MSG currentMessage = {};
	while (appRuntime.internal.runtimeFlag)
	{

		// ---------------------------------------------------------------------
		// Process Window Messages
		// ---------------------------------------------------------------------

		// Process incoming window messages.
		while (PeekMessageA(&currentMessage, NULL, 0, 0, PM_REMOVE))
		{

			LRESULT pStatus = ImGui_ImplWin32_WndProcHandler(windowHandle, currentMessage.message,
				currentMessage.wParam, currentMessage.lParam);
			if (pStatus == TRUE) break;

			// Exit the loop if WM_QUIT was received.
			if (currentMessage.message == WM_QUIT)
			{
				appRuntime.internal.runtimeFlag = false;
				break;
			}

			TranslateMessage(&currentMessage);
			DispatchMessageA(&currentMessage);
		}

		// ---------------------------------------------------------------------
		// Render the Frame
		// ---------------------------------------------------------------------

		RenderFrame(windowHandle, OpenGLDeviceContext, OpenGLRenderContext);

		// ---------------------------------------------------------------------
		// Remove Abandoned Windows
		// ---------------------------------------------------------------------

		// Find orphaned mapping windows.
		std::vector<GUIWindow*> mappingWindowOrphans = {};
		for (ui64 m_index = 0; m_index < application::get().mappingWindows.size(); ++m_index)
			if (!application::get().mappingWindows[m_index]->isOpen())
				mappingWindowOrphans.push_back(application::get().mappingWindows[m_index].get());

		// Delete the orphaned mapping windows.
		for (GUIWindow* orphan : mappingWindowOrphans)
		{
			for (ui64 i = 0; i < application::get().mappingWindows.size(); ++i)
			{
				if (application::get().mappingWindows[i].get() == orphan)
				{
					application::get().mappingWindows.erase(application::get().mappingWindows.begin() + i);
					break;
				}
			}
		}
		

	}

	// -------------------------------------------------------------------------
	// Clean-up & Exit Process
	// -------------------------------------------------------------------------

	return 0;
}


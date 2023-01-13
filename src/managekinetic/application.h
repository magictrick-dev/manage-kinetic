#ifndef MANAGE_KINETIC_APPLICATION_H
#define MANAGE_KINETIC_APPLICATION_H
#include <memory>
#include <string>
#include <filesystem>

#include <windows.h>
#include <managekinetic/primitives.h>
#include <managekinetic/mappingtable.h>
#include <managekinetic/uicomponent.h>

// Defines all the work that needs to be done.
typedef std::vector<std::string> UnprocessedTable;
typedef std::vector<UnprocessedTable> TableWorkQueue;

struct parseprogress
{
	ui32 recordsProcessed;
	ui32 recordsTotal;
};

class document
{
	public:
		document(std::filesystem::path path, ui32 maxRecordsPerDocument);
		~document();

		std::string 	getFilename() const;

		inline bool isInitializing() const { return this->initializingFlag; };
		inline bool isParsing() const { return this->parsingFlag; }
		inline bool isReady() const { return this->readyFlag; }

		std::vector<parseprogress> getParsingProgress();

		//MappingTable* operator[](ui64 index) { return this->mappingTables[index]; }
		//ui64 mappingTableCount() const { return this->mappingTables.size(); }

		tablemap& 	getTable() { return this->table; };
		void 		lockTable() { this->tableLock.lock(); }
		void 		unlockTable() { this->tableLock.unlock(); }


	public:
		i32 currentPreviewPage = 0;

	protected:
		std::filesystem::path filepath;
	private:
		std::thread documentLoaderThread;

		void parse();
		void parseSubroutine(ui32 workerIdentifier);
		void parseSubroutineCreateMappingTable(ui32, parseprogress*,
			UnprocessedTable*);

		std::atomic<bool> initializingFlag;
		std::atomic<bool> parsingFlag;
		std::atomic<bool> readyFlag;

		std::mutex parsingProgressLock;
		std::vector<parseprogress> parsingProgress;

		std::mutex workQueueLock;
		TableWorkQueue* workQueue 	= nullptr;
		ui32 workQueueIndex 		= 0;

	private:
		std::mutex tableLock;
		tablemap table;

};

/**
 * Serves as the backbone for the application runtime. The application itself is
 * a lazy-loading singleton which is designed to be fetchable wherever it may need.
 */
class application
{
	public:
		static inline application& get()
			{
				static application* _instance = nullptr;
				if (_instance == nullptr)
				{
					_instance = new application;
				}
				return *_instance;
			}

	public:

		// Internal Runtime State
		struct
		{
			HWND 				windowHandle;
			HGLRC				OpenGLRenderContext;
			b32 				runtimeFlag;
		} internal;

		// Current loaded document.
		document* 	currentDocument = nullptr;

		// Windows
		std::shared_ptr<GUIWindow> mainWindow;
		std::shared_ptr<GUIWindow> parseWindow;
		std::vector<std::unique_ptr<GUIWindow>> mappingWindows;

		// Application runtime parameters.
		std::mutex	parmetersLock;
		i32		parseingThreads 	= 4;
		i32		recordsPerTable 	= 6000;
		char 	fielddelim[2] 		= ";";
		i32 	multivalue 			= 253;

		std::string currentVersion = "Version 0.3.4i";

};

#endif
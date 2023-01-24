#ifndef MANAGE_KINETIC_FILEIO_WIN32_H
#define MANAGE_KINETIC_FILEIO_WIN32_H
#include <managekinetic/primitives.h>
#include <string>

b32 			sourceFileExists(std::string filepath);
ui64 			sourceFileSize(std::string filepath);
ui64 			sourceFileOpen(void* buffer, ui64 buffersize, std::string filepath, ui64 fileSize);
ui64			sourceFileSave(void* buffer, ui64 buffersize, std::string filepath);
std::string 	sourceOpenFileDialogue(void* windowHandle);
std::string     sourceSaveFileDialogue(void* windowHandle, const char* defaultExt, const char* filterPair);
#endif
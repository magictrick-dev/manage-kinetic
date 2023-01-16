#ifndef MANAGE_KINETIC_FILEIO_WIN32_H
#define MANAGE_KINETIC_FILEIO_WIN32_H
#include <managekinetic/primitives.h>
#include <string>

b32 			sourceFileExists(std::string filepath);
ui64 			sourceFileSize(std::string filepath);
ui64 			sourceFileOpen(void* buffer, ui64 buffersize, std::string filepath, ui64 fileSize);
std::string 	sourceOpenFileDialogue(void* windowHandle);

#endif
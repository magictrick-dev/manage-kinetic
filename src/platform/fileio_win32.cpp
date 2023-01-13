#include <windows.h>
#include <platform/fileio_win32.h>

/**
 * Opens the Win32 file dialogue.
 */
std::string
sourceOpenFileDialogue(void* windowHandle)
{
	// Zero the buffer.
	char filepathBuffer[260];
	memset(filepathBuffer, 0x00, 260);

	// Initialize the structure.
	OPENFILENAMEA fileOpenResults 	= {};
	fileOpenResults.lStructSize 	= sizeof(OPENFILENAMEA);
	fileOpenResults.hwndOwner 		= *(HWND*)windowHandle;
	fileOpenResults.lpstrFile 		= filepathBuffer;
	fileOpenResults.nMaxFile 		= 260;
	fileOpenResults.Flags 			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Perform the action.
	if (GetOpenFileNameA(&fileOpenResults))
		return std::string(fileOpenResults.lpstrFile);
	else
		return "";
}

/**
 * Determines if a file exists.
 * 
 * @param filepath The path to the file.
 * 
 * @returns True if the file exists, false otherwise.
 */
b32
sourceFileExists(std::string filepath)
{
	DWORD dwAttrib = GetFileAttributesA(filepath.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

/**
 * Retrieves the size of a file, in bytes. If the file does not exist, the filesize
 * will be zero. However, this shouldn't be used to determine if a file doesn't exist
 * since it is possible that a file with the size of zero is possible.
 * 
 * @param filepath The path to the file.
 * 
 * @returns The filesize, in bytes.
 */
ui64
sourceFileSize(std::string filepath)
{

	// Attempt to open the file. If it can't be opened, return zero.
	HANDLE fileHandle = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) return 0;

	// Once the file is open, retrieve the size.
	LARGE_INTEGER filesize = {};
	BOOL filesizeStatus = GetFileSizeEx(fileHandle, &filesize);

	// Close and return the size.
	CloseHandle(fileHandle);
	return (ui64)filesize.QuadPart;

}

/**
 * Opens a file, then dumps the contents of the file into the buffer.
 * 
 * @param buffer The buffer to put the file into.
 * @param buffersize The size of the buffer.
 * @param filepath The path to the file to open.
 * 
 * @returns The number of bytes that were placed into the buffer. If the
 * number of bytes that were placed into the buffer equal to the filesize,
 * then the file was loaded entirely into memory.
 */
ui64
sourceFileOpen(void* buffer, ui64 buffersize, std::string filepath, ui64 fileSize)
{

	HANDLE fileHandle = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) return 0;
	
	// Since ReadFile doesn't guarantee we read the entire file in one gulp, we
	// should design it such that we keep reading until EOF.
	LPVOID bufferOffset = (LPVOID)buffer;
	ui64 totalBytesRead = 0;
	DWORD bytesIntoBuffer = 0;
	while (totalBytesRead < fileSize &&
		ReadFile(fileHandle, bufferOffset, fileSize-totalBytesRead, &bytesIntoBuffer, NULL))
	{
		// If we read zero bytes, there is nothing more to read into the buffer.
		if (bytesIntoBuffer == 0) break;

		// Otherwise we should bump the buffer offset and increase total bytes read.
		totalBytesRead += bytesIntoBuffer;
		bufferOffset = (ui8*)bufferOffset + bytesIntoBuffer;
	}

	CloseHandle(fileHandle);
	return totalBytesRead;
}
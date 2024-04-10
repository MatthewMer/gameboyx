#pragma once

#ifdef _WIN32
#include "windows.h"
#include "WinBase.h"
#endif

class FileMapper {
public:
	FileMapper(const char* _path, const size_t& _size);
	~FileMapper();

	LPVOID GetMappedFile();

private:
	HANDLE hFile;
	HANDLE hMapping;
	LPVOID lpMapAddress;
};
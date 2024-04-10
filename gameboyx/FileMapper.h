#pragma once

#ifdef _WIN32
#include "windows.h"
#include "WinBase.h"
#else
#endif

class FileMapper {
public:
	FileMapper(const char* _path, const size_t& _size);
	~FileMapper();

#ifdef _WIN32
	LPVOID
#else
#endif
	GetMappedFile();

private:
#ifdef _WIN32
	HANDLE hFile;
	HANDLE hMapping;
	LPVOID lpMapAddress;
#else
#endif
};
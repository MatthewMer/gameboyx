#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "WinBase.h"
#else
#endif

class FileMapper {
public:
	FileMapper(const char* _path, const size_t& _size);
	FileMapper() {};
	~FileMapper();

#ifdef _WIN32
	LPVOID GetMappedFile();
	LPVOID GetMappedFile(const char* _path, const size_t& _size);
#else
#endif

private:
#ifdef _WIN32
	HANDLE hFile = nullptr;
	HANDLE hMapping = nullptr;
	LPVOID lpMapAddress = nullptr;

	LPVOID MapFile(const char* _path, const size_t& _size);
#else
#endif

	void UnmapFile();
};
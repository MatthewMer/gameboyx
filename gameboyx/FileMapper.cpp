#include "FileMapper.h"

#include <filesystem>
#include <fstream>
#include <vector>

#include "logger.h"

using namespace std;
namespace fs = filesystem;

FileMapper::FileMapper(const char* _path, const size_t& _size) {
#ifdef _WIN32
	if (!fs::exists(_path)) {
		std::vector<char> zero(_size, 0x00);
		std::ofstream os(_path, std::ios::binary | std::ios::out);
		if (!os.write(zero.data(), zero.size())) {
			LOG_ERROR("[FS] create file");
		}
	}

	hFile = CreateFileA(
		_path,											// file name
		GENERIC_READ | GENERIC_WRITE,					// access type
		0,												// other processes can't share
		NULL,											// security
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		LOG_ERROR("[FS] create file handle "), std::format("{:d}", GetLastError());

	hMapping = CreateFileMapping(
		hFile,											// file handle
		NULL,
		PAGE_READWRITE,
		0,												// entire file at once (not single pages)
		0,												// ...
		nullptr);										// don't share
	if (hMapping == NULL)
		LOG_ERROR("[FS] create file mapping "), std::format("{:d}", GetLastError());

	lpMapAddress = MapViewOfFile(
		hMapping,
		FILE_MAP_ALL_ACCESS,
		NULL,
		NULL,
		NULL
	);

	if (lpMapAddress == NULL) {
		LOG_ERROR("[FS] create map view "), std::format("{:d}", GetLastError());
	}
#else
#endif
}

FileMapper::~FileMapper() {
#ifdef _WIN32
	UnmapViewOfFile(lpMapAddress);
	CloseHandle(hMapping);
	CloseHandle(hFile);
#else
#endif
}

#ifdef _WIN32
LPVOID
#else
#endif
FileMapper::GetMappedFile() {
	if (lpMapAddress != NULL) {
		return lpMapAddress;
	} else {
		LOG_ERROR("[FS] map view is null");
		return nullptr;
	}
}
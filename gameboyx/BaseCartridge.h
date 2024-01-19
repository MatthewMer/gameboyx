#pragma once

#include <string>
#include <memory>

#include "general_config.h"
#include "defs.h"
#include "helper_functions.h"
#include "logger.h"

enum info_types {
	NONE_INFO_TYPE,
	TITLE,
	CONSOLE,
	GAME_VER,
	FILE_NAME,
	FILE_PATH
};

class BaseCartridge {
public:
	static bool check_ext(const std::string& _file_path);
	static BaseCartridge* new_game(const std::string& _file_path);
	static BaseCartridge* existing_game(const std::string& _title, const std::string& _file_name, const std::string& _file_path, const console_ids& _id, const std::string& _version);

	bool CopyToRomFolder();
	virtual bool ReadRom() = 0;
	void ClearRom();

	std::vector<u8>& GetRomVector();

	std::string title;
	console_ids console;
	std::string version;
	std::string fileName;
	std::string filePath;

	// clone/assign protection
	BaseCartridge(BaseCartridge const&) = delete;
	BaseCartridge(BaseCartridge&&) = delete;
	BaseCartridge& operator=(BaseCartridge const&) = delete;
	BaseCartridge& operator=(BaseCartridge&&) = delete;

	~BaseCartridge() = default;

protected:
	std::vector<u8> vecRom;

	// constructor
	explicit BaseCartridge(const console_ids& _id, const std::string& _file) : console(_id) {
		std::string file;

		auto file_split = split_string(_file, "\\");
		for (int i = 0; i < file_split.size() - 1; i++) {
			file += file_split[i] + "/";
		}
		file += file_split.back();

		file_split = split_string(file, "/");
		file = "";
		for (int i = 0; i < file_split.size() - 1; i++) {
			file += file_split[i] + "/";
		}

		filePath = file;
		fileName = file_split.back();
	}

private:
	
};
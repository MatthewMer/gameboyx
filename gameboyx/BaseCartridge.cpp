#include "BaseCartridge.h"
#include "GameboyCartridge.h"

#include <iostream>
#include <string>
#include <fstream>

#include "logger.h"
#include "data_io.h"
#include "general_config.h"
#include "helper_functions.h"
#include "VHardwareTypes.h"

using namespace std;

namespace Emulation {
	bool BaseCartridge::CopyToRomFolder() {
		string rom_path = Config::ROM_FOLDER + fileName;

		if (Backend::FileIO::check_file_exists(rom_path)) {
			LOG_INFO("[emu] file already present in ", Config::ROM_FOLDER);
			LOG_INFO("[emu] fallback to given path: ", filePath);
			return false;
		}

		LOG_INFO("[emu] Copying file to ", Config::ROM_FOLDER);

		ofstream os(rom_path, ios::binary | ios::beg);
		if (!os) { return false; }
		copy(vecRom.begin(), vecRom.end(), ostream_iterator<u8>(os));
		os.close();

		ifstream is(rom_path, ios::binary | ios::beg);
		if (!is) { return false; }
		vector<u8> read_buffer((istreambuf_iterator<char>(is)), istreambuf_iterator<char>());

		if (!equal(vecRom.begin(), vecRom.end(), read_buffer.begin()) || vecRom.size() != read_buffer.size()) { return false; }

		filePath = Config::ROM_FOLDER;
		return true;
	}

	std::vector<u8>& BaseCartridge::GetRomVector() {
		return vecRom;
	}

	void BaseCartridge::ClearRom() {
		vecRom.clear();
	}

	BaseCartridge* BaseCartridge::new_game(const std::string& _file_path) {
		if (!check_ext(_file_path)) {
			LOG_ERROR("game file not recognized");
			return nullptr;
		}

		string ext = Helpers::split_string(_file_path, ".").back();
		console_ids id = CONSOLE_NONE;
		for (const auto& [key, value] : FILE_EXTS) {
			if (ext.compare(value.second) == 0) {
				id = key;
			}
		}

		return new Gameboy::GameboyCartridge(id, _file_path);
	}

	BaseCartridge* BaseCartridge::existing_game(const std::string& _title, const std::string& _file_name, const std::string& _file_path, const console_ids& _id, const std::string& _version) {
		BaseCartridge* cartridge;

		switch (_id) {
		case GB:
		case GBC:
			cartridge = new Gameboy::GameboyCartridge(_id, _file_path + _file_name);
			cartridge->title = _title;
			cartridge->version = _version;
			break;
		default:
			return nullptr;
			break;
		}

		return cartridge;
	}

	bool BaseCartridge::check_ext(const std::string& _file_path) {
		string ext = Helpers::split_string(_file_path, ".").back();

		for (const auto& [key, value] : FILE_EXTS) {
			if (value.second.compare(ext) == 0) {
				return true;
			}
		}

		return false;
	}
}
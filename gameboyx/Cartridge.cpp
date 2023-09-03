#include "Cartridge.h"

#include "imguigameboyx_config.h"
#include "gameboy_config.h"
#include "logger.h"
#include "helper_functions.h"

#include <fstream>

using namespace std;

bool Cartridge::ReadData() {
	if (!read_rom_to_buffer(*this->gameCtx, this->vecRom)) return false;
	if(!read_header_info(*this->gameCtx, this->vecRom)) return false;

	// TODO: load cartridge data for execution

	return true;
}

bool Cartridge::read_header_info(game_info& _game_ctx, vector<u8>& _vec_rom) {
	// read header info -----
	LOG_INFO("Reading header info");

	// cgb/sgb flags
	_game_ctx.is_cgb = _vec_rom[ROM_HEAD_CGBFLAG] == 0x80 || _vec_rom[ROM_HEAD_CGBFLAG] == 0xC0 ;
	_game_ctx.is_sgb = _vec_rom[ROM_HEAD_SGBFLAG] == 0x03;

	// title
	string title = "";
	int title_size_max = (_game_ctx.is_cgb ? ROM_HEAD_TITLE_SIZE_CGB : ROM_HEAD_TITLE_SIZE_GB);
	for (int i = 0; _vec_rom[ROM_HEAD_TITLE + i] != 0x00 && i < title_size_max; i++) {
		title += (char)_vec_rom[ROM_HEAD_TITLE + i];
	}
	_game_ctx.title = title;

	// licensee
	string licensee_code("");
	licensee_code += (char)_vec_rom[ROM_HEAD_NEWLIC];
	licensee_code += (char)_vec_rom[ROM_HEAD_NEWLIC + 1];

	_game_ctx.licensee = get_licensee(_vec_rom[ROM_HEAD_OLDLIC], licensee_code);

	// cart type
	_game_ctx.cart_type = get_cart_type(_vec_rom[ROM_HEAD_HW_TYPE]);

	// checksum
	u8 chksum_expected = _vec_rom[ROM_HEAD_CHKSUM];
	u8 chksum_calulated = 0;
	for (int i = ROM_HEAD_TITLE; i < ROM_HEAD_CHKSUM; i++) {
		chksum_calulated -= (_vec_rom[i] + 1);
	}

	// game version
	_game_ctx.game_ver = to_string(_vec_rom[ROM_HEAD_VERSION]);

	// destination code
	_game_ctx.dest_code = get_dest_code(_vec_rom[ROM_HEAD_DEST]);

	_game_ctx.chksum_passed = chksum_expected == chksum_calulated;
	LOG_INFO("Header read (checksum passed: ", (_game_ctx.chksum_passed ? "true" : "false"), ")");
	return true;
}

bool Cartridge::read_rom_to_buffer(const game_info& _game_ctx, vector<u8> & _vec_rom) {
	LOG_INFO("Reading rom");

	string full_file_path = get_full_file_path(_game_ctx);

	ifstream is(full_file_path, ios::binary | ios::beg);
	if (!is) { return false; }
	vector<u8> read_buffer((istreambuf_iterator<char>(is)), istreambuf_iterator<char>());
	is.close();

	_vec_rom = vector<u8>(read_buffer);
	return !_vec_rom.empty();
}

bool Cartridge::copy_rom_to_rom_folder(game_info& game_ctx, std::vector<u8>& _vec_rom, const string& _new_file_path) {
	if (_new_file_path.compare(game_ctx.file_path) == 0) return true;
	
	LOG_INFO("Copying file to .", ROM_FOLDER);

	if (check_and_create_file(ROM_FOLDER + game_ctx.file_name)) {
		LOG_WARN("File with same name already in .", ROM_FOLDER);
		return false;
	}
	
	ofstream os(_new_file_path + game_ctx.file_name, ios::binary | ios::beg);
	if (!os) { return false; }
	copy(_vec_rom.begin(), _vec_rom.end(), ostream_iterator<u8>(os));
	os.close();

	ifstream is(_new_file_path + game_ctx.file_name, ios::binary);
	if (!is) { return false; }
	vector<u8> read_buffer((istreambuf_iterator<char>(is)), istreambuf_iterator<char>());
	
	if (!equal(_vec_rom.begin(), _vec_rom.end(), read_buffer.begin()) || _vec_rom.size() != read_buffer.size()) { return false; }

	_vec_rom = read_buffer;
	game_ctx.file_path = _new_file_path;
	return true;
}

bool Cartridge::read_new_game(game_info& _game_ctx, const string& _path_to_rom) {
	auto vec_path_to_rom = split_string(_path_to_rom, "\\");

	string current_path = get_current_path();
	string s_path_rom_folder = current_path + ROM_FOLDER;
	string s_path_config = current_path + CONFIG_FOLDER;

	if (!check_ext(vec_path_to_rom.back())) return false;

	_game_ctx.file_path = "";
	for (int i = 0; i < vec_path_to_rom.size() - 1; i++) {
		_game_ctx.file_path += vec_path_to_rom[i] + "\\";
	}
	_game_ctx.file_name = vec_path_to_rom.back();

	vector<u8> vec_rom;
	if (!Cartridge::read_rom_to_buffer(_game_ctx, vec_rom)) {
		LOG_ERROR("Error while reading rom");
		return false;
	}

	if (!Cartridge::copy_rom_to_rom_folder(_game_ctx, vec_rom, s_path_rom_folder)) {
		LOG_WARN("Fallback to given path");
	}

	if (!Cartridge::read_header_info(_game_ctx, vec_rom)) {
		LOG_ERROR("Rom header corrupted");
		return false;
	}

	return true;
}
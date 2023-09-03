#include "Cartridge.h"

#include <fstream>

#include "config.h"
#include "logger.h"
#include "game_info.h"

using namespace std;

Cartridge::Cartridge(game_info& game_ctx) :
	game_ctx(&game_ctx)
{}

bool Cartridge::ReadData() {
	if (!read_rom_to_buffer(*this->game_ctx, vec_read_rom)) return false;
	if(!read_header_info(*this->game_ctx, vec_read_rom)) return false;

	// TODO: load cartridge data for execution

	return true;
}

bool Cartridge::read_header_info(game_info& game_ctx, vector<u8>& vec_rom) {
	// read header info -----
	LOG_INFO("Reading header info");

	// cgb/sgb flags
	game_ctx.is_cgb = vec_rom[ROM_HEAD_CGBFLAG] == 0x80 || vec_rom[ROM_HEAD_CGBFLAG] == 0xC0 ;
	game_ctx.is_sgb = vec_rom[ROM_HEAD_SGBFLAG] == 0x03;

	// title
	string title = "";
	int title_size_max = (game_ctx.is_cgb ? ROM_HEAD_TITLE_SIZE_CGB : ROM_HEAD_TITLE_SIZE_GB);
	for (int i = 0; vec_rom[ROM_HEAD_TITLE + i] != 0x00 && i < title_size_max; i++) {
		title += (char)vec_rom[ROM_HEAD_TITLE + i];
	}
	game_ctx.title = title;

	// licensee
	string licensee_code("");
	licensee_code += (char)vec_rom[ROM_HEAD_NEWLIC];
	licensee_code += (char)vec_rom[ROM_HEAD_NEWLIC + 1];

	game_ctx.licensee = get_licensee(vec_rom[ROM_HEAD_OLDLIC], licensee_code);

	// cart type
	game_ctx.cart_type = get_cart_type(vec_rom[ROM_HEAD_HW_TYPE]);

	// checksum
	u8 chksum_expected = vec_rom[ROM_HEAD_CHKSUM];
	u8 chksum_calulated = 0;
	for (int i = ROM_HEAD_TITLE; i <= ROM_HEAD_MASK; i++) {
		chksum_calulated -= ((vec_rom[i]) + 1);
	}

	game_ctx.chksum_passed = chksum_expected == chksum_calulated;
	LOG_INFO("Header read (checksum passed: ", (game_ctx.chksum_passed ? "true" : "false"), ")");
	return true;
}

bool Cartridge::read_rom_to_buffer(const game_info& game_ctx, vector<u8> &vec_rom) {
	LOG_INFO("Reading rom");

	string full_file_path = get_full_file_path(game_ctx);

	ifstream is(full_file_path, ios::binary);
	if (!is) { return false; }
	vector<u8> read_buffer((istreambuf_iterator<char>(is)), istreambuf_iterator<char>());
	vec_rom = read_buffer;

	is.close();
	return true;
}

bool Cartridge::copy_rom_to_rom_folder(game_info& game_info, const std::vector<u8>& vec_rom, const string& new_file_path) {
	LOG_INFO("Copying file to /rom");
	
	ofstream os(new_file_path + game_info.file_name, ios::binary);
	if (!os) { return false; }
	copy(vec_rom.begin(), vec_rom.end(), ostream_iterator<u8>(os));
	os.close();

	ifstream is(new_file_path + game_info.file_name, ios::binary);
	if (!is) { return false; }
	vector<u8> read_buffer((istreambuf_iterator<char>(is)), istreambuf_iterator<char>());
	
	if (!equal(vec_rom.begin(), vec_rom.end(), read_buffer.begin())) { return false; }

	game_info.file_path = new_file_path;
	return true;
}
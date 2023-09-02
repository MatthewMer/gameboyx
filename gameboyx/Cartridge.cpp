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

bool Cartridge::read_header_info(game_info& game_ctx, vector<byte>& vec_rom) {
	// read header info -----

	// cgb/sgb flags
	game_ctx.is_gbc = vec_rom[ROM_HEAD_CGBFLAG] == byte{ 0x80 } || vec_rom[ROM_HEAD_CGBFLAG] == byte{ 0xC0 };
	game_ctx.is_sgb = vec_rom[ROM_HEAD_SGBFLAG] == byte{ 0x03 };

	// title
	string title = "";
	int title_size_max = (game_ctx.is_gbc ? ROM_HEAD_TITLE_SIZE_CGB : ROM_HEAD_TITLE_SIZE_GB);
	for (int i = 0; vec_rom[ROM_HEAD_TITLE + i] != byte{0x00} && i < title_size_max; i++) {
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
	game_ctx.chksum = to_integer<u8>(vec_rom[ROM_HEAD_CHKSUM]);
	u8 chksum_calulated = 0;
	for (int i = ROM_HEAD_TITLE; i <= ROM_HEAD_MASK; i++) {
		chksum_calulated -= (to_integer<u8>(vec_rom[i]) + 1);
	}

	game_ctx.chksum_passed = (game_ctx.chksum.compare(to_string(chksum_calulated)) == 0);
	LOG_INFO("Header read (checksum passed: ", (game_ctx.chksum_passed ? "true" : "false"), ")");
	return true;
}

bool Cartridge::read_rom_to_buffer(const game_info& game_ctx, vector<byte> &vec_rom) {
	LOG_INFO("Reading file");

	vec_rom.clear();
	auto read_buffer = vector<char>();

	string full_file_path = get_full_file_path(game_ctx);

	ifstream file(full_file_path, ios::binary | ios::ate);
	if (!file) {
		LOG_ERROR("Failed to open file");
		return false;
	}

	streamsize size = file.tellg();
	file.seekg(0, ios::beg);

	read_buffer = vector<char>(size);

	if (!file.read(read_buffer.data(), size)) {
		LOG_ERROR("Failed to read file");
		file.close();
		return false;
	}

	vec_rom = vector<byte>(read_buffer.size());

	for (const char& c : read_buffer) {
		vec_rom.push_back((byte)(c & 0xff));
	}
	file.close();
	return true;
}
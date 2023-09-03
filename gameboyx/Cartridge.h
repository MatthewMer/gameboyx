#pragma once

/* ***********************************************************************************************************
	INCLUDES
*********************************************************************************************************** */

#include <vector>

#include "defs.h"
#include "game_info.h"

/* ***********************************************************************************************************
	CLASSES
*********************************************************************************************************** */
class Cartridge
{
private:
	// vec_read_rom
	std::vector<u8> vec_read_rom;
	game_info* game_ctx;

public:
	explicit Cartridge(game_info& game_ctx);
	bool ReadData();

	static bool read_header_info(game_info& game_info, std::vector<u8>& vec_rom);
	static bool read_rom_to_buffer(const game_info& game_info, std::vector<u8> &vec_rom);
	static bool copy_rom_to_rom_folder(game_info& game_info, std::vector<u8>& vec_rom, const std::string& new_file_path);
	static bool read_new_game(game_info& game_ctx, const std::string& path_to_rom);
};
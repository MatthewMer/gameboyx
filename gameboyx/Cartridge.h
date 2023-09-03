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
	std::vector<u8> vecRom;
	game_info* gameCtx;

public:
	explicit constexpr Cartridge(game_info& _game_ctx) : gameCtx(&_game_ctx) {};
	bool ReadData();

	static bool read_header_info(game_info& _game_info, std::vector<u8>& _vec_rom);
	static bool read_rom_to_buffer(const game_info& _game_info, std::vector<u8>& _vec_rom);
	static bool copy_rom_to_rom_folder(game_info& _game_info, std::vector<u8>& _vec_rom, const std::string& _new_file_path);
	static bool read_new_game(game_info& _game_ctx, const std::string& _path_to_rom);
};
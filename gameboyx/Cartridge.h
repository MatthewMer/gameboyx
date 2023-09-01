#pragma once

/* ***********************************************************************************************************
	INCLUDES
*********************************************************************************************************** */

#include <vector>

#include "defs.h"
#include "game_info.h"

using namespace std;

/* ***********************************************************************************************************
	CLASSES
*********************************************************************************************************** */
class Cartridge
{
private:
	// vec_read_rom
	vector<byte> vec_read_rom;
	game_info* game_ctx;

public:
	explicit Cartridge(game_info& game_ctx);
	bool ReadData();

	static bool read_header_info(game_info& game_info, vector<byte>& vec_read_rom);
	static bool read_rom_to_buffer(const game_info& game_info, vector<byte> &vec_read_rom);
};
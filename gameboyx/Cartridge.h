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
public:
	// singleton instance access
	static Cartridge* getInstance(const game_info& _game_ctx);
	static void resetInstance();

	// clone/assign protection
	Cartridge(Cartridge const&) = delete;
	Cartridge(Cartridge&&) = delete;
	Cartridge& operator=(Cartridge const&) = delete;
	Cartridge& operator=(Cartridge&&) = delete;

	static bool read_basic_header_info(game_info& _game_info, std::vector<u8>& _vec_rom);
	static bool read_rom_to_buffer(const game_info& _game_info, std::vector<u8>& _vec_rom);
	static bool copy_rom_to_rom_folder(game_info& _game_info, std::vector<u8>& _vec_rom, const std::string& _new_file_path);
	static bool read_new_game(game_info& _game_ctx, const std::string& _path_to_rom);
	static void check_and_create_rom_folder();

	// getter
	constexpr const game_info* GetGameInfo() const {
		return std::to_address(gameCtx);
	}

private:
	// constructor
	static Cartridge* instance;
	explicit constexpr Cartridge(const game_info& _game_ctx) : gameCtx(&_game_ctx) {};
	// destructor
	~Cartridge() = default;

	// vec_read_rom
	std::vector<u8> vecRom = std::vector<u8>();
	const game_info* gameCtx;

	bool ReadData();
};
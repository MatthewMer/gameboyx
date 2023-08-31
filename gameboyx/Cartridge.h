#pragma once

#include <vector>
#include "defs.h"

enum mapper_types {
	ROM_ONLY,
	MBC1,
	MBC2,
	MBC3,
	MBC5,
	MBC6,
	MBC7,
	HuC3,
	HuC1
};

class Cartridge
{
private:
	// read_buffer
	std::vector<char> read_buffer;

	// info
	char* file_path;
	bool is_gbc = false;
	int rom_bank_num;
	int rom_bank_size;
	int ram_bank_num;
	int ram_bank_size;
	int mapper;
	bool battery = false;
	bool timer = false;

public:
	Cartridge(char* file_path);
	bool read_data();
};


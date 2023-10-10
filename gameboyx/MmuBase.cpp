#include "MmuBase.h"

#include "MmuSM83.h"
#include "logger.h"

#include <vector>

using namespace std;

/* ***********************************************************************************************************
	MAPPER TYPES (GAMEBOY)
*********************************************************************************************************** */
enum gameboy_mapper_types {
	MAPPER_NONE,
	ROM,
	MBC1,
	MBC2,
	MBC3,
	MBC5,
	MBC6,
	MBC7,
	MMM01,
	HuC1,
	HuC3
};

inline const vector<pair<u8, gameboy_mapper_types>> gameboy_mapper_map{
	{0x00, ROM},
	{0x01, MBC1},
	{0x02, MBC1},
	{0x03, MBC1},
	{0x05, MBC2},
	{0x06, MBC2},
	{0x08, ROM},
	{0x09, ROM},
	{0x0B, MMM01},
	{0x0C, MMM01},
	{0x0D, MMM01},
	{0x0F, MBC3},
	{0x10, MBC3},
	{0x11, MBC3},
	{0x12, MBC3},
	{0x13, MBC3},
	{0x19, MBC5},
	{0x1A, MBC5},
	{0x1B, MBC5},
	{0x1C, MBC5},
	{0x1D, MBC5},
	{0x1E, MBC5},
	{0x20, MBC6},
	{0x22, MBC7},
	{0xFC, MAPPER_NONE},			// TODO
	{0xFD, MAPPER_NONE},			// TODO
	{0xFE, HuC3},
	{0xFF, HuC1}
};

inline gameboy_mapper_types gameboy_get_mapper(const u8& _type_code) {
	for (const auto& n : gameboy_mapper_map) {
		if (n.first == _type_code) { return n.second; }
	}
	return MAPPER_NONE;
}

/* ***********************************************************************************************************
	MMU BASE CLASS
*********************************************************************************************************** */
MmuBase* MmuBase::instance = nullptr;

MmuBase* MmuBase::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = getNewMmuInstance(_machine_info);
	if (instance == nullptr) {
		LOG_ERROR("Couldn't create MMU");
	}
	return instance;
}

void MmuBase::resetInstance() {
	if (instance != nullptr) {
		instance->ResetChildMemoryInstances();
		delete instance;
		instance = nullptr;
	}
}

MmuBase* MmuBase::getNewMmuInstance(machine_information& _machine_info) {
	vector<u8> vec_rom = Cartridge::getInstance()->GetRomVector();

	switch (gameboy_get_mapper(vec_rom[ROM_HEAD_HW_TYPE])) {
	case MBC1:
		return new MmuSM83_MBC1(_machine_info);
		break;
	case MBC3:
		return new MmuSM83_MBC3(_machine_info);
		break;
	default:
		return nullptr;
		break;
	}
}


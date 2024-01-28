#include "GameboyMMU.h"

#include "GameboyMEM.h"
#include "gameboy_defines.h"

#include "logger.h"

#include <format>
#include <unordered_map>

using namespace std;

/* ***********************************************************************************************************
	MAPPER TYPES (GAMEBOY)
*********************************************************************************************************** */
enum gameboy_mapper_types {
	GB_NONE,
	ROM_ONLY,
	ROM_RAM,
	ROM_RAM_BATTERY,
	MBC1,
	MBC1_RAM,
	MBC1_RAM_BATTERY,
	MBC2,
	MBC2_BATTERY,
	MBC3,
	MBC3_RAM,
	MBC3_RAM_BATTERY,
	MBC3_TIMER_BATTERY,
	MBC3_TIMER_RAM_BATTERY,
	MBC5,
	MBC5_RAM,
	MBC5_RAM_BATTERY,
	MBC6,
	MBC7_RAM_BATTERY,
	MMM01,
	MMM01_RAM,
	MMM01_RAM_BATTERY,
	HuC1_RAM_BATTERY,
	HuC3
};

const unordered_map<u8, gameboy_mapper_types> gameboy_mapper_map{
	{0x00, ROM_ONLY},
	{0x01, MBC1},
	{0x02, MBC1_RAM},
	{0x03, MBC1_RAM_BATTERY},
	{0x05, MBC2},
	{0x06, MBC2_BATTERY},
	{0x08, ROM_RAM},
	{0x09, ROM_RAM_BATTERY},
	{0x0B, MMM01},
	{0x0C, MMM01_RAM},
	{0x0D, MMM01_RAM_BATTERY},
	{0x0F, MBC3_TIMER_BATTERY},
	{0x10, MBC3_TIMER_RAM_BATTERY},
	{0x11, MBC3},
	{0x12, MBC3_RAM},
	{0x13, MBC3_RAM_BATTERY},
	{0x19, MBC5},
	{0x1A, MBC5_RAM},
	{0x1B, MBC5_RAM_BATTERY},
	{0x1C, MBC5},
	{0x1D, MBC5_RAM},
	{0x1E, MBC5_RAM_BATTERY},
	{0x20, MBC6},
	{0x22, MBC7_RAM_BATTERY},
	{0xFC, GB_NONE},			// TODO
	{0xFD, GB_NONE},			// TODO
	{0xFE, HuC3},
	{0xFF, HuC1_RAM_BATTERY}
};

GameboyMMU* GameboyMMU::getInstance(machine_information& _machine_info) {
	const auto vec_rom = _machine_info.cartridge->GetRomVector();
	u8 type_code = vec_rom[ROM_HEAD_HW_TYPE];

	if (gameboy_mapper_map.find(type_code) != gameboy_mapper_map.end()) {

		gameboy_mapper_types type = gameboy_mapper_map.at(type_code);
		switch (type) {
		case ROM_RAM_BATTERY:
			_machine_info.batteryBuffered = true;
		case ROM_RAM:
			_machine_info.ramPresent = true;
		case ROM_ONLY:
			return new MmuSM83_ROM(_machine_info);
			break;
		case MBC1_RAM_BATTERY:
			_machine_info.batteryBuffered = true;
		case MBC1_RAM:
			_machine_info.ramPresent = true;
		case MBC1:
			return new MmuSM83_MBC1(_machine_info);
			break;
		case MBC3_TIMER_RAM_BATTERY:
			_machine_info.ramPresent = true;
		case MBC3_TIMER_BATTERY:
			_machine_info.timerPresent = true;
		case MBC3_RAM_BATTERY:
			_machine_info.batteryBuffered = true;
		case MBC3_RAM:
			if (type == MBC3_RAM_BATTERY || type == MBC3_RAM) {
				_machine_info.ramPresent = true;
			}
		case MBC3:
			return new MmuSM83_MBC3(_machine_info);
			break;
		default:
			LOG_WARN("Mapper type ", format("{:02x}", type_code), " not implemented");
			break;
		}
	} else {
		LOG_ERROR("Mapper type ", format("{:02x}", type_code), " unknown");
		return nullptr;
	}
}

GameboyMMU::GameboyMMU(machine_information& _machine_info) : BaseMMU(_machine_info){
	mem_instance = GameboyMEM::getInstance(_machine_info);
	machine_ctx = mem_instance->GetMachineContext();
}

/* ***********************************************************************************************************
*
*		ROM only
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
	CONSTRUCTOR
*********************************************************************************************************** */
MmuSM83_ROM::MmuSM83_ROM(machine_information& _machine_info) : GameboyMMU(_machine_info) {
	if (_machine_info.batteryBuffered && _machine_info.ramPresent) {
		ReadSave();
	}
}

/* ***********************************************************************************************************
	MEMORY ACCESS
*********************************************************************************************************** */
void MmuSM83_ROM::Write8Bit(const u8& _data, const u16& _addr) {
	// ROM Bank 0
	if (_addr < ROM_N_OFFSET) {
		return;
	}
	// ROM Bank 1
	else if (_addr < VRAM_N_OFFSET) {
		return;
	}
	// VRAM 0-n
	else if (_addr < RAM_N_OFFSET) {
		mem_instance->WriteVRAM_N(_data, _addr);
	}
	// RAM 0-n
	else if (_addr < WRAM_0_OFFSET) {
		if (machineInfo.ramPresent) {
			mem_instance->WriteRAM_N(_data, _addr);
		} else {
			return;
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		mem_instance->WriteWRAM_0(_data, _addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		mem_instance->WriteWRAM_N(_data, _addr);
	}
	// MIRROR WRAM, prohibited
	else if (_addr < OAM_OFFSET) {
		return;
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		mem_instance->WriteOAM(_data, _addr);
	}
	// NOT USED
	else if (_addr < IO_OFFSET) {
		return;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		mem_instance->WriteIO(_data, _addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		mem_instance->WriteHRAM(_data, _addr);
	}
	// IE register
	else {
		mem_instance->WriteIE(_data);
	}
}

// CGB is little endian: low byte first, high byte second
void MmuSM83_ROM::Write16Bit(const u16& _data, const u16& _addr) {
	Write8Bit(_data & 0xFF, _addr);
	Write8Bit((_data & 0xFF00) >> 8, _addr + 1);
}


u8 MmuSM83_ROM::Read8Bit(const u16& _addr) {
	// ROM Bank 0
	if (_addr < ROM_N_OFFSET) {
		return mem_instance->ReadROM_0(_addr);
	}
	// ROM Bank 1-n
	else if (_addr < VRAM_N_OFFSET) {
		return mem_instance->ReadROM_N(_addr);
	}
	// VRAM 0-n
	else if (_addr < RAM_N_OFFSET) {
		return mem_instance->ReadVRAM_N(_addr);
	}
	// RAM 0-n
	else if (_addr < WRAM_0_OFFSET) {
		if (machineInfo.ramPresent) {
			return mem_instance->ReadRAM_N(_addr);
		}
		else {
			return 0xFF;
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		return mem_instance->ReadWRAM_N(_addr);
	}
	// MIRROR WRAM (prohibited)
	else if (_addr < OAM_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		return mem_instance->ReadOAM(_addr);
	}
	// NOT USED
	else if (_addr < IO_OFFSET) {
		return 0x00;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		return mem_instance->ReadIO(_addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		return mem_instance->ReadHRAM(_addr);
	}
	// IE register
	else {
		return mem_instance->ReadIE();
	}
}

/* ***********************************************************************************************************
*
*		MBC1
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
	MBC1 DEFINES
*********************************************************************************************************** */
// addresses
#define MBC1_RAM_TIMER_ENABLE			0x0000
#define MBC1_ROM_BANK_NUMBER_SEL_0_4	0x2000
#define MBC1_ROM_BANK_NUMBER_SEL_5_6	0x4000

#define MBC1_RAM_BANK_NUMBER			0x4000
#define MBC1_BANKING_MODE				0x6000

// masks
#define MBC1_ROM_BANK_MASK_0_4			0x1f
#define MBC1_ROM_BANK_MASK_5_6			0x60
#define MBC1_RAM_ENABLE_MASK			0x0F
#define MBC1_RAM_ROM_BANK_MASK			0x03

// values
#define MBC1_RAM_ENABLE					0x0A

/* ***********************************************************************************************************
	CONSTRUCTOR
*********************************************************************************************************** */
MmuSM83_MBC1::MmuSM83_MBC1(machine_information& _machine_info): GameboyMMU(_machine_info) {

	switch (machine_ctx->ram_bank_num) {
	case 0:
	case 1:
		ramBankMask = 0x00;
		break;
	case 2:
		ramBankMask = 0x01;
		break;
	case 3:
	case 4:
		ramBankMask = 0x03;
		break;
	}

	if (_machine_info.batteryBuffered && _machine_info.ramPresent) {
		ReadSave();
	}

	/*
	// bit mask for ROM Bank Number
	for (int i = machine_ctx->rom_bank_num; i < 32 && i >= 2; i *= 2) {
		romBankMask >>= 1;
	}
	// set mask for upper bits of zero bank
	if (machine_ctx->rom_bank_num < 64) { romBankMaskAdvanced &= 0x00; }
	else if(machine_ctx->rom_bank_num < 128) { romBankMaskAdvanced &= 0x20; }
	*/
}

/* ***********************************************************************************************************
	MEMORY ACCESS
*********************************************************************************************************** */
void MmuSM83_MBC1::Write8Bit(const u8& _data, const u16& _addr) {
	// ROM Bank 0 -> RAM/Timer enable and ROM Bank select
	if (_addr < ROM_N_OFFSET) {
		// RAM/TIMER enable
		if (_addr < MBC1_ROM_BANK_NUMBER_SEL_0_4) {
			ramEnable = (_data & MBC1_RAM_ENABLE_MASK) == MBC1_RAM_ENABLE;

			if (!ramEnable && machineInfo.ramPresent && machineInfo.batteryBuffered) {
				WriteSave();
			}
			
		}
		// ROM Bank number
		else {
			int romBankNumber = _data & MBC1_ROM_BANK_MASK_0_4;
			if (romBankNumber == 0) romBankNumber = 1;
			if (advancedBankingMode) {
				romBankNumber |= (advancedBankingValue << 5) & MBC1_ROM_BANK_MASK_5_6;
			}
			machine_ctx->rom_bank_selected = romBankNumber - 1;
		}
	}
	// ROM Bank 1-n -> RAM Bank select or RTC register select or Latch Clock Data
	else if (_addr < VRAM_N_OFFSET) {
		// RAM Bank number or RTC register select
		if (_addr < MBC1_BANKING_MODE) {
			advancedBankingValue = _data & MBC1_RAM_ROM_BANK_MASK;
			machine_ctx->ram_bank_selected = advancedBankingValue & ramBankMask;
		}
		else {
			advancedBankingMode = (_data ? true : false);
		}
	}
	// VRAM 0-n
	else if (_addr < RAM_N_OFFSET) {
		mem_instance->WriteVRAM_N(_data, _addr);
	}
	// RAM 0-n
	else if (_addr < WRAM_0_OFFSET) {
		if (machineInfo.ramPresent) {
			if (ramEnable) {
				mem_instance->WriteRAM_N(_data, _addr);
			}
		} else {
			LOG_ERROR("[emu] tried to access nonpresent RAM");
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		mem_instance->WriteWRAM_0(_data, _addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		mem_instance->WriteWRAM_N(_data, _addr);
	}
	// MIRROR WRAM, prohibited
	else if (_addr < OAM_OFFSET) {
		return;
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		mem_instance->WriteOAM(_data, _addr);
	}
	// NOT USED
	else if (_addr < IO_OFFSET) {
		return;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		mem_instance->WriteIO(_data, _addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		mem_instance->WriteHRAM(_data, _addr);
	}
	// IE register
	else {
		mem_instance->WriteIE(_data);
	}
}

// CGB is little endian: low byte first, high byte second
void MmuSM83_MBC1::Write16Bit(const u16& _data, const u16& _addr) {
	Write8Bit(_data & 0xFF, _addr);
	Write8Bit((_data & 0xFF00) >> 8, _addr + 1);
}


u8 MmuSM83_MBC1::Read8Bit(const u16& _addr) {
	// ROM Bank 0
	if (_addr < ROM_N_OFFSET) {
		if (advancedBankingMode) {

		}
		else {
			return mem_instance->ReadROM_0(_addr);
		}
	}
	// ROM Bank 1-n
	else if (_addr < VRAM_N_OFFSET) {
		return mem_instance->ReadROM_N(_addr);
	}
	// VRAM 0-n
	else if (_addr < RAM_N_OFFSET) {
		return mem_instance->ReadVRAM_N(_addr);
	}
	// RAM 0-n
	else if (_addr < WRAM_0_OFFSET) {
		if (ramEnable && machineInfo.ramPresent) {
			return mem_instance->ReadRAM_N(_addr);
		}
		else {
			return 0xFF;
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		return mem_instance->ReadWRAM_N(_addr);
	}
	// MIRROR WRAM (prohibited)
	else if (_addr < OAM_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		return mem_instance->ReadOAM(_addr);
	}
	// NOT USED
	else if (_addr < IO_OFFSET) {
		return 0x00;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		return mem_instance->ReadIO(_addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		return mem_instance->ReadHRAM(_addr);
	}
	// IE register
	else {
		return mem_instance->ReadIE();
	}
}

/* ***********************************************************************************************************
*
*		MBC3
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
	MBC3 DEFINES
*********************************************************************************************************** */
// addresses
#define MBC3_RAM_TIMER_ENABLE			0x0000
#define MBC3_ROM_BANK_NUMBER_SELECT		0x2000

#define MBC3_RAM_BANK_NUMBER_OR_RTC_REG	0x4000
#define MBC3_LATCH_CLOCK_DATA			0x6000

// masks
#define MBC3_ROM_BANK_MASK				0x7f
#define MBC3_RAM_ENABLE_MASK			0x0F
#define MBC3_RAM_BANK_MASK				0x0F

// values
#define MBC3_RAM_ENABLE					0x0A

/* ***********************************************************************************************************
	CONSTRUCTOR
*********************************************************************************************************** */
MmuSM83_MBC3::MmuSM83_MBC3(machine_information& _machine_info) : GameboyMMU(_machine_info) {
	if (_machine_info.batteryBuffered && _machine_info.ramPresent) {
		ReadSave();
	}
}

/* ***********************************************************************************************************
	MEMORY ACCESS
*********************************************************************************************************** */
void MmuSM83_MBC3::Write8Bit(const u8& _data, const u16& _addr) {
	// ROM Bank 0 -> RAM/Timer enable and ROM Bank select
	if (_addr < ROM_N_OFFSET) {
		// RAM/TIMER enable
		if (_addr < MBC3_ROM_BANK_NUMBER_SELECT) {
			timerRamEnable = (_data & MBC3_RAM_ENABLE_MASK) == MBC3_RAM_ENABLE;
			if (!timerRamEnable && machineInfo.ramPresent && machineInfo.batteryBuffered) {
				WriteSave();
			}
		}
		// ROM Bank number
		else {
			int romBankNumber = _data & MBC3_ROM_BANK_MASK;
			if (romBankNumber == 0) romBankNumber = 1;
			machine_ctx->rom_bank_selected = romBankNumber - 1;
		}
	}
	// ROM Bank 1-n -> RAM Bank select or RTC register select or Latch Clock Data
	else if (_addr < VRAM_N_OFFSET) {
		// RAM Bank number or RTC register select
		if (_addr < MBC3_LATCH_CLOCK_DATA) {
			int ramBankRtcNumber = _data & MBC3_RAM_BANK_MASK;
			machine_ctx->ram_bank_selected = ramBankRtcNumber;
		}
		else {
			if (_data == 0x01 && rtcRegistersLastWrite == 0x00) {
				LatchClock();
			}
			else {
				rtcRegistersLastWrite = _data;
			}
		}
	}
	// VRAM 0-n
	else if (_addr < RAM_N_OFFSET) {
		mem_instance->WriteVRAM_N(_data, _addr);
	}
	// RAM 0-n -> RTC Registers 08-0C
	else if (_addr < WRAM_0_OFFSET) {
		if(machineInfo.ramPresent){
			if (timerRamEnable) {
				int ramBankNumber = machine_ctx->ram_bank_selected;
				if (ramBankNumber < 0x04) {
					mem_instance->WriteRAM_N(_data, _addr);
				} else if (ramBankNumber > 0x07 && ramBankNumber < 0x0D) {
					WriteClock(_data);
				}
			}
		} else {
			LOG_ERROR("[emu] tried to access nonpresent RAM");
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		mem_instance->WriteWRAM_0(_data, _addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		mem_instance->WriteWRAM_N(_data, _addr);
	}
	// MIRROR WRAM, prohibited
	else if (_addr < OAM_OFFSET) {
		return;
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		mem_instance->WriteOAM(_data, _addr);
	}
	// NOT USED
	else if (_addr < IO_OFFSET) {
		return;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		mem_instance->WriteIO(_data, _addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		mem_instance->WriteHRAM(_data, _addr);
	}
	// IE register
	else {
		mem_instance->WriteIE(_data);
	}
}

// CGB is little endian: low byte first, high byte second
void MmuSM83_MBC3::Write16Bit(const u16& _data, const u16& _addr) {
	Write8Bit(_data & 0xFF, _addr);
	Write8Bit((_data & 0xFF00) >> 8, _addr + 1);
}

u8 MmuSM83_MBC3::Read8Bit(const u16& _addr) {
	// ROM Bank 0
	if (_addr < ROM_N_OFFSET) {
		return mem_instance->ReadROM_0(_addr);
	}
	// ROM Bank 1-n
	else if (_addr < VRAM_N_OFFSET) {
		return mem_instance->ReadROM_N(_addr);
	}
	// VRAM 0-n
	else if (_addr < RAM_N_OFFSET) {
		return mem_instance->ReadVRAM_N(_addr);
	}
	// RAM 0-n
	else if (_addr < WRAM_0_OFFSET) {
		if (timerRamEnable && machineInfo.ramPresent) {
			int ramBankNumber = machine_ctx->ram_bank_selected;
			if (ramBankNumber < 0x04) {
				return mem_instance->ReadRAM_N(_addr);
			}
			else if (ramBankNumber > 0x07 && ramBankNumber < 0x0D) {
				return ReadClock();
			}
		} else {
			return 0xFF;
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		return mem_instance->ReadWRAM_N(_addr);
	}
	// MIRROR WRAM (prohibited)
	else if (_addr < OAM_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		return mem_instance->ReadOAM(_addr);
	}
	// NOT USED
	else if (_addr < IO_OFFSET) {
		return 0x00;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		return mem_instance->ReadIO(_addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		return mem_instance->ReadHRAM(_addr);
	}
	// IE register
	else {
		return mem_instance->ReadIE();
	}
}

/* ***********************************************************************************************************
	RTC			TODO !!!!!
*********************************************************************************************************** */
void MmuSM83_MBC3::LatchClock() {

}

u8 MmuSM83_MBC3::ReadClock() {
	return 0x00;
}

void MmuSM83_MBC3::WriteClock(const u8& _data) {

}
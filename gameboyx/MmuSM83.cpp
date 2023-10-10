#include "MmuSM83.h"

#include "MemorySM83.h"
#include "gameboy_defines.h"

#include "logger.h"

#include <format>

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
#define MBC1_BANKING_MODE				0x6000

#define MBC1_RAM_BANK_NUMBER			0x4000

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
MmuSM83_MBC1::MmuSM83_MBC1(machine_information& _machine_info) : MmuBase(_machine_info) {
	mem_instance = MemorySM83::getInstance(_machine_info);
	machine_ctx = mem_instance->GetMachineContext();
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
			machine_ctx->ram_bank_selected = advancedBankingValue;
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
		if (ramEnable) {
			int ramBankNumber = machine_ctx->ram_bank_selected;
			if (ramBankNumber < 0x04) {
				mem_instance->WriteRAM_N(_data, _addr);
			}
			else if (ramBankNumber > 0x07 && ramBankNumber < 0x0D) {
				// WriteClock(_data);
			}
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
	else if (_addr < IO_REGISTERS_OFFSET) {
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
		return mem_instance->ReadRAM_N(_addr);
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
	else if (_addr < IO_REGISTERS_OFFSET) {
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

// CGB is little endian: low byte first, high byte second
u16 MmuSM83_MBC1::Read16Bit(const u16& _addr) {
	dataBuffer = ((u16)Read8Bit(_addr + 1) << 8);
	dataBuffer |= Read8Bit(_addr);
	return dataBuffer;
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
MmuSM83_MBC3::MmuSM83_MBC3(machine_information& _machine_info) : MmuBase(_machine_info) {
	mem_instance = MemorySM83::getInstance(_machine_info);
	machine_ctx = mem_instance->GetMachineContext();
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
		if (timerRamEnable) {
			int ramBankNumber = machine_ctx->ram_bank_selected;
			if (ramBankNumber < 0x04) {
				mem_instance->WriteRAM_N(_data, _addr);
			}
			else if (ramBankNumber > 0x07 && ramBankNumber < 0x0D) {
				WriteClock(_data);
			}
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
	else if (_addr < IO_REGISTERS_OFFSET) {
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
		if (timerRamEnable) {
			int ramBankNumber = machine_ctx->ram_bank_selected;
			if (ramBankNumber < 0x04) {
				return mem_instance->ReadRAM_N(_addr);
			}
			else if (ramBankNumber > 0x07 && ramBankNumber < 0x0D) {
				return ReadClock();
			}
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
	else if (_addr < IO_REGISTERS_OFFSET) {
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

// CGB is little endian: low byte first, high byte second
u16 MmuSM83_MBC3::Read16Bit(const u16& _addr) {
	dataBuffer = ((u16)Read8Bit(_addr + 1) << 8);
	dataBuffer |= Read8Bit(_addr);
	return dataBuffer;
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
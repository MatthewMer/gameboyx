#include "MmuSM83.h"

#include "gameboy_config.h"

/* ***********************************************************************************************************
	DEFINES
*********************************************************************************************************** */
#define RAM_TIMER_ENABLE			

/* ***********************************************************************************************************
	CONSTRUCTOR
*********************************************************************************************************** */
MmuSM83_MBC3::MmuSM83_MBC3(const Cartridge& _cart_obj) {
	this->isCgb = _cart_obj.GetIsCgb();

	InitMmu(_cart_obj);

	mem_instance = Memory::getInstance(_cart_obj);
}

/* ***********************************************************************************************************
	MEMORY ACCESS
*********************************************************************************************************** */
void MmuSM83_MBC3::Write8Bit(const u8& _data, const u16& _addr) {
	// ROM Bank 0 -> RAM/Timer enable and ROM Bank select
	if (_addr < ROM_BANK_N_OFFSET) {
		// TODO
	}
	// ROM Bank 1-n -> RAM Bank select or RTC register select or Latch Clock Data
	else if (_addr < VRAM_N_OFFSET) {
		// TODO
	}
	// VRAM 0-n
	else if (_addr < RAM_BANK_N_OFFSET) {
		mem_instance->WriteVRAM_N(_data, _addr, 0x00);
	}
	// RAM 0-n -> RTC Registers 08-0C
	else if (_addr < WRAM_0_OFFSET) {
		// TODO
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		mem_instance->WriteWRAM_0(_data, _addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		mem_instance->WriteWRAM_N(_data, _addr, 0x00);
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
	if (_addr < ROM_BANK_N_OFFSET) {
		return mem_instance->ReadROM_0(_addr);
	}
	// ROM Bank 1-n
	else if (_addr < VRAM_N_OFFSET) {
		return mem_instance->ReadROM_N(_addr, 0x00);
	}
	// VRAM 0-n
	else if (_addr < RAM_BANK_N_OFFSET) {
		return mem_instance->ReadVRAM_N(_addr, 0x00);
	}
	// RAM 0-n
	else if (_addr < WRAM_0_OFFSET) {
		return mem_instance->ReadRAM_N(_addr, 0x00);
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		return mem_instance->ReadWRAM_N(_addr, 0x00);
	}
	// MIRROR WRAM
	else if (_addr < OAM_OFFSET) {
		//return mem_instance->ReadWRAM_0(_addr - MIRROR_WRAM_OFFSET);
		return 0x00;
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
}

/* ***********************************************************************************************************
	MMU INITIALIZATION
*********************************************************************************************************** */
void MmuSM83_MBC3::InitMmu(const Cartridge& _cart_obj) {
	if (!ReadRomHeaderInfo(_cart_obj.GetRomVector())) { return; }


}

bool MmuSM83_MBC3::ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) {
	if (_vec_rom.size() < ROM_HEAD_ADDR + ROM_HEAD_SIZE) { return false; }
}
#pragma once

#include "Cartridge.h"
#include "Memory.h"

#include "defs.h"

class Memory
{
public:
	// get/reset instance
	static Memory* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	Memory(Memory const&) = delete;
	Memory(Memory&&) = delete;
	Memory& operator=(Memory const&) = delete;
	Memory& operator=(Memory&&) = delete;

	// members for memory access
	u8 ReadROM_0(const u16& _addr);
	u8 ReadROM_N(const u16& _addr, const int& _bank);
	u8 ReadVRAM_N(const u16& _addr, const int& _bank);
	u8 ReadRAM_N(const u16& _addr, const int& _bank);
	u8 ReadWRAM_0(const u16& _addr);
	u8 ReadWRAM_N(const u16& _addr, const int& _bank);
	u8 ReadOAM(const u16& _addr);
	u8 ReadIO(const u16& _addr);
	u8 ReadHRAM(const u16& _addr);
	u8 ReadIE();

	void WriteVRAM_N(const u8& _data, const u16& _addr, const int& _bank);
	void WriteRAM_N(const u8& _data, const u16& _addr, const int& _bank);
	void WriteWRAM_0(const u8& _data, const u16& _addr);
	void WriteWRAM_N(const u8& _data, const u16& _addr, const int& _bank);
	void WriteOAM(const u8& _data, const u16& _addr);
	void WriteIO(const u8& _data, const u16& _addr);
	void WriteHRAM(const u8& _data, const u16& _addr);
	void WriteIE(const u8& _data);

private:
	// constructor
	explicit Memory(const Cartridge& _cart_obj);
	static Memory* instance;
	// destructor
	~Memory() = default;

	// members
	void InitMemory(const Cartridge& _cart_obj);
	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom);
	bool CopyRom(const std::vector<u8>& _vec_rom);

	void AllocateMemory();
	void CleanupMemory();

	int romBankNum = 0;
	int ramBankNum = 0;

	bool isCgb = false;

	// actual memory
	u8* ROM_0;
	u8** ROM_N;
	u8** VRAM_N;
	u8** RAM_N;
	u8* WRAM_0;
	u8** WRAM_N;
	u8* OAM;
	u8* IO;
	u8* HRAM;
	u8 IE = 0;
};
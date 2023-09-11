#pragma once

#include "Cartridge.h"
#include "MemoryBase.h"

#include "defs.h"

class MemorySM83 : private MemoryBase
{
public:
	// get/reset instance
	static MemorySM83* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	MemorySM83(MemorySM83 const&) = delete;
	MemorySM83(MemorySM83&&) = delete;
	MemorySM83& operator=(MemorySM83 const&) = delete;
	MemorySM83& operator=(MemorySM83&&) = delete;

	// members for memory access
	u8 ReadROM_0(const u16& _addr);
	u8 ReadROM_N(const u16& _addr);
	u8 ReadVRAM_N(const u16& _addr);
	u8 ReadRAM_N(const u16& _addr);
	u8 ReadWRAM_0(const u16& _addr);
	u8 ReadWRAM_N(const u16& _addr);
	u8 ReadOAM(const u16& _addr);
	u8 ReadIO(const u16& _addr);
	u8 ReadHRAM(const u16& _addr);
	u8 ReadIE();

	void WriteVRAM_N(const u8& _data, const u16& _addr);
	void WriteRAM_N(const u8& _data, const u16& _addr);
	void WriteWRAM_0(const u8& _data, const u16& _addr);
	void WriteWRAM_N(const u8& _data, const u16& _addr);
	void WriteOAM(const u8& _data, const u16& _addr);
	void WriteIO(const u8& _data, const u16& _addr);
	void WriteHRAM(const u8& _data, const u16& _addr);
	void WriteIE(const u8& _data);

	// io registers getter
	u8 ReadVRAMSelect();
	u8 ReadWRAMSelect();
	

	// bank selects
	int romBank = 0;
	int ramBank = 0;
	void SetRomBank(const u8& _bank);
	void SetRamBank(const u8& _bank);
	u8 GetRamBank();
	u8 GetRomBank();


private:
	// constructor
	explicit MemorySM83(const Cartridge& _cart_obj);
	static MemorySM83* instance;
	// destructor
	~MemorySM83() = default;

	// members
	void InitMemory(const Cartridge& _cart_obj) override;
	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) override;
	bool CopyRom(const std::vector<u8>& _vec_rom) override;

	void AllocateMemory() override;
	void CleanupMemory() override;

	bool isCgb = false;

	// actual memory
	u8* ROM_0;
	u8** ROM_N;
	u8** VRAM_N;
	u8** RAM_N;
	u8* WRAM_0;
	u8** WRAM_N;
	u8* OAM;
	u8* HRAM;
	u8 IE = 0;

	// IO *****************
	u8 GetIOValue(const u16& _addr);
	void SetIOValue(const u8& _data, const u16& _addr);
	void VRAM_DMA();
	void OAM_DMA();

	// CGB IO registers mapped to IO array for direct access
	// SPEED SWITCH
	u8 SPEEDSWITCH;
	// VRAM BANK SELECT
	u8 VRAM_BANK = 0;
	// LCD VRAM DMA ADDRESS SOURCE
	u8 HDMA1;
	u8 HDMA2;
	// LCD VRAM DMA ADDRESS DESTINATION
	u8 HDMA3;
	u8 HDMA4;
	// VRAM DMA length/mode/start
	u8 HDMA5;
	// OBJECT PRIORITY MODE
	u8 OBJ_PRIO;
	// WRAM BANK SELECT
	u8 WRAM_BANK = 0;

	// IO registers mapped to IO array for direct access

};
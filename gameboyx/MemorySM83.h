#pragma once

#include "Cartridge.h"
#include "MemoryBase.h"

#include "defs.h"

struct machine_state_context {
	u8 IE = 0x00;
	u8 IF = 0x00;
	int currentSpeed = 1;
	int wramBank = 0;
	int romBank = 0;
	int ramBank = 0;
	bool isCgb = false;
};

struct graphics_context {
	// LCD Control
	u8 LCDC = 0;

	// LCD Status
	u8 LY = 0;
	u8 LYC = 0;
	u8 STAT = 0;

	// Scrolling
	u8 SCY = 0;
	u8 SCX = 0;
	u8 WY = 0;
	u8 WX = 0;

	// Palettes (monochrome)
	u8 BGP = 0;
	u8 OBP0 = 0;
	u8 OBP1 = 0;
	// Palettes (color)
	u8 BCPS_BGPI = 0;
	u8 BCPD_BGPD = 0;
	u8 OCPS_OBPI = 0;
	u8 OCPD_OBPD = 0;
};

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

	machine_state_context* GetMachineContext() const;

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

	// actual memory
	u8* ROM_0;
	u8** ROM_N;
	u8** VRAM_N;
	u8** RAM_N;
	u8* WRAM_0;
	u8** WRAM_N;
	u8* OAM;
	u8* HRAM;

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
	u8 WRAM_BANK = 1;

	// IO registers
	//u8 

	// speed switch
	void SwitchSpeed(const u8& _data);

	// obj prio
	void SetObjPrio(const u8& _data);

	// memory cpu context
	machine_state_context* machine_ctx = new machine_state_context();
};
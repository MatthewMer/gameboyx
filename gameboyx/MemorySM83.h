#pragma once

#include "Cartridge.h"
#include "MemoryBase.h"
#include "gameboy_defines.h"
#include "defs.h"

struct machine_state_context {
	// interrupt
	u8 IE = 0x00;
	u8 IF = 0x00;

	// speed switch
	int currentSpeed = 1;

	// bank selects
	int wram_bank_selected = 0;
	int rom_bank_selected = 0;
	int ram_bank_selected = 0;

	// hardware
	bool isCgb = false;

	// timers
	u8 DIV = 0;
	int machineCyclesPerDIVIncrement = 0;
	int machineCyclesDIVCounter = 0;
	u8 TIMA = 0;
	int machineCyclesPerTIMAIncrement = 0;
	int machineCyclesTIMACounter = 0;
	u8 TMA = 0;
	u8 TAC = 0;

	int rom_bank_num = 0;
	int ram_bank_num = 0;
	int wram_bank_num = 0;
};

struct graphics_context {
	// hardware
	bool isCgb = false;

	// VRAM/OAM
	u8** VRAM_N;
	u8* OAM;

	// VRAM BANK SELECT
	u8 VRAM_BANK = 0;
	int vram_bank_num = 0;

	// TODO: chekc initial register states
	// LCD Control
	u8 LCDC = PPU_LCDC_ENABLE;

	// LCD Status
	u8 LY = 0;
	u8 LY_COPY = 0;
	u8 LYC = 0;
	u8 STAT = PPU_STAT_MODE1_VBLANK;

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

	// object prio mode
	u8 OBJ_PRIO;
};

struct sound_context {
	// hardware
	//bool isCgb = false;

	u8 NR10 = 0;
	u8 NR11 = 0;
	u8 NR12 = 0;
	u8 NR13 = 0;
	u8 NR14 = 0;

	u8 NR21 = 0;
	u8 NR22 = 0;
	u8 NR23 = 0;
	u8 NR24 = 0;

	u8 NR30 = 0;
	u8 NR31 = 0;
	u8 NR32 = 0;
	u8 NR33 = 0;
	u8 NR34 = 0;

	u8 NR41 = 0;
	u8 NR42 = 0;
	u8 NR43 = 0;
	u8 NR44 = 0;

	u8 NR50 = 0;
	u8 NR51 = 0;
	u8 NR52 = 0;
	
	u8* WAVE_RAM;
};

struct joypad_context {
	u8 JOYP_P1 = 0;
};

class MemorySM83 : private MemoryBase
{
public:
	// get/reset instance
	static MemorySM83* getInstance();
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
	graphics_context* GetGraphicsContext() const;
	sound_context* GetSoundContext() const;

	void RequestInterrupts(const u8& isr_flags) override;

private:
	// constructor
	MemorySM83() {
		InitMemory(*Cartridge::getInstance());
	};
	static MemorySM83* instance;
	// destructor
	~MemorySM83() = default;

	// members
	void InitMemory(const Cartridge& _cart_obj) override;
	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) override;
	bool CopyRom(const std::vector<u8>& _vec_rom) override;
	void InitTimers();
	void ProcessTAC();

	void AllocateMemory() override;
	void CleanupMemory() override;

	// actual memory
	u8* ROM_0;
	u8** ROM_N;
	u8** RAM_N;
	u8* WRAM_0;
	u8** WRAM_N;
	u8* HRAM;
	u8* IO;

	// IO *****************
	u8 GetIOValue(const u16& _addr);
	void SetIOValue(const u8& _data, const u16& _addr);
	void VRAM_DMA();
	void OAM_DMA();

	// CGB IO registers mapped to IO array for direct access
	// SPEED SWITCH
	u8 SPEEDSWITCH = 0;
	// LCD VRAM DMA ADDRESS SOURCE
	u8 HDMA1 = 0;
	u8 HDMA2 = 0;
	// LCD VRAM DMA ADDRESS DESTINATION
	u8 HDMA3 = 0;
	u8 HDMA4 = 0;
	// VRAM DMA length/mode/start
	u8 HDMA5 = 0;
	
	// WRAM BANK SELECT
	u8 WRAM_BANK = 1;

	// OAM DMA
	u8 OAM_DMA_REG = 0;

	// speed switch
	void SwitchSpeed(const u8& _data);

	// obj prio
	void SetObjPrio(const u8& _data);

	// memory cpu context
	machine_state_context* machine_ctx = new machine_state_context();
	graphics_context* graphics_ctx = new graphics_context();
	sound_context* sound_ctx = new sound_context();
	joypad_context* joyp_ctx = new joypad_context();
};
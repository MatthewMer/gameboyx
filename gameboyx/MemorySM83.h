#pragma once

#include "Cartridge.h"
#include "MemoryBase.h"
#include "gameboy_defines.h"
#include "defs.h"
#include "information_structs.h"

enum memory_areas {
	ENUM_ROM_N,
	ENUM_VRAM_N,
	ENUM_RAM_N,
	ENUM_WRAM_N,
	ENUM_OAM,
	ENUM_IO,
	ENUM_HRAM,
	ENUM_IE
};

struct machine_state_context {
	// interrupt
	u8 IE;
	u8* IF;

	// speed switch
	int currentSpeed = 1;
	bool speed_switch_requested = false;

	// bank selects
	int wram_bank_selected = 0;
	int rom_bank_selected = 0;
	int ram_bank_selected = 0;

	// hardware
	bool isCgb = false;

	// timers
	u8* DIV;
	int machineCyclesPerDIVIncrement = 0;
	int machineCyclesDIVCounter = 0;
	u8* TIMA;
	int machineCyclesPerTIMAIncrement = 0;
	int machineCyclesTIMACounter = 0;
	u8* TMA;
	u8* TAC;

	int rom_bank_num = 0;
	int ram_bank_num = 0;
	int wram_bank_num = 0;
};

struct graphics_context {
	// hardware
	bool isCgb = false;

	// VRAM/OAM
	std::vector<std::vector<u8>> VRAM_N;
	std::vector<u8> OAM;

	// VRAM BANK SELECT
	u8* VRAM_BANK;
	int vram_bank_num = 0;

	// TODO: chekc initial register states
	// LCD Control
	u8* LCDC;

	u16 bg_tilemap_offset = 0;
	u16 win_tilemap_offset = 0;
	bool obj_size_16 = false;
	bool obj_enable = false;
	bool bg_win_enable = false;
	bool bg_win_8800_addr_mode = false;
	bool win_enable = false;
	bool ppu_enable = false;

	// LCD Status
	u8* LY;
	u8* LYC;
	u8* STAT;

	// Scrolling
	u8* SCY;
	u8* SCX;
	u8* WY;
	u8* WX;

	// Palettes (monochrome)
	u8* BGP;
	u8* OBP0;
	u8* OBP1;
	// Palettes (color)
	u8* BCPS_BGPI;
	u8* BCPD_BGPD;
	u8* OCPS_OBPI;
	u8* OCPD_OBPD;

	// object prio mode
	u8* OBJ_PRIO;
};

struct sound_context {
	// hardware
	//bool isCgb = false;

	u8* NR10;
	u8* NR11;
	u8* NR12;
	u8* NR13;
	u8* NR14;

	u8* NR21;
	u8* NR22;
	u8* NR23;
	u8* NR24;

	u8* NR30;
	u8* NR31;
	u8* NR32;
	u8* NR33;
	u8* NR34;

	u8* NR41;
	u8* NR42;
	u8* NR43;
	u8* NR44;

	u8* NR50;
	u8* NR51;
	u8* NR52;

	u8* WAVE_RAM;
};

struct serial_context {
	u8* SB;
	u8* SC;
};

struct joypad_context {
	u8* JOYP_P1;
};

class MemorySM83 : private MemoryBase
{
public:
	// get/reset instance
	static MemorySM83* getInstance(machine_information& _machine_info);
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

	machine_state_context* GetMachineContext();
	graphics_context* GetGraphicsContext();
	sound_context* GetSoundContext();

	void RequestInterrupts(const u8& isr_flags) override;

private:
	// constructor
	explicit MemorySM83(machine_information& _machine_info) : MemoryBase(_machine_info) {
		InitMemory();
	};
	static MemorySM83* instance;
	// destructor
	~MemorySM83() = default;

	bool isCgb = false;

	// members
	void InitMemory() override;
	void InitMemoryState() override;
	void SetupDebugMemoryAccess() override;

	void SetIOReferences();

	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) override;
	bool CopyRom(const std::vector<u8>& _vec_rom) override;
	void InitTimers();
	void ProcessTAC();

	void AllocateMemory() override;

	// actual memory
	std::vector<u8> ROM_0;
	std::vector<std::vector<u8>> ROM_N;
	std::vector<std::vector<u8>> RAM_N;
	std::vector<u8> WRAM_0;
	std::vector<std::vector<u8>> WRAM_N;
	std::vector<u8> HRAM;
	std::vector<u8> IO;

	// IO *****************
	u8 GetIOValue(const u16& _addr);
	void SetIOValue(const u8& _data, const u16& _addr);
	void VRAM_DMA(const u8& _data);
	void OAM_DMA();

	// CGB IO registers mapped to IO array for direct access
	// SPEED SWITCH
	u8* SPEEDSWITCH;
	// LCD VRAM DMA ADDRESS SOURCE
	u8* HDMA1;
	u8* HDMA2;
	// LCD VRAM DMA ADDRESS DESTINATION
	u8* HDMA3;
	u8* HDMA4;
	// VRAM DMA length/mode/start
	u8* HDMA5;

	// WRAM BANK SELECT
	u8* WRAM_BANK;

	// OAM DMA
	u8* OAM_DMA_REG;

	// speed switch
	void SwitchSpeed(const u8& _data);

	// obj prio
	void SetObjPrio(const u8& _data);

	// action for LCDC write
	void SetLCDCValues(const u8& _data);

	void SetVRAMBank(const u8& _data);
	void SetWRAMBank(const u8& _data);

	// memory cpu context
	machine_state_context machine_ctx = machine_state_context();
	graphics_context graphics_ctx = graphics_context();
	sound_context sound_ctx = sound_context();
	joypad_context joyp_ctx = joypad_context();
	serial_context serial_ctx = serial_context();
};
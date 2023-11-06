#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The MemorySM83 memory class.
*	This class is purely there to emulate this memory type.
*/

#include "Cartridge.h"
#include "MemoryBase.h"
#include "gameboy_defines.h"
#include "defs.h"
#include "information_structs.h"

struct machine_state_context {
	// interrupt
	u8 IE;

	// speed switch
	int currentSpeed = 1;
	bool speed_switch_requested = false;

	// bank selects
	int wram_bank_selected = 0;
	int rom_bank_selected = 0;
	int ram_bank_selected = 0;
	int vram_bank_selected = 0;

	// hardware
	bool isCgb = false;

	// timers
	u8 div_low_byte = 0x00;
	u16 timaDivMask = 0x0000;
	bool tima_reload_cycle = false;
	bool tima_overflow_cycle = false;
	bool tima_reload_if_write = false;

	int rom_bank_num = 0;
	int ram_bank_num = 0;
	int wram_bank_num = 0;
	int vram_bank_num = 0;
};

struct graphics_context {
	// hardware
	bool isCgb = false;

	// VRAM/OAM
	std::vector<std::vector<u8>> VRAM_N;
	std::vector<u8> OAM;

	// TODO: chekc initial register states
	// LCD Control

	u16 bg_tilemap_offset = 0;
	u16 win_tilemap_offset = 0;
	bool obj_size_16 = false;
	bool obj_enable = false;
	bool bg_win_enable = false;
	bool bg_win_8800_addr_mode = false;
	bool win_enable = false;
	bool ppu_enable = false;
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

	// overload for MBC1
	u8 ReadROM_0(const u16& _addr, const int& _bank);

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

	// direct hardware access
	void RequestInterrupts(const u8& _isr_flags) override;
	u8& GetIORef(const u16& _addr);

	std::vector<std::pair<int, std::vector<u8>>> GetProgramData() const override;

	// actual memory
	std::vector<u8> ROM_0;
	std::vector<std::vector<u8>> ROM_N;
	std::vector<std::vector<u8>> RAM_N;
	std::vector<u8> WRAM_0;
	std::vector<std::vector<u8>> WRAM_N;
	std::vector<u8> HRAM;
	std::vector<u8> IO;

private:
	// constructor
	explicit MemorySM83(machine_information& _machine_info) : MemoryBase(_machine_info) {};
	static MemorySM83* instance;
	// destructor
	~MemorySM83() = default;

	bool isCgb = false;

	// members
	void InitMemory() override;
	void InitMemoryState() override;
	void SetupDebugMemoryAccess() override;

	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) override;
	bool CopyRom(const std::vector<u8>& _vec_rom) override;
	void InitTimers();
	void ProcessTAC();

	void AllocateMemory() override;

	// IO *****************
	void WriteIORegister(const u8& _data, const u16& _addr);
	u8 ReadIORegister(const u16& _addr);
	void VRAM_DMA(const u8& _data);
	void OAM_DMA();

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
};
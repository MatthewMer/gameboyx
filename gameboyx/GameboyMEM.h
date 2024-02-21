#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The GameboyMEM memory class.
*	This class is purely there to emulate this memory type.
*/

#include "GameboyCartridge.h"
#include "BaseMEM.h"
#include "gameboy_defines.h"
#include "defs.h"

struct machine_context {
	std::string title = "";
	bool battery_buffered = false;
	bool ram_present = false;
	bool timer_present = false;

	// interrupt
	u8 IE = 0x00;

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
	u8 apuDivMask = 0x10;

	int rom_bank_num = 0;
	int ram_bank_num = 0;
	int wram_bank_num = 0;
	int vram_bank_num = 0;
};

struct graphics_context {
	// VRAM/OAM
	std::vector<std::vector<u8>> VRAM_N;
	std::vector<u8> OAM;

	bool vblank_if_write = false;

	// TODO: chekc initial register states
	// LCD Control

	// LCDC
	// bit 0 DMG
	bool bg_win_enable = false;
	// bit 0 CGB
	bool obj_prio = false;
	// bit 1
	bool obj_enable = false;
	// bit 2
	bool obj_size_16 = false;
	// bit 3
	u16 bg_tilemap_offset = 0;
	// bit 4
	bool bg_win_addr_mode_8000 = false;
	// bit 5
	bool win_enable = false;
	// bit 6
	u16 win_tilemap_offset = 0;
	// bit 7
	bool ppu_enable = false;

	// LCD STAT
	// bit 0-1
	int mode = PPU_MODE_1;
	// bit 2
	//bool lyc_ly_flag = false;
	// bit 3
	bool mode_0_int_sel = false;
	// bit 4
	bool mode_1_int_sel = false;
	// bit 5
	bool mode_2_int_sel = false;
	// bit 6
	bool lyc_ly_int_sel = false;

	u32 dmg_bgp_color_palette[4] = {
		DMG_COLOR_WHITE_ALT,
		DMG_COLOR_LIGHTGREY_ALT,
		DMG_COLOR_DARKGREY_ALT,
		DMG_COLOR_BLACK_ALT
	};

	u32 dmg_obp0_color_palette[4] = {
		DMG_COLOR_WHITE_ALT,
		DMG_COLOR_LIGHTGREY_ALT,
		DMG_COLOR_DARKGREY_ALT,
		DMG_COLOR_BLACK_ALT
	};

	u32 dmg_obp1_color_palette[4] = {
		DMG_COLOR_WHITE_ALT,
		DMG_COLOR_LIGHTGREY_ALT,
		DMG_COLOR_DARKGREY_ALT,
		DMG_COLOR_BLACK_ALT
	};

	// CGB
	bool obj_prio_mode_cgb = false;

	u8 cgb_obp_palette_ram[PPU_PALETTE_RAM_SIZE_CGB] = {};
	u8 cgb_bgp_palette_ram[PPU_PALETTE_RAM_SIZE_CGB] = {};

	u32 cgb_obp_color_palettes[8][4] = {
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT}
	};

	u32 cgb_bgp_color_palettes[8][4] = {
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT},
		{DMG_COLOR_WHITE_ALT, DMG_COLOR_LIGHTGREY_ALT, DMG_COLOR_DARKGREY_ALT, DMG_COLOR_BLACK_ALT}
	};
};

inline const std::unordered_map<u8, float> VOLUME_MAP = {
	{0, 1.f / 8},
	{1, 2.f / 8},
	{2, 3.f / 8},
	{3, 4.f / 8},
	{4, 5.f / 8},
	{5, 6.f / 8},
	{6, 7.f / 8},
	{7, 8.f / 8}
};

struct sound_context {
	// master control	NR52
	bool apuEnable = true;					// set by CPU
	bool ch1Enable = false;					// set by APU
	bool ch2Enable = false;					// as before
	bool ch3Enable = false;					// ...
	bool ch4Enable = false;

	// channel panning	NR51
	// Bit order:
	// CH1 right
	// CH2 right
	// CH3 right
	// CH4 right
	// CH1 left
	// CH2 left
	// CH3 left
	// CH4 left
	bool channelPanning[8] = {};				

	// master volume	NR50
	float masterVolumeRight = 1.f;
	float masterVolumeLeft = 1.f;
	bool outRightEnabled = false;
	bool outLeftEnabled = false;

	// channel 1
	// sweep				NR10
	int ch1Pace = 0;
	bool ch1DirSubtract = false;
	int ch1PeriodStep = 0;
	// timer, duty cycle	NR11
	int ch1LengthTimer = 0;
	int ch1DutyCycleIndex = 0;

	//u8 divApuBitMask = DIV_APU_SINGLESPEED_BIT;
	//u8 divApuCounter = 0;
	//bool divApuBitWasHigh = false;
	//bool divApuBitHigh = false;
};

struct control_context {
	bool buttons_selected = false;
	bool dpad_selected = false;

	bool start_pressed = false;
	bool down_pressed = false;
	bool select_pressed = false;
	bool up_pressed = false;
	bool b_pressed = false;
	bool left_pressed = false;
	bool a_pressed = false;
	bool right_pressed = false;
};

class GameboyMEM : private BaseMEM
{
public:
	friend class BaseMEM;

	// clone/assign protection
	GameboyMEM(GameboyMEM const&) = delete;
	GameboyMEM(GameboyMEM&&) = delete;
	GameboyMEM& operator=(GameboyMEM const&) = delete;
	GameboyMEM& operator=(GameboyMEM&&) = delete;

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

	machine_context* GetMachineContext();
	graphics_context* GetGraphicsContext();
	sound_context* GetSoundContext();
	control_context* GetControlContext();

	// direct hardware access
	void RequestInterrupts(const u8& _isr_flags) override;
	u8& GetIO(const u16& _addr);
	void SetIO(const u16& _addr, const u8& _data);
	void CopyDataToRAM(const std::vector<char>& _data);
	void CopyDataFromRAM(std::vector<char>& _data) const;

	std::vector<u8>* GetProgramData(const int& _bank) const override;

	void SetButton(const u8& _bit, const bool& _is_button);
	void UnsetButton(const u8& _bit, const bool& _is_button);

	void GetMemoryDebugTables(std::vector<Table<memory_entry>>& _tables) override;

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
	explicit GameboyMEM(BaseCartridge* _cartridge) {
		InitMemory(_cartridge);
		machine_ctx.title = _cartridge->title;
		machine_ctx.battery_buffered = _cartridge->batteryBuffered;
		machine_ctx.ram_present = _cartridge->ramPresent;
		machine_ctx.timer_present = _cartridge->timerPresent;
	};
	// destructor
	~GameboyMEM() override = default;

	// members
	void InitMemory(BaseCartridge* _cartridge) override;
	void InitMemoryState() override;
	void FillMemoryDebugTable(TableSection<memory_entry>& _table_section, std::vector<u8>* _bank_data, const int& _offset) override;

	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) override;
	bool CopyRom(const std::vector<u8>& _vec_rom) override;
	void ProcessTAC();

	void AllocateMemory() override;

	// controller
	void SetControlValues(const u8& _data);

	// IO *****************
	void WriteIORegister(const u8& _data, const u16& _addr);
	u8 ReadIORegister(const u16& _addr);
	void VRAM_DMA(const u8& _data);
	void OAM_DMA();

	// speed switch
	void SwitchSpeed(const u8& _data);

	// obj prio
	void SetObjPrio(const u8& _data);

	void SetColorPaletteValues(const u8& _data, u32* _color_palette);

	// action for LCDC write
	void SetLCDCValues(const u8& _data);
	void SetLCDSTATValues(const u8& _data);

	void SetVRAMBank(const u8& _data);
	void SetWRAMBank(const u8& _data);

	// apu
	void SetAPUMasterControl(const u8& _data);
	void SetAPUChannelPanning(const u8& _data);
	void SetAPUMasterVolume(const u8& _data);

	void SetAPUCh1Sweep(const u8& _data);
	void SetAPUCh1TimerDutyCycle(const u8& _data);

	// memory cpu context
	machine_context machine_ctx = machine_context();
	graphics_context graphics_ctx = graphics_context();
	sound_context sound_ctx = sound_context();
	control_context control_ctx = control_context();
};
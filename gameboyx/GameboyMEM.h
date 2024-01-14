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
#include "data_containers.h"

struct machine_context {
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

	u8 cgb_obp_palette_ram[PPU_PALETTE_RAM_SIZE_CGB];
	u8 cgb_bgp_palette_ram[PPU_PALETTE_RAM_SIZE_CGB];

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

struct sound_context {

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
	// get/reset instance
	static GameboyMEM* getInstance(machine_information& _machine_info);
	static GameboyMEM* getInstance();
	static void resetInstance();

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

	std::vector<std::pair<int, std::vector<u8>>> GetProgramData() const override;

	void SetButton(const u8& _bit, const bool& _is_button);
	void UnsetButton(const u8& _bit, const bool& _is_button);

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
	explicit GameboyMEM(machine_information& _machine_info) : BaseMEM(_machine_info) {};
	static GameboyMEM* instance;
	// destructor
	~GameboyMEM() = default;

	// members
	void InitMemory() override;
	void InitMemoryState() override;
	void SetupDebugMemoryAccess() override;

	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) override;
	bool CopyRom(const std::vector<u8>& _vec_rom) override;
	void InitTimers();
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

	// memory cpu context
	machine_context machine_ctx = machine_context();
	graphics_context graphics_ctx = graphics_context();
	sound_context sound_ctx = sound_context();
	control_context control_ctx = control_context();
};
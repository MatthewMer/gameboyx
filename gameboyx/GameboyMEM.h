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
#include <atomic>
#include <mutex>
#include <vector>

enum MEM_TYPE {
	ROM0 = 0,
	ROMn = 1,
	RAMn = 2,
	WRAM0 = 3,
	WRAMn = 4
};

struct machine_context {
	std::string title = "";
	bool battery_buffered = false;
	bool ram_present = false;
	bool timer_present = false;

	// interrupt
	u8 IE = 0x00;

	bool halted = false;
	bool stopped = false;

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
	u8 apuDivMask = APU_DIV_BIT_SINGLESPEED;		// singlespeedmode

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

	u32 dmg_bgp_color_palette[4];
	u32 dmg_obp0_color_palette[4];
	u32 dmg_obp1_color_palette[4];

	// CGB
	bool obj_prio_mode_cgb = false;

	bool obp_increment = false;
	bool bgp_increment = false;

	u8 cgb_obp_palette_ram[PPU_PALETTE_RAM_SIZE_CGB] = {};
	u8 cgb_bgp_palette_ram[PPU_PALETTE_RAM_SIZE_CGB] = {};

	u32 cgb_obp_color_palettes[8][4] = {
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE}
	};
	u32 cgb_bgp_color_palettes[8][4] = {
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE},
		{CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE, CGB_DMG_COLOR_WHITE}
	};

	bool dma_hblank = false;
	u16 dma_source_addr = 0;
	u16 dma_dest_addr = 0;
	MEM_TYPE dma_source_mem = ROM0;
	bool dma_hblank_ppu_en = false;
};

inline const std::unordered_map<u8, float> VOLUME_MAP = {
	{0, 1.f / 8},
	{1, 2.f / 8},
	{2, 3.f / 8},
	{3, 4.f / 8},
	{4, 5.f / 8},
	{5, 6.f / 8},
	{6, 7.f / 8},
	{7, 8.f / 8},
	{8, .0f}			// usually not used, just for Channel 3 (muted)
};

struct sound_context {
	// master control	NR52
	bool apuEnable = true;					// set by CPU
	bool ch1EnableFlag = false;				// set by APU
	bool ch2EnableFlag = false;				// as before
	bool ch3EnableFlag = false;				// ...
	bool ch4EnableFlag = false;

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
	alignas(64) std::atomic<bool> ch1Right = false;
	alignas(64) std::atomic<bool> ch2Right = false;
	alignas(64) std::atomic<bool> ch3Right = false;
	alignas(64) std::atomic<bool> ch4Right = false;
	alignas(64) std::atomic<bool> ch1Left = false;
	alignas(64) std::atomic<bool> ch2Left = false;
	alignas(64) std::atomic<bool> ch3Left = false;
	alignas(64) std::atomic<bool> ch4Left = false;

	// master volume	NR50
	alignas(64) std::atomic<float> masterVolumeRight = 1.f;
	alignas(64) std::atomic<float> masterVolumeLeft = 1.f;
	alignas(64) std::atomic<bool> outRightEnabled = true;
	alignas(64) std::atomic<bool> outLeftEnabled = true;

	// channel 1
	// sweep						NR10
	int ch1SweepPace = 0;
	bool ch1SweepDirSubtract = false;
	int ch1SweepPeriodStep = 0;
	// timer, duty cycle			NR11
	int ch1LengthTimer = 0;
	bool ch1LengthAltered = false;
	alignas(64) std::atomic<int> ch1DutyCycleIndex = 0;
	// envelope						NR12
	int ch1EnvelopeVolume = 0;
	bool ch1EnvelopeIncrease = false;
	int ch1EnvelopePace = 0;
	alignas(64) std::atomic<float> ch1Volume = .0f;
	// period						NR13
	int ch1Period = 0;
	alignas(64) std::atomic<float> ch1SamplingRate = 1.f;

	// period + ctrl				NR14
	bool ch1LengthEnable = false;
	alignas(64) std::atomic<bool> ch1Enable = false;
	bool ch1DAC = false;

	// channel 2
	// timer, duty cycle			NR21
	int ch2LengthTimer = 0;
	bool ch2LengthAltered = false;
	alignas(64) std::atomic<int> ch2DutyCycleIndex = 0;
	// envelope						NR22
	int ch2EnvelopeVolume = 0;
	bool ch2EnvelopeIncrease = false;
	int ch2EnvelopePace = 0;
	alignas(64) std::atomic<float> ch2Volume = .0f;
	// period						NR13
	int ch2Period = 0;
	alignas(64) std::atomic<float> ch2SamplingRate = 1.f;

	// period + ctrl				NR14
	bool ch2LengthEnable = false;
	alignas(64) std::atomic<bool> ch2Enable = false;
	bool ch2DAC = false;

	// channel 3
	// control						NR30
	// see NR34
	// length						NR31
	int ch3LengthTimer = 0;
	bool ch3LengthAltered = false;
	// volume						NR32
	alignas(64) std::atomic<float> ch3Volume = 1.f;
	// period low					NR33
	int ch3Period = 0;
	alignas(64) std::atomic<float> ch3SamplingRate = 1.f;
	// period high and control		NR34
	bool ch3LengthEnable = false;
	alignas(64) std::atomic<bool> ch3Enable = false;
	bool ch3DAC = false;
	// Ch3 Wave RAM
	std::mutex mutWaveRam;
	float waveRam[32] = {
		.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f,
		.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f,
		.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f,
		.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f
	};

	// channel 4
	// length						NR41
	int ch4LengthTimer = 0;
	bool ch4LengthAltered = false;
	// envelope						NR42
	int ch4EnvelopeVolume = 0;
	bool ch4EnvelopeIncrease = false;
	int ch4EnvelopePace = 0;
	alignas(64) std::atomic<float> ch4Volume = 1.f;
	// frequency and randomness		NR43
	bool ch4LFSRWidth7Bit = false;
	u16 ch4LFSR = CH_4_LFSR_INIT_VALUE;
	float ch4LFSRStep = .0f;
	alignas(64) std::atomic<float> ch4SamplingRate = (float)CH_4_LFSR_MAX_SAMPL_RATE;
	//  and control					NR44
	bool ch4LengthEnable = false;
	alignas(64) std::atomic<bool> ch4Enable = false;
	bool ch4DAC = false;
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
	void CopyDataFromRAM(std::vector<char>& _data);

	std::vector<u8>* GetProgramData(const int& _bank) const override;

	void SetButton(const u8& _bit, const bool& _is_button);
	void UnsetButton(const u8& _bit, const bool& _is_button);

	void GetMemoryDebugTables(std::vector<Table<memory_entry>>& _tables) override;

	const std::vector<u8>* GetBank(const MEM_TYPE& _type, const int& _bank);

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

		if (machine_ctx.isCgb) {
			graphics_ctx.dmg_bgp_color_palette[0] = CGB_DMG_COLOR_WHITE;
			graphics_ctx.dmg_bgp_color_palette[1] = CGB_DMG_COLOR_LIGHTGREY;
			graphics_ctx.dmg_bgp_color_palette[2] = CGB_DMG_COLOR_DARKGREY;
			graphics_ctx.dmg_bgp_color_palette[3] = CGB_DMG_COLOR_BLACK;
		} else {
			graphics_ctx.dmg_bgp_color_palette[0] = DMG_COLOR_WHITE_ALT;
			graphics_ctx.dmg_bgp_color_palette[1] = DMG_COLOR_LIGHTGREY_ALT;
			graphics_ctx.dmg_bgp_color_palette[2] = DMG_COLOR_DARKGREY_ALT;
			graphics_ctx.dmg_bgp_color_palette[3] = DMG_COLOR_BLACK_ALT;
			graphics_ctx.dmg_obp0_color_palette[0] = DMG_COLOR_WHITE_ALT;
			graphics_ctx.dmg_obp0_color_palette[1] = DMG_COLOR_LIGHTGREY_ALT;
			graphics_ctx.dmg_obp0_color_palette[2] = DMG_COLOR_DARKGREY_ALT;
			graphics_ctx.dmg_obp0_color_palette[3] = DMG_COLOR_BLACK_ALT;
			graphics_ctx.dmg_obp1_color_palette[0] = DMG_COLOR_WHITE_ALT;
			graphics_ctx.dmg_obp1_color_palette[1] = DMG_COLOR_LIGHTGREY_ALT;
			graphics_ctx.dmg_obp1_color_palette[2] = DMG_COLOR_DARKGREY_ALT;
			graphics_ctx.dmg_obp1_color_palette[3] = DMG_COLOR_BLACK_ALT;
		}
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
	void OAM_DMA(const u8& _data);

	// speed switch
	void SwitchSpeed(const u8& _data);

	// obj prio
	void SetObjPrio(const u8& _data);

	void SetColorPaletteValues(const u8& _data, u32* _color_palette);

	void SetBGWINPaletteValues(const u8& _data);
	void SetOBJPaletteValues(const u8& _data);
	void SetBCPS(const u8& _data);
	void SetOCPS(const u8& _data);

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
	void SetAPUCh1Envelope(const u8& _data);
	void SetAPUCh1PeriodLow(const u8& _data);
	void SetAPUCh1PeriodHighControl(const u8& _data);

	void SetAPUCh2TimerDutyCycle(const u8& _data);
	void SetAPUCh2Envelope(const u8& _data);
	void SetAPUCh2PeriodLow(const u8& _data);
	void SetAPUCh2PeriodHighControl(const u8& _data);

	void SetAPUCh3DACEnable(const u8& _data);
	void SetAPUCh3Timer(const u8& _data);
	void SetAPUCh3Volume(const u8& _data);
	void SetAPUCh3PeriodLow(const u8& _data);
	void SetAPUCh3PeriodHighControl(const u8& _data);
	void SetAPUCh3WaveRam(const u16& _addr, const u8& _data);

	void SetAPUCh4Timer(const u8& _data);
	void SetAPUCh4Envelope(const u8& _data);
	void SetAPUCh4FrequRandomness(const u8& _data);
	void SetAPUCh4Control(const u8& _data);

	// memory cpu context
	machine_context machine_ctx = machine_context();
	graphics_context graphics_ctx = graphics_context();
	sound_context sound_ctx = sound_context();
	control_context control_ctx = control_context();
};
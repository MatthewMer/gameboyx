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
#include <format>

namespace Emulation {
	namespace Gameboy {
		struct machine_context {
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
			bool is_cgb = false;
			bool cgb_compatibility = false;

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

			bool boot_rom_mapped = false;
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

			bool vram_dma = false;
			u16 vram_dma_src_addr = 0;
			u16 vram_dma_dst_addr = 0;
			MEM_TYPE vram_dma_mem = MEM_TYPE::ROM0;
			bool vram_dma_ppu_en = false;

			bool oam_dma = false;
			u16 oam_dma_src_addr = 0;
			MEM_TYPE oam_dma_mem = MEM_TYPE::ROM0;
			int oam_dma_counter = 0;
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

		struct ch_registers {
			u16 nrX0 = 0;
			u16 nrX1 = 0;
			u16 nrX2 = 0;
			u16 nrX3 = 0;
			u16 nrX4 = 0;

			ch_registers() {};

			ch_registers(u16 nrX0_addr, u16 nrX1_addr, u16 nrX2_addr, u16 nrX3_addr, u16 nrX4_addr)
				: nrX0(nrX0_addr), nrX1(nrX1_addr), nrX2(nrX2_addr), nrX3(nrX3_addr), nrX4(nrX4_addr)
			{}

			ch_registers(u16 nrX1_addr, u16 nrX2_addr, u16 nrX3_addr, u16 nrX4_addr)
				: nrX1(nrX1_addr), nrX2(nrX2_addr), nrX3(nrX3_addr), nrX4(nrX4_addr) 
			{}
		};

		enum CH_EXT_TYPE {
			ENVELOPE,
			PERIOD,
			PWM,
			WAVE_RAM,
			LFSR
		};

		struct ch_extension {
			virtual ~ch_extension() {};
		};

		struct ch_ext_envelope : ch_extension {
			int envelope_volume = 0;							// envelope NR12, NR22, ...
			bool envelope_increase = false;
			int envelope_pace = 0;

			~ch_ext_envelope() = default;
		};

		struct ch_ext_period : ch_extension {
			int sweep_pace = 0;									// sweep NR10
			bool sweep_dir_subtract = false;
			int sweep_period_step = 0;

			~ch_ext_period() = default;
		};

		struct ch_ext_pwm : ch_extension {
			alignas(64) std::atomic<int> duty_cycle_index = 0;

			~ch_ext_pwm() = default;
		};

		struct ch_ext_waveram : ch_extension {
			std::mutex mut_wave_ram;							// wave RAM
			float wave_ram[32] = {
				.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f,
				.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f,
				.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f,
				.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f
			};
			float sample_count = 0;

			~ch_ext_waveram() = default;
		};

		struct ch_ext_lfsr : ch_extension {
			bool lfsr_width_7bit = false;						// frequency and randomness NR43
			u16 lfsr = CH_4_LFSR_INIT_VALUE;
			float lfsr_step = .0f;

			~ch_ext_lfsr() = default;
		};

		struct channel_context {
			ch_registers regs;

			alignas(64) std::atomic<bool> enable = false;		// set by APU

			alignas(64) std::atomic<bool> right = false;		// channel panning	NR51
			alignas(64) std::atomic<bool> left = false;

			int length_timer = 0;								// length NR11, NR21, ...
			bool length_altered = false;
			bool length_enable = false;							// ctrl NR14, ...

			alignas(64) std::atomic<float> volume = .0f;

			alignas(64) std::atomic<float> sampling_rate = 1.f;

			bool dac = false;

			u8 enable_bit = 0x00;

			int period = 0;

			std::unordered_map<CH_EXT_TYPE, std::unique_ptr<ch_extension>> exts;

			channel_context() {};
			channel_context(int _channel) {
				switch (_channel) {
				case 1:
					regs = ch_registers(NR10_ADDR, NR11_ADDR, NR12_ADDR, NR13_ADDR, NR14_ADDR);
					exts[ENVELOPE]	= std::unique_ptr<ch_extension>(new ch_ext_envelope());
					exts[PERIOD]	= std::unique_ptr<ch_extension>(new ch_ext_period());
					exts[PWM]		= std::unique_ptr<ch_extension>(new ch_ext_pwm());
					enable_bit = CH_1_ENABLE;
					break;
				case 2:
					regs = ch_registers(NR21_ADDR, NR22_ADDR, NR23_ADDR, NR24_ADDR);
					exts[ENVELOPE]	= std::unique_ptr<ch_extension>(new ch_ext_envelope());
					exts[PWM]		= std::unique_ptr<ch_extension>(new ch_ext_pwm());
					enable_bit = CH_2_ENABLE;
					break;
				case 3:
					regs = ch_registers(NR30_ADDR, NR31_ADDR, NR32_ADDR, NR33_ADDR, NR34_ADDR);
					exts[WAVE_RAM]	= std::unique_ptr<ch_extension>(new ch_ext_waveram());
					enable_bit = CH_3_ENABLE;
					break;
				case 4:
					regs = ch_registers(NR41_ADDR, NR42_ADDR, NR43_ADDR, NR44_ADDR);
					exts[ENVELOPE]	= std::unique_ptr<ch_extension>(new ch_ext_envelope());
					exts[LFSR]		= std::unique_ptr<ch_extension>(new ch_ext_lfsr());
					enable_bit = CH_4_ENABLE;
					break;
				default:
					break;
				}
			};
		};


		struct sound_context {
			// master control	NR52
			bool apuEnable = true;					// set by CPU

			// master Volume	NR50
			alignas(64) std::atomic<float> masterVolumeRight = 1.f;
			alignas(64) std::atomic<float> masterVolumeLeft = 1.f;
			alignas(64) std::atomic<bool> outRightEnabled = true;
			alignas(64) std::atomic<bool> outLeftEnabled = true;

			// stack allocated array of channel contexts
			channel_context ch_ctxs[4] = {
				channel_context(1),
				channel_context(2),
				channel_context(3),
				channel_context(4)
			};

			~sound_context() {}
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

		struct serial_context {
			bool transfer_requested = false;
			u8 outgoing = 0x00;
			
			bool reveived = false;
			u8 incoming = 0x00;

			bool master = false;

			bool div_low_byte = false;
			u8 div_bit = SERIAL_NORMAL_SPEED_BIT;
		};

		class GameboyMEM : public BaseMEM {
		public:
			friend class BaseMEM;
			// constructor
			explicit GameboyMEM(std::shared_ptr<BaseCartridge> _cartridge);
			virtual ~GameboyMEM() override;
			void Init() override;

			// clone/assign protection
			//GameboyMEM(GameboyMEM const&) = delete;
			//GameboyMEM(GameboyMEM&&) = delete;
			//GameboyMEM& operator=(GameboyMEM const&) = delete;
			//GameboyMEM& operator=(GameboyMEM&&) = delete;

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

			void SetButton(const u8& _bit, const bool& _is_button);
			void UnsetButton(const u8& _bit, const bool& _is_button);

			const u8* GetBank(const MEM_TYPE& _type, const int& _bank);

			// actual memory
			std::vector<u8> ROM_0;
			std::vector<std::vector<u8>> ROM_N;
			std::vector<u8*> RAM_N;
			std::vector<u8> WRAM_0;
			std::vector<std::vector<u8>> WRAM_N;
			std::vector<u8> HRAM;
			std::vector<u8> IO;

			Backend::FileIO::FileMapper mapper;
			std::string saveFile = "";

		private:
			// members
			void InitMemory(std::shared_ptr<BaseCartridge> _cartridge) override;
			void InitMemoryState() override;

			void GenerateMemoryTables() override;
			void FillMemoryTable(std::vector<std::tuple<int, memory_entry>>& _table_section, u8* _bank_data, const int& _offset, const size_t& _size) override;

			bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) override;
			bool InitRom(const std::vector<u8>& _vec_rom) override;
			bool InitBootRom(const std::vector<u8>& _boot_rom, const std::vector<u8>& _vec_rom) override;

			void ProcessTAC();

			void AllocateMemory(std::shared_ptr<BaseCartridge> _cartridge) override;

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

			void SetColorPaletteValues(const u8& _data, u32* _color_palette, u32* _source_palette);

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
			void SetAPUCh12TimerDutyCycle(const u8& _data, channel_context* _ch_ctx);
			void SetAPUCh124Envelope(const u8& _data, channel_context* _ch_ctx);
			void SetAPUCh12PeriodLow(const u8& _data, channel_context* _ch_ctx);
			void SetAPUCh12PeriodHighControl(const u8& _data, channel_context* _ch_ctx);

			void SetAPUCh3DACEnable(const u8& _data);
			void SetAPUCh3Timer(const u8& _data);
			void SetAPUCh3Volume(const u8& _data);
			void SetAPUCh3PeriodLow(const u8& _data);
			void SetAPUCh3PeriodHighControl(const u8& _data);
			void SetAPUCh3WaveRam(const u16& _addr, const u8& _data);

			void SetAPUCh4Timer(const u8& _data);
			void SetAPUCh4FrequRandomness(const u8& _data);
			void SetAPUCh4Control(const u8& _data);

			// serial
			void SetSerialData(const u8& _data);
			void SetSerialControl(const u8& _data);

			void UnmapBootRom();

			// memory cpu context
			machine_context machineCtx = machine_context();
			graphics_context graphics_ctx = graphics_context();
			sound_context sound_ctx = sound_context();
			control_context control_ctx = control_context();
			serial_context serial_ctx = serial_context();
		};
	}
}
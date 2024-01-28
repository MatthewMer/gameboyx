#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	Different information structs (data containers) used for information passing from and to GUI without the need for an extensive usage
*	of Getters and Setters (reduce unnecessary function call overhead and complexity)
*/

#include "SDL.h"
#include <queue>
#include <string>
#include <unordered_map>

#include "defs.h"
#include "GuiScrollableTable.h"
#include "general_config.h"
#include "BaseCartridge.h"

template <class T> using MemoryBufferEntry = std::pair<std::string, std::vector<T>>;	// memory buffer: type, vector<Table>
template <class T> using MemoryBuffer = std::vector<MemoryBufferEntry<T>>;				// memory buffer: type, vector<Table>

struct machine_information {
	BaseCartridge* cartridge;

	bool instruction_debug_enabled = false;
	// index, address, raw data, resolved data
	GuiScrollableTable<debug_instr_data> program_buffer = GuiScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
	GuiScrollableTable<debug_instr_data> program_buffer_tmp = GuiScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
	std::vector<misc_output_data> register_values = std::vector<misc_output_data>();
	std::vector<misc_output_data> flag_values = std::vector<misc_output_data>();
	std::vector<misc_output_data> misc_values = std::vector<misc_output_data>();

	bool pause_execution = true;

	int current_pc = -1;
	int current_rom_bank = 0;
	std::string current_instruction = "";

	int emulation_speed = 1;

	// current hardware state
	bool track_hardware_info = false;
	float current_frequency = .0f;
	float current_framerate = .0f;
	std::vector<misc_output_data> hardware_info = std::vector<misc_output_data>();

	bool ramPresent = false;
	bool batteryBuffered = false;
	bool timerPresent = false;

	MemoryBuffer<GuiScrollableTable<memory_data>> memory_buffer = MemoryBuffer<GuiScrollableTable<memory_data>>();

	// controller
	std::unordered_map<SDL_Keycode, int> key_map = std::unordered_map<SDL_Keycode, int>();

	void reset_machine_information() {
		cartridge = nullptr;

		program_buffer = GuiScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
		program_buffer_tmp = GuiScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
		register_values = std::vector<misc_output_data>();
		flag_values = std::vector<misc_output_data>();
		misc_values = std::vector<misc_output_data>();

		pause_execution = true;

		current_pc = -1;
		current_rom_bank = 0;
		current_instruction = "";

		current_frequency = .0f;
		current_framerate = .0f;

		hardware_info = std::vector<misc_output_data>();

		ramPresent = false;
		batteryBuffered = false;
		timerPresent = false;

		memory_buffer = MemoryBuffer<GuiScrollableTable<memory_data>>();
	}
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
	bool request_reset = false;
};

struct graphics_information {
	bool shaders_compilation_finished = false;
	int shaders_total = 0;
	int shaders_compiled = 0;

	// drawing mode
	bool is2d = true;
	bool en2d = true;

	// data for gameboy output
	std::vector<u8> image_data = std::vector<u8>();
	u32 lcd_width;
	u32 lcd_height;
	float aspect_ratio = 1.f;

	// graphics backend data
	u32 win_width;
	u32 win_height;

	/*
	float ascpect_ratio = .0f;
	int x_ = 0;
	int y_ = 0;
	int texels_per_pixel = 0;
	int x_offset = 0;
	int y_offset = 0;
	*/
};

struct audio_information {
	int max_channels = 0;
	int max_sampling_rate = 0;

	float volume = 1.f;
	int channels = 0;
	int sampling_rate = 0;
};
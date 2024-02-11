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
#include "GuiTable.h"
#include "general_config.h"
#include "BaseCartridge.h"

// typedefs for table contents
using debug_instr_entry_contents = std::pair<std::string, std::string>;
using misc_output_data = std::pair<std::string, std::string>;

enum memory_data_types {
	MEM_ENTRY_ADDR,
	MEM_ENTRY_LEN,
	MEM_ENTRY_REF
};
using memory_data = std::tuple<std::string, int, u8*>;

// general data container for data regarding the machine state
struct machine_information {
	BaseCartridge* cartridge;

	bool debug_instr_enabled = false;

	Table<debug_instr_entry_contents> debug_instr_table = Table<debug_instr_entry_contents>(DEBUG_INSTR_LINES);
	Table<debug_instr_entry_contents> debug_instr_table_tmp = Table<debug_instr_entry_contents>(DEBUG_INSTR_LINES);

	std::vector<Table<memory_data>> memory_tables = std::vector<Table<memory_data>>();

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

	// controller
	std::unordered_map<SDL_Keycode, int> key_map = std::unordered_map<SDL_Keycode, int>();

	void reset_machine_information() {
		cartridge = nullptr;

		debug_instr_table = Table<debug_instr_entry_contents>(DEBUG_INSTR_LINES);
		debug_instr_table_tmp = Table<debug_instr_entry_contents>(DEBUG_INSTR_LINES);
		memory_tables = std::vector<Table<memory_data>>();

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
};

struct audio_information {
	int channels_max = 0;
	int sampling_rate_max = 0;

	float volume = 1.f;
	int channels = 0;
	int sampling_rate = 0;
};
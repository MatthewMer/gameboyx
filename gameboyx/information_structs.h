#pragma once

#include <queue>
#include <string>

#include "defs.h"
#include "ScrollableTable.h"
#include "imguigameboyx_config.h"

template <class T> using MemoryBufferEntry = std::pair<std::string, std::vector<T>>;	// memory buffer: type, vector<Table>
template <class T> using MemoryBuffer = std::vector<MemoryBufferEntry<T>>;				// memory buffer: type, vector<Table>

struct machine_information {
	// debug isntructions
	bool instruction_debug_enabled = false;
	// index, address, raw data, resolved data
	ScrollableTable<debug_instr_data> program_buffer = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
	ScrollableTable<debug_instr_data> program_buffer_tmp = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
	std::vector<register_data> register_values = std::vector<register_data>();
	std::vector<register_data> flag_values = std::vector<register_data>();
	bool instruction_logging = false;
	bool pause_execution = true;
	int current_pc = -1;
	int current_rom_bank = 0;
	std::string current_instruction = "";

	// current hardware state
	bool track_hardware_info = false;
	float current_frequency = .0f;
	float current_framerate = .0f;
	float max_framerate = .0f;
	int wram_bank_selected = 0;
	int wram_bank_num = 0;
	int ram_bank_selected = 0;
	int ram_bank_num = 0;
	int rom_bank_selected = 0;
	int rom_bank_num = 0;
	int vram_bank_selected = 0;
	int vram_bank_num = 0;
	int current_speed = 1;

	MemoryBuffer<ScrollableTable<memory_data>> memory_buffer = MemoryBuffer<ScrollableTable<memory_data>>();

	void reset_machine_information() {
		program_buffer = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
		program_buffer_tmp = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
		register_values = std::vector<register_data>();
		instruction_logging = false;
		pause_execution = true;
		current_pc = -1;
		current_rom_bank = 0;
		current_instruction = "";

		current_frequency = .0f;
		current_framerate = .0f;
		wram_bank_selected = 0;
		wram_bank_num = 0;
		ram_bank_selected = 0;
		ram_bank_num = 0;
		rom_bank_selected = 0;
		rom_bank_num = 0;
		vram_bank_selected = 0;
		vram_bank_num = 0;

		memory_buffer = MemoryBuffer<ScrollableTable<memory_data>>();
	}
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
	bool request_reset = false;
};
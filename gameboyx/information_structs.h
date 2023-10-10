#pragma once

#include <queue>
#include <string>

#include "defs.h"
#include "ScrollableTable.h"
#include "imguigameboyx_config.h"

struct machine_information {
	// debug isntructions
	bool instruction_debug_enabled = false;
	// index, address, raw data, resolved data
	ScrollableTable<debug_instr_data> program_buffer = ScrollableTable<debug_instr_data>(DEBUG_INSTR_ELEMENTS);
	std::vector<debug_instr_data> register_values = std::vector<debug_instr_data>();
	bool instruction_logging = false;
	bool pause_execution = true;
	int current_pc = 0;
	int rom_bank_size = 0;
	int current_rom_bank = 0;
	std::string current_instruction = "";

	// current hardware state
	bool track_hardware_info = false;
	float current_frequency = .0f;
	int wram_bank_selected = 0;
	int wram_bank_num = 0;
	int ram_bank_selected = 0;
	int ram_bank_num = 0;
	int rom_bank_selected = 0;
	int rom_bank_num = 0;
	int vram_bank_selected = 0;
	int vram_bank_num = 0;

	// memory access	-> <memory type, name>, number, size, base address, reference to memory
	std::vector<std::tuple<std::pair<int, std::string>, int, int, int, u8**>> debug_memory = std::vector<std::tuple<std::pair<int, std::string>, int, int, int, u8**>>();

	void reset_machine_information() {
		ScrollableTable<debug_instr_data> program_buffer = ScrollableTable<debug_instr_data>(DEBUG_INSTR_ELEMENTS);
		std::vector<debug_instr_data> register_values = std::vector<debug_instr_data>();
		bool instruction_logging = false;
		bool pause_execution = true;
		int current_pc = 0;
		int rom_bank_size = 0;
		int current_rom_bank = 0;
		std::string current_instruction = "";

		float current_frequency = .0f;
		int wram_bank_selected = 0;
		int wram_bank_num = 0;
		int ram_bank_selected = 0;
		int ram_bank_num = 0;
		int rom_bank_selected = 0;
		int rom_bank_num = 0;
		int vram_bank_selected = 0;
		int vram_bank_num = 0;
	}
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
	bool request_reset = false;
};
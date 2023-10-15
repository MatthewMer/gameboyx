#pragma once

#include <queue>
#include <string>

#include "defs.h"
#include "ScrollableTable.h"
#include "imguigameboyx_config.h"

template <class T> using MemoryBufferEntry = std::pair<std::string, std::vector<T>>;			// memory buffer: type, vector<Table>
template <class T> using MemoryBuffer = std::vector<MemoryBufferEntry<T>>;				// memory buffer: type, vector<Table>

using DirectMemoryAccessEntry = std::tuple<int, int, int, int, u8**>;					// memory access: type, num, size, base_ptr, ref
using DirectMemoryAccess = std::vector<DirectMemoryAccessEntry>;						// memory access: type, num, size, base_ptr, ref

struct machine_information {
	// debug isntructions
	bool instruction_debug_enabled = false;
	// index, address, raw data, resolved data
	ScrollableTable<debug_instr_data> program_buffer = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
	std::vector<register_data> register_values = std::vector<register_data>();
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

	DirectMemoryAccess memory_access = DirectMemoryAccess();
	MemoryBuffer<ScrollableTable<memory_data>> memory_buffer = MemoryBuffer<ScrollableTable<memory_data>>();

	void reset_machine_information() {
		ScrollableTable<debug_instr_data> program_buffer = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);
		std::vector<register_data> register_values = std::vector<register_data>();
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

		DirectMemoryAccess memory_access = DirectMemoryAccess();
		MemoryBuffer<ScrollableTable<memory_data>> memory_buffer = MemoryBuffer<ScrollableTable<memory_data>>();
	}
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
	bool request_reset = false;
};
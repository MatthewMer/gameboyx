#pragma once

#include <queue>
#include <string>

#include "defs.h"

#define TIME_DELTA_THERSHOLD			1000000000		// ns

// TODO: fifo for debug info (GUI)
struct message_buffer {
	// debug isntructions
	bool instruction_debug_enabled = false;

	bool pause_execution = true;
	bool instruction_buffer_log = true;

	// vector for loaded program
	// index, pc location, raw data, resolved data
	std::vector<std::vector<std::tuple<int, int, std::string, std::string>>> program_buffer = std::vector<std::vector<std::tuple<int, int, std::string, std::string>>>();
	std::vector<std::pair<std::string, std::string>> register_values = std::vector<std::pair<std::string, std::string>>();
	int current_pc = 0;
	int last_pc = -1;
	int rom_bank_size = 0;
	int current_rom_bank = 0;

	// current hardware state
	bool track_hardware_info = false;
	float current_frequency = .0f;
	int wram_bank_selected = 0;
	int wram_bank_num = 0;
	int ram_bank_selected = 0;
	int ram_bank_num = 0;
	int rom_bank_selected = 0;
	int rom_bank_num = 0;
	int vram_bank = 0;
	int num_vram = 0;
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
};

static void reset_message_buffer(message_buffer& _msg_buffer) {
	//_msg_buffer.instruction_buffer = "";
	_msg_buffer.pause_execution = true;
	_msg_buffer.current_frequency = .0f;
	_msg_buffer.wram_bank_selected = 0;
	_msg_buffer.wram_bank_num = 0;
	_msg_buffer.ram_bank_selected = 0;
	_msg_buffer.ram_bank_num = 0;
	_msg_buffer.rom_bank_selected = 0;
	_msg_buffer.rom_bank_num = 0;
	_msg_buffer.vram_bank = 0;
	_msg_buffer.num_vram = 0;
	_msg_buffer.program_buffer = std::vector<std::vector<std::tuple<int, int, std::string, std::string>>>();
	_msg_buffer.register_values = std::vector<std::pair<std::string, std::string>>();
	_msg_buffer.current_pc = 0;
	_msg_buffer.last_pc = -1;
	_msg_buffer.rom_bank_size = 0;
	_msg_buffer.current_rom_bank = 0;
}
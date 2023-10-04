#pragma once

#include <queue>
#include <string>

#include "defs.h"

#define TIME_DELTA_THERSHOLD			1000000000		// ns

struct machine_information {
	// debug isntructions
	bool instruction_debug_enabled = false;
	// index, pc location, raw data, resolved data
	std::vector<std::vector<std::tuple<int, int, std::string, std::string>>> program_buffer = std::vector<std::vector<std::tuple<int, int, std::string, std::string>>>();
	std::vector<std::pair<std::string, std::string>> register_values = std::vector<std::pair<std::string, std::string>>();
	bool instruction_logging = true;
	bool pause_execution = true;
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
	int vram_bank_selected = 0;
	int vram_bank_num = 0;


};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
	bool request_reset = false;
};

static void reset_message_buffer(machine_information& _machine_info) {
	//_machine_info.instruction_buffer = "";
	_machine_info.pause_execution = true;
	_machine_info.current_frequency = .0f;
	_machine_info.wram_bank_selected = 0;
	_machine_info.wram_bank_num = 0;
	_machine_info.ram_bank_selected = 0;
	_machine_info.ram_bank_num = 0;
	_machine_info.rom_bank_selected = 0;
	_machine_info.rom_bank_num = 0;
	_machine_info.vram_bank_selected = 0;
	_machine_info.vram_bank_num = 0;
	_machine_info.program_buffer = std::vector<std::vector<std::tuple<int, int, std::string, std::string>>>();
	_machine_info.register_values = std::vector<std::pair<std::string, std::string>>();
	_machine_info.current_pc = 0;
	_machine_info.last_pc = -1;
	_machine_info.rom_bank_size = 0;
	_machine_info.current_rom_bank = 0;
}
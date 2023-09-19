#pragma once

#include <queue>
#include <string>

#include "defs.h"

#define TIME_DELTA_THERSHOLD			1000000000		// ns

// TODO: fifo for debug info (GUI)
struct message_buffer {
	// debug isntructions
	std::string instruction_buffer = "";
	bool instruction_buffer_enabled = false;
	bool pause_execution = true;
	bool auto_run = false;
	bool instruction_buffer_log = true;

	// current hardware state
	float current_frequency = .0f;
	bool track_hardware_info = false;
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
};
#pragma once

#include <queue>
#include <string>

// TODO: fifo for debug info (GUI)
struct message_buffer {
	std::string debug_instruction = "";
	bool debug_instruction_enabled = false;
	bool pause_execution = true;
	bool auto_run = false;
	bool debug_instruction_log = true;
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
};
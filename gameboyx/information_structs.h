#pragma once

#include <queue>
#include <string>

// TODO: fifo for debug info (GUI)
struct message_fifo {
	std::queue<std::string> debug_instructions = std::queue<std::string>();
	bool debug_instructions_enabled = false;
	bool pause_execution = true;
	bool auto_run = false;
};

struct game_status {
	bool game_running = false;
	bool pending_game_start = false;
	bool pending_game_stop = false;
	int game_to_start = 0;
};
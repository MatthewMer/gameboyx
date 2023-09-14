#pragma once

#include <queue>
#include <string>

// TODO: fifo for debug info (GUI)
struct message_fifo {
	std::queue<std::string> debug_instructions = std::queue<std::string>();
	bool debug_instructions_enabled = false;
};
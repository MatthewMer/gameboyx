#pragma once

#include <vector>

using namespace std;

struct game {
	char* title;
	char* licensee;
	char* cart_type;
	bool is_gbc;
	bool is_sgb;
	char* dest_code;
	char* game_ver;
	char* chksum;
	char* file_name;
};

bool read_games_from_config(vector<game>& games, const char* config_path);
bool write_game_to_config(game &new_game, const char* config_path);
bool delete_game_from_config(game& new_game, const char* config_path);
bool check_file_exist(const char* config_path);
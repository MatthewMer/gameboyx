#pragma once

#include <vector>
#include "game_info.h"

bool read_games_from_config(std::vector<game_info>& _games, const std::string& _config_path_rel);
bool write_games_to_config(const std::vector<game_info>& _games, const std::string& _config_path_rel, const bool& _rewrite);
bool delete_games_from_config(std::vector<game_info>& _games, const std::string& _config_path_rel);

bool write_to_debug_log(const std::string& _output, const std::string& _file_path_rel, const bool& _rewrite);

void check_and_create_config_folders();
void check_and_create_config_files();
void check_and_create_log_folders();
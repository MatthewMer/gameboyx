#pragma once

#include <vector>
#include "game_info.h"

bool read_games_from_config(std::vector<game_info>& _games, const std::string& _config_path_rel);
bool write_games_to_config(const std::vector<game_info>& _games, const std::string& _config_path_rel, bool _rewrite);
bool delete_games_from_config(std::vector<game_info>& _games, const std::string& _config_path_rel);

void check_and_create_config_folders();
void check_and_create_config_files();
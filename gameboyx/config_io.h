#pragma once

#include <vector>
#include "game_info.h"

bool read_games_from_config(std::vector<game_info>& games, const std::string& config_path);
bool write_game_to_config(const game_info& game_ctx, const std::string& config_path);
bool delete_game_from_config(std::vector<game_info>& games, const game_info& game_ctx, const std::string& config_path);
bool write_new_game(const game_info& game_ctx);
bool check_and_create_folders();
bool check_and_create_files();
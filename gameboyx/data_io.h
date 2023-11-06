#pragma once
/*
*	this header provides the functionality to interact with the computers filesystem. Unnecessary to put it in a seperate class
*	due to just occasionaly calling functions.
*/

#include <vector>
#include "game_info.h"

bool read_games_from_config(std::vector<game_info>& _games, const std::string& _config_path_rel);
bool write_games_to_config(const std::vector<game_info>& _games, const std::string& _config_path_rel, const bool& _rewrite);
bool delete_games_from_config(std::vector<game_info>& _games, const std::string& _config_path_rel);

bool write_to_debug_log(const std::string& _output, const std::string& _file_path_rel, const bool& _rewrite);

bool read_data(std::vector<std::string>& _input, const std::string& _file_path, const bool& _relative);
bool write_data(const std::vector<std::string>& _output, const std::string& _file_path, const bool& _relative, const bool& _rewrite);
bool read_data(std::vector<char>& _input, const std::string& _file_path, const bool& _relative);
bool write_data(const std::vector<char>& _output, const std::string& _file_path, const bool& _relative, const bool& _rewrite);

std::string check_and_create_file(const std::string& _path_to_file_rel, const bool& _relative);
std::string check_and_create_path(const std::string& _path_rel);

std::string get_current_path();
std::vector<std::string> get_files_in_path(const std::string& _path_rel);

void check_and_create_config_folders();
void check_and_create_config_files();
void check_and_create_log_folders();
void check_and_create_shader_folders();
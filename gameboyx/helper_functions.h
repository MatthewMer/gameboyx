#pragma once

#include <vector>
#include <string>

#include "defs.h"

std::vector<std::string> split_string(const std::string& _in_string, const std::string& _delimiter);
std::string check_and_create_file(const std::string& _path_to_file_rel, const bool& _relative);
std::string check_and_create_path(const std::string& _path_rel);
std::string get_current_path();
std::vector<std::string> get_files_in_path(const std::string& _path_rel);
std::string trim(const std::string& _in_string);
std::string ltrim(const std::string& _in_string);
std::string rtrim(const std::string& _in_string);
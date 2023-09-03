#pragma once

#include <vector>
#include <string>

#include "defs.h"


std::vector<std::string> split_string(const std::string& in_string, const std::string& delimiter);
bool check_and_create_file(const std::string& path_to_file);
bool check_and_create_path(const std::string& path_rel);
std::string get_current_path();
std::string trim(const std::string& in_string);
std::string ltrim(const std::string& in_string);
std::string rtrim(const std::string& in_string);
#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	A collection of usefull functions
*/

#include <vector>
#include <string>

#include "defs.h"

std::vector<std::string> split_string(const std::string& _in_string, const std::string& _delimiter);
std::vector<std::string> get_files_in_path(const std::string& _path_rel);
std::string trim(const std::string& _in_string);
std::string ltrim(const std::string& _in_string);
std::string rtrim(const std::string& _in_string);
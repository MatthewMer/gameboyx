#pragma once

#include <vector>
#include <string>

#include "defs.h"

struct Vec2 {
    int x;
    int y;

    Vec2(int _x, int _y) : x(_x), y(_y) {};

    bool operator==(const Vec2& n) const {
        return (n.x == x) && (n.y == y);
    }
};

std::vector<std::string> split_string(const std::string& _in_string, const std::string& _delimiter);
std::string check_and_create_file(const std::string& _path_to_file_rel);
std::string check_and_create_path(const std::string& _path_rel);
std::string get_current_path();
std::string trim(const std::string& _in_string);
std::string ltrim(const std::string& _in_string);
std::string rtrim(const std::string& _in_string);
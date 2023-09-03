#include "helper_functions.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

std::vector<std::string> split_string(const std::string& in_string, const std::string& delimiter) {
    std::vector<std::string> vec_in_string;
    std::string in_string_copy = in_string;

    while (in_string_copy.find(delimiter) != std::string::npos) {
        vec_in_string.push_back(in_string_copy.substr(0, in_string_copy.find(delimiter)));
        in_string_copy.erase(0, in_string_copy.find(delimiter) + delimiter.length());
    }
    vec_in_string.push_back(in_string_copy);

    return vec_in_string;
}

bool check_and_create_file(const std::string& path_to_file) {
    if (fs::exists(path_to_file)) {
        return true;
    }
    else {
        ofstream(path_to_file).close();
        return false;
    }
}

bool check_and_create_path(const std::string& path) {
    if (!fs::is_directory(path) || !fs::exists(path)) {
        fs::create_directory(path);
    }

    return fs::is_directory(path) || fs::exists(path);
}

string get_current_path() {
    string current_path = fs::current_path().string();
    return current_path;
}
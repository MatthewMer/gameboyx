#include "helper_functions.h"

#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

const string WHITESPACE = " \n\r\t\f\v";

vector<string> split_string(const string& _in_string, const string& _delimiter) {
    vector<string> vec_in_string;
    string in_string_copy = _in_string;

    while (in_string_copy.find(_delimiter) != string::npos) {
        vec_in_string.push_back(in_string_copy.substr(0, in_string_copy.find(_delimiter)));
        in_string_copy.erase(0, in_string_copy.find(_delimiter) + _delimiter.length());
    }
    vec_in_string.push_back(in_string_copy);

    return vec_in_string;
}

string check_and_create_file(const string& _path_to_file_rel) {
    string full_file_path = get_current_path() + _path_to_file_rel;

    if (!fs::exists(full_file_path)) {
        ofstream(full_file_path).close();
    }
    
    return full_file_path;
}

string check_and_create_path(const string& _path_rel) {
    string full_path = get_current_path() + _path_rel;
    if (!fs::is_directory(full_path) || !fs::exists(full_path)) {
        fs::create_directory(full_path);
    }

    return full_path;
}

string get_current_path() {
    vector<string> current_path_vec = split_string(fs::current_path().string(), "\\");
    string current_path = "";
    for (int i = 0; i < current_path_vec.size() - 1; i++) {
        current_path += current_path_vec[i] + "/";
    }
    current_path += current_path_vec.back();

    return current_path;
}

string trim(const string& _in_string) {
    return rtrim(ltrim(_in_string));
}

string ltrim(const string& _in_string) {
    size_t start = _in_string.find_first_not_of(WHITESPACE);
    return (start == string::npos) ? "" : _in_string.substr(start);
}

string rtrim(const string& _in_string) {
    size_t end = _in_string.find_last_not_of(WHITESPACE);
    return (end == string::npos) ? "" : _in_string.substr(0, end + 1);
}
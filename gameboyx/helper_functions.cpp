#include "helper_functions.h"

#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

const string WHITESPACE = " \n\r\t\f\v";

vector<string> split_string(const string& in_string, const string& delimiter) {
    vector<string> vec_in_string;
    string in_string_copy = in_string;

    while (in_string_copy.find(delimiter) != string::npos) {
        vec_in_string.push_back(in_string_copy.substr(0, in_string_copy.find(delimiter)));
        in_string_copy.erase(0, in_string_copy.find(delimiter) + delimiter.length());
    }
    vec_in_string.push_back(in_string_copy);

    return vec_in_string;
}

bool check_and_create_file(const string& path_to_file_rel) {
    string current_path = get_current_path();

    if (fs::exists(current_path + path_to_file_rel)) {
        return true;
    }
    else {
        ofstream(current_path + path_to_file_rel).close();
        return false;
    }
}

bool check_and_create_path(const string& path_rel) {
    string current_path = get_current_path();
    string new_path = current_path + path_rel;
    if (!fs::is_directory(new_path) || !fs::exists(new_path)) {
        fs::create_directory(new_path);
    }

    return fs::is_directory(new_path) || fs::exists(new_path);
}

string get_current_path() {
    string current_path = fs::current_path().string();
    return current_path;
}

string trim(const string& in_string) {
    return rtrim(ltrim(in_string));
}

string ltrim(const string& in_string) {
    size_t start = in_string.find_first_not_of(WHITESPACE);
    return (start == string::npos) ? "" : in_string.substr(start);
}

string rtrim(const string& in_string) {
    size_t end = in_string.find_last_not_of(WHITESPACE);
    return (end == string::npos) ? "" : in_string.substr(0, end + 1);
}
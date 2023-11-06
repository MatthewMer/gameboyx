#include "data_io.h"

#include "game_info.h"
#include "logger.h"
#include "helper_functions.h"
#include "general_config.h"

#include <fstream>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

void games_from_string(vector<game_info>& _games, const vector<string>& _config_games);
void games_to_string(const vector<game_info>& _games, vector<string>& _config_games);

bool read_games_from_config(vector<game_info>& _games, const string& _config_path_rel) {
    if (auto config_games = vector<string>(); read_data(config_games, _config_path_rel, true)) {
        games_from_string(_games, config_games);
        return true;
    }

    return false;
}


bool write_games_to_config(const vector<game_info>& _games, const string& _config_path_rel, const bool& _rewrite) {
    auto config_games = vector<string>();
    games_to_string(_games, config_games);

    if (write_data(config_games, _config_path_rel, true, _rewrite)) {
        if(!_rewrite) LOG_INFO(_games.size(), " game(s) added to ." + _config_path_rel);
        return true; 
    }

    return false;
}

string check_and_create_path(const string& _path_rel) {
    string full_path = get_current_path() + _path_rel;
    if (!fs::is_directory(full_path) || !fs::exists(full_path)) {
        fs::create_directory(full_path);
    }

    return full_path;
}

string check_and_create_file(const string& _path_to_file, const bool& _relative) {
    string full_file_path;
    if (_relative) {
        full_file_path = get_current_path() + _path_to_file;
    }
    else {
        full_file_path = _path_to_file;
    }

    if (!fs::exists(full_file_path)) {
        ofstream(full_file_path).close();
    }

    return full_file_path;
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

vector<string> get_files_in_path(const string& _path_rel) {
    auto files = vector<string>();

    for (const auto& n : fs::directory_iterator(get_current_path() + _path_rel)) {
        if (n.is_directory()) { continue; }
        files.emplace_back(n.path().generic_string());
    }

    return files;
}

bool delete_games_from_config(vector<game_info>& _games, const std::string& _config_path_rel) {
    auto games = vector<game_info>();
    if (!read_games_from_config(games, _config_path_rel)) {
        return false;
    }

    for (int i = games.size() - 1; i >= 0; i--) {
        for (const auto& m : _games) {
            if (games[i] == m) {
                games.erase(games.begin() + i);
            }
        }
    }

    if (!write_games_to_config(games, _config_path_rel, true)) {
        return false;
    }

    LOG_INFO(_games.size(), " game(s) removed from .", _config_path_rel);

    return true;
}


bool write_to_debug_log(const string& _output, const string& _file_path_rel, const bool& _rewrite) {
    check_and_create_log_folders();

    return write_data({ _output }, _file_path_rel, true, _rewrite);
}


bool read_data(vector<string>& _input, const string& _file_path, const bool& _relative) {
    string file_path = check_and_create_file(_file_path, _relative);

    ifstream is(file_path, ios::beg);
    if (!is) {
        LOG_WARN("Couldn't open ", file_path);
        return false;
    }

    string line;
    _input.clear();
    while (getline(is, line)) {
        _input.push_back(line);
    }

    is.close();
    return true;
}

bool read_data(std::vector<char>& _input, const std::string& _file_path, const bool& _relative) {
    string file_path = check_and_create_file(_file_path, _relative);

    ifstream is(file_path, ios::beg | ios::binary);
    if (!is) {
        LOG_WARN("Couldn't open ", file_path);
        return false;
    }

    _input = vector<char>();
    _input = vector<char>(istreambuf_iterator<char>(is), istreambuf_iterator<char>());
}

bool write_data(const vector<string>& _output, const string& _file_path, const bool& _relative, const bool& _rewrite) {
    string file_path = check_and_create_file(_file_path, _relative);
    
    ofstream os(file_path, (_rewrite ? ios::trunc : ios::app));
    if (!os.is_open()) {
        LOG_WARN("Couldn't write ", file_path);
        return false;
    }

    for (const auto& n : _output) {
        os << n << endl;
    }

    os.close();
    return true;
}

bool write_data(const vector<char>& _output, const string& _file_path, const bool& _relative, const bool& _rewrite) {
    string file_path = check_and_create_file(_file_path, _relative);

    ofstream os(file_path, (_rewrite ? ios::trunc : ios::app) | ios::binary);
    if (!os.is_open()) {
        LOG_WARN("Couldn't write ", file_path);
        return false;
    }

    for (const auto& n : _output) {
        os << n << endl;
    }

    os.close();
    return true;
}

void check_and_create_config_folders() {
    check_and_create_path(CONFIG_FOLDER);
}

void check_and_create_config_files() {
    check_and_create_file(CONFIG_FOLDER + GAMES_CONFIG_FILE, true);
}

void check_and_create_log_folders() {
    check_and_create_path(LOG_FOLDER);
}

void check_and_create_shader_folders() {
    check_and_create_path(SHADER_FOLDER);
    check_and_create_path(SPIR_V_FOLDER);
}
#include "data_io.h"

#include "logger.h"
#include "helper_functions.h"
#include "general_config.h"
#include "BaseCartridge.h"

#include <fstream>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

void games_from_string(vector<BaseCartridge*>& _games, const vector<string>& _config_games);
void games_to_string(const vector<BaseCartridge*>& _games, vector<string>& _config_games);

bool read_games_from_config(vector<BaseCartridge*>& _games) {
    if (auto config_games = vector<string>(); read_data(config_games, CONFIG_FOLDER + GAMES_CONFIG_FILE)) {
        games_from_string(_games, config_games);
        return true;
    }

    return false;
}


bool write_games_to_config(const vector<BaseCartridge*>& _games, const bool& _rewrite) {
    auto config_games = vector<string>();
    games_to_string(_games, config_games);

    if (write_data(config_games, CONFIG_FOLDER + GAMES_CONFIG_FILE, _rewrite)) {
        if (!_rewrite) LOG_INFO("[emu] ", (int)_games.size(), " game(s) added to ." + CONFIG_FOLDER + GAMES_CONFIG_FILE);
        return true;
    }

    return false;
}

string check_and_create_path(const string& _path_rel) {
    if (!fs::is_directory(_path_rel) || !fs::exists(_path_rel)) {
        fs::create_directory(_path_rel);
    }

    return get_current_path() + "/" + _path_rel;
}

string check_and_create_file(const string& _path_to_file) {
    if (!fs::exists(_path_to_file)) {
        ofstream(_path_to_file).close();
    }

    return _path_to_file;
}

bool check_file(const string& _path_to_file) {
    return fs::exists(_path_to_file);
}

bool check_file_exists(const string& _path_to_file) {
    return fs::exists(_path_to_file);
}

string get_current_path() {
    vector<string> current_path_vec = split_string(fs::current_path().string(), "\\");
    string current_path = "";
    for (const auto& n : current_path_vec) {
        current_path += n + "/";
    }

    return current_path;
}

void get_files_in_path(vector<string>& _input, const string& _path_rel) {
    _input.clear();

    for (const auto& n : fs::directory_iterator(_path_rel)) {
        if (n.is_directory()) { continue; }
        _input.emplace_back(n.path().generic_string());
    }
}

bool delete_games_from_config(vector<BaseCartridge*>& _games) {
    auto games = vector<BaseCartridge*>();
    if (!read_games_from_config(games)) {
        LOG_ERROR("[emu] reading games from ", CONFIG_FOLDER + GAMES_CONFIG_FILE);
        return false;
    }

    for (int i = (int)games.size() - 1; i >= 0; i--) {
        for (const auto& m : _games) {
            if ((games[i]->filePath + games[i]->fileName).compare(m->filePath + m->fileName) == 0) {
                delete games[i];
                games.erase(games.begin() + i);
            }
        }
    }

    if (!write_games_to_config(games, true)) {
        return false;
    }

    LOG_INFO("[emu] ", (int)_games.size(), " game(s) removed from .", CONFIG_FOLDER + GAMES_CONFIG_FILE);

    return true;
}


bool write_to_debug_log(const string& _output, const string& _file_path, const bool& _rewrite) {
    check_and_create_log_folders();

    return write_data({ _output }, _file_path, _rewrite);
}


bool read_data(vector<string>& _input, const string& _file_path) {
    string file_path = check_and_create_file(_file_path);

    ifstream is(file_path, ios::beg);
    if (!is) {
        LOG_WARN("[emu] Couldn't open ", file_path);
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

bool read_data(std::vector<char>& _input, const std::string& _file_path) {
    if (check_file(_file_path)) {
        ifstream is(_file_path, ios::beg | ios::binary);
        if (!is) {
            LOG_WARN("[emu] Couldn't open ", _file_path);
            return false;
        }

        _input = vector<char>(istreambuf_iterator<char>(is), istreambuf_iterator<char>());
        return true;
    } else {
        LOG_ERROR("[emu] file ", _file_path, " does not exist");
        return false;
    }
}

bool write_data(const vector<string>& _output, const string& _file_path, const bool& _rewrite) {
    string file_path = check_and_create_file(_file_path);

    ofstream os(file_path, (_rewrite ? ios::trunc : ios::app));
    if (!os.is_open()) {
        LOG_WARN("[emu] Couldn't write ", file_path);
        return false;
    }

    for (const auto& n : _output) {
        os << n << endl;
    }

    os.close();
    return true;
}

bool write_data(const vector<char>& _output, const string& _file_path, const bool& _rewrite) {
    string file_path = check_and_create_file(_file_path);

    ofstream os(file_path, (_rewrite ? ios::trunc : ios::app) | ios::binary);
    if (!os.is_open()) {
        LOG_WARN("[emu] Couldn't write ", file_path);
        return false;
    }

    for (const auto& n : _output) {
        os << n;
    }

    os.close();
    return true;
}

void check_and_create_config_folders() {
    check_and_create_path(CONFIG_FOLDER);
}

void check_and_create_config_files() {
    check_and_create_file(CONFIG_FOLDER + GAMES_CONFIG_FILE);
}

void check_and_create_log_folders() {
    check_and_create_path(LOG_FOLDER);
}

void check_and_create_shader_folders() {
    check_and_create_path(SHADER_FOLDER);
    check_and_create_path(SPIR_V_FOLDER);
}

void check_and_create_save_folders() {
    check_and_create_path(SAVE_FOLDER);
}

void check_and_create_rom_folder() {
    check_and_create_path(ROM_FOLDER);
}

const unordered_map<info_types, std::string> INFO_TYPES_MAP = {
    { TITLE, "title" },
    { CONSOLE,"console" },
    { FILE_NAME,"file_name" },
    { FILE_PATH,"file_path" },
    { GAME_VER,"game_ver" }
};

void games_from_string(vector<BaseCartridge*>& _games, const vector<string>& _config_games) {
    string line;
    _games.clear();

    string title = "";
    string file_name = "";
    string file_path = "";
    string console = "";
    string version = "";
    console_ids id = CONSOLE_NONE;

    bool entries_found = false;

    for (int i = 0; i < (int)_config_games.size(); i++) {
        if (_config_games[i].compare("") == 0) { continue; }
        line = trim(_config_games[i]);

        // find start of entry
        if (line.find("[") == 0 && line.find("]") == line.length() - 1) {
            if (entries_found) {
                _games.emplace_back(BaseCartridge::existing_game(title, file_name, file_path, id, version));
                title = file_name = file_path = version = "";
                id = CONSOLE_NONE;
            }
            entries_found = true;

            title = split_string(split_string(line, "[").back(), "]").front();
        }
        // filter entry parameters
        else if(_config_games[i].compare("") != 0 && entries_found) {
            auto parameter = split_string(line, "=");

            if ((int)parameter.size() != 2) {
                LOG_WARN("[emu] Error in line ", (i + 1));
                continue;
            }

            parameter[0] = trim(parameter[0]);
            parameter[1] = trim(parameter[1]);

            if (parameter.front().compare(INFO_TYPES_MAP.at(CONSOLE)) == 0) {
                for (const auto& [key, value] : FILE_EXTS) {
                    if (value.second.compare(parameter.back()) == 0) {
                        id = key;
                    }
                }
            } else if (parameter.front().compare(INFO_TYPES_MAP.at(FILE_NAME)) == 0) {
                file_name = parameter.back();
            } else if (parameter.front().compare(INFO_TYPES_MAP.at(FILE_PATH)) == 0) {
                file_path = parameter.back();
            } else if (parameter.front().compare(INFO_TYPES_MAP.at(GAME_VER)) == 0) {
                version = parameter.back();
            }
        }
    }

    for (const auto& [key, value] : FILE_EXTS) {
        if (value.second.compare(console) == 0) {
            id = key;
        }
    }

    auto cartridge = BaseCartridge::existing_game(title, file_name, file_path, id, version);
    if (cartridge != nullptr) {
        _games.emplace_back(cartridge);
    }

    return;
}

void games_to_string(const vector<BaseCartridge*>& _games, vector<string>& _config_games) {
    _config_games.clear();
    for (const auto& n : _games) {
        _config_games.emplace_back("");
        _config_games.emplace_back("[" + n->title + "]");
        _config_games.emplace_back(INFO_TYPES_MAP.at(FILE_NAME) + "=" + n->fileName);
        _config_games.emplace_back(INFO_TYPES_MAP.at(FILE_PATH) + "=" + n->filePath);
        _config_games.emplace_back(INFO_TYPES_MAP.at(CONSOLE) + "=" + FILE_EXTS.at(n->console).second);
        _config_games.emplace_back(INFO_TYPES_MAP.at(GAME_VER) + "=" + n->version);
    }
}
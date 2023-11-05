#include "data_io.h"

#include "game_info.h"
#include "logger.h"
#include "helper_functions.h"
#include "general_config.h"

#include <fstream>
#include <string>

using namespace std;


bool read_data(vector<string>& _config_input, const string& _config_path_rel);
bool write_data(const vector<string>& _games, const string& _config_path_rel, bool _rewrite);

void games_from_string(vector<game_info>& _games, const vector<string>& _config_games);
void games_to_string(const vector<game_info>& _games, vector<string>& _config_games);
bool filter_parameter_game_info_enum(game_info& _game_ctx, const vector<string>& _parameter);


bool read_games_from_config(vector<game_info>& _games, const string& _config_path_rel) {
    if (auto config_games = vector<string>(); read_data(config_games, _config_path_rel)) {
        games_from_string(_games, config_games);
        return true;
    }

    return false;
}


bool write_games_to_config(const vector<game_info>& _games, const string& _config_path_rel, const bool& _rewrite) {
    auto config_games = vector<string>();
    games_to_string(_games, config_games);

    if (write_data(config_games, _config_path_rel, _rewrite)) {
        if(!_rewrite) LOG_INFO(_games.size(), " game(s) added to ." + _config_path_rel);
        return true; 
    }

    return false;
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

    return write_data({ _output }, _file_path_rel, _rewrite);
}


bool read_data(vector<string>& _config_input, const string& _config_path_rel) {
    string full_config_path = check_and_create_file(_config_path_rel);

    ifstream is(full_config_path, ios::beg);
    if (!is) { 
        LOG_WARN("Couldn't read .", _config_path_rel);
        return false; 
    }
    string line;
    _config_input.clear();
    while (getline(is, line)) {
        _config_input.push_back(line);
    }

    is.close();
    return true;
}

bool write_data(const vector<string>& _output, const string& _file_path_rel, bool _rewrite) {
    string full_config_path = check_and_create_file(_file_path_rel);
    
    ofstream os(full_config_path, (_rewrite ? ios::trunc : ios::app));
    if (!os.is_open()) {
        LOG_WARN("Couldn't write .", _file_path_rel);
        return false;
    }

    for (const auto& n : _output) {
        os << n << endl;
    }

    os.close();
    return true;
}

void games_from_string(vector<game_info>& _games, const vector<string>& _config_games) {
    game_info game_ctx("");
    string line;
    _games.clear();

    for (int i = 0; i < _config_games.size(); i++) {
        if (_config_games[i].compare("") == 0) { continue; }
        line = trim(_config_games[i]);

        // find start of entry
        if (line.find("[") == 0 && line.find("]") == line.length() - 1) {
            if (game_ctx.title.compare("") != 0) {
                _games.push_back(game_ctx);
            }

            game_ctx = game_info(line.substr(1, line.length() - 2));
        }
        // filter entry parameters
        else if (game_ctx.title.compare("") != 0) {
            const auto parameter = split_string(line, "=");

            if (parameter.size() != 2) {
                LOG_WARN("Multiple '=' on line ", (i + 1));
                continue;
            }

            if (!filter_parameter_game_info_enum(game_ctx, parameter)) {
                LOG_WARN("Faulty parameter on line ", (i + 1));
            }
        }
        else {
            LOG_WARN("Game entry error line ", i);
        }
    }

    if (game_ctx.title.compare("") != 0)
        _games.push_back(game_ctx);

    return;
}

void games_to_string(const vector<game_info>& _games, vector<string>& _config_games) {
    _config_games.clear();
    for (const auto& n : _games) {
        _config_games.push_back("");
        _config_games.push_back("[" + n.title + "]");
        _config_games.push_back(get_info_type_string(FILE_NAME) + "=" + n.file_name);
        _config_games.push_back(get_info_type_string(FILE_PATH) + "=" + n.file_path);
        _config_games.push_back(get_info_type_string(GAME_VER) + "=" + n.game_ver);
        _config_games.push_back(get_info_type_string(IS_CGB) + "=" + (n.is_cgb ? PARAMETER_TRUE : PARAMETER_FALSE));
        _config_games.push_back(get_info_type_string(CART_TYPE) + "=" + n.cart_type);
        _config_games.push_back(get_info_type_string(LICENSEE) + "=" + n.licensee);
        _config_games.push_back(get_info_type_string(DEST_CODE) + "=" + n.dest_code);
    }
}

bool filter_parameter_game_info_enum(game_info& _game_ctx, const vector<string>& _parameter) {
    switch (get_info_type_enum(trim(_parameter[0]))) {
    case TITLE:
        _game_ctx.title = trim(_parameter[1]);
        break;
    case LICENSEE:
        _game_ctx.licensee = trim(_parameter[1]);
        break;
    case CART_TYPE:
        _game_ctx.cart_type = trim(_parameter[1]);
        break;
    case IS_CGB: {
        string value = trim(_parameter[1]);
        bool result = (value.compare(PARAMETER_TRUE) == 0 ? true : false);
        _game_ctx.is_cgb = result;
        if (!result && value.compare(PARAMETER_FALSE) != 0) {
            return false;
        }
    }
        break;
    case IS_SGB: {
        string value = trim(_parameter[1]);
        bool result = (value.compare(PARAMETER_TRUE) == 0 ? true : false);
        _game_ctx.is_sgb = result;
        if (!result && value.compare(PARAMETER_FALSE) != 0) {
            return false;
        }
    }
        break;
    case DEST_CODE:
        _game_ctx.dest_code = trim(_parameter[1]);
        break;
    case GAME_VER:
        _game_ctx.game_ver = trim(_parameter[1]);
        break;
    case CHKSUM_PASSED: {
        string value = trim(_parameter[1]);
        bool result = (value.compare(PARAMETER_TRUE) == 0 ? true : false);
        _game_ctx.chksum_passed = result;
        if (!result && value.compare(PARAMETER_FALSE) != 0) {
            return false;
        }
    }
        break;
    case FILE_NAME:
        _game_ctx.file_name = trim(_parameter[1]);
        break;
    case FILE_PATH:
        _game_ctx.file_path = trim(_parameter[1]);
        break;
    default:
        return false;
        break;
    }
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

void check_and_create_shader_folder() {
    check_and_create_path(SHADER_FOLDER);
}
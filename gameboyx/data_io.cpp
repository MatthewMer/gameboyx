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

namespace Backend {
    namespace FileIO {
        void games_from_string(vector<Emulation::BaseCartridge*>& _games, const vector<string>& _config_games);
        void games_to_string(const vector<Emulation::BaseCartridge*>& _games, vector<string>& _config_games);

        bool read_games_from_config(vector<Emulation::BaseCartridge*>& _games) {
            if (auto config_games = vector<string>(); read_data(config_games, Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE)) {
                games_from_string(_games, config_games);
                return true;
            }

            return false;
        }


        bool write_games_to_config(const vector<Emulation::BaseCartridge*>& _games, const bool& _rewrite) {
            auto config_games = vector<string>();
            games_to_string(_games, config_games);

            if (FileIO::write_data(config_games, Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE, _rewrite)) {
                if (!_rewrite) LOG_INFO("[emu] ", (int)_games.size(), " game(s) added to " + Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);
                return true;
            }

            return false;
        }

        bool check_and_create_path(const string& _path_rel) {
            auto path = Helpers::split_string(_path_rel, "/");
            bool exists = true;

            for (size_t i = 0; i < path.size(); i++) {
                string sub_path = "";
                for (int j = 0; j <= i; j++) {
                    sub_path += path[j] + "/";
                }

                if (!fs::exists(sub_path) || !fs::is_directory(sub_path)) {
                    fs::create_directory(sub_path);
                    exists = false;
                }
            }

            return exists;
        }

        bool check_and_create_file(const string& _path_to_file) {
            if (!fs::exists(_path_to_file)) {
                ofstream(_path_to_file).close();
                return false;
            } else {
                return true;
            }
        }

        bool check_file(const string& _path_to_file) {
            return fs::exists(_path_to_file);
        }

        bool check_file_exists(const string& _path_to_file) {
            return fs::exists(_path_to_file);
        }

        string get_current_path() {
            vector<string> current_path_vec = Helpers::split_string(fs::current_path().string(), "\\");
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

        bool delete_games_from_config(vector<Emulation::BaseCartridge*>& _games) {
            auto games = vector<Emulation::BaseCartridge*>();
            if (!read_games_from_config(games)) {
                LOG_ERROR("[emu] reading games from ", Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);
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

            LOG_INFO("[emu] ", (int)_games.size(), " game(s) removed from ", Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);

            return true;
        }


        bool write_to_debug_log(const string& _output, const string& _file_path, const bool& _rewrite) {
            check_and_create_log_folders();

            return write_data({ _output }, _file_path, _rewrite);
        }


        bool read_data(vector<string>& _input, const string& _file_path) {
            check_and_create_file(_file_path);

            ifstream is(_file_path, ios::beg);
            if (!is) {
                LOG_WARN("[emu] Couldn't open ", _file_path);
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
                if (_input.size() > 0) {
                    return true;
                } else {
                    LOG_ERROR("[emu] file ", _file_path, " damaged");
                    return false;
                }
            } else {
                LOG_ERROR("[emu] file ", _file_path, " does not exist");
                return false;
            }
        }

        bool write_data(const vector<string>& _output, const string& _file_path, const bool& _rewrite) {
            check_and_create_file(_file_path);

            ofstream os(_file_path, (_rewrite ? ios::trunc : ios::app));
            if (!os.is_open()) {
                LOG_WARN("[emu] Couldn't write ", _file_path);
                return false;
            }

            for (const auto& n : _output) {
                os << n << endl;
            }

            os.close();
            return true;
        }

        bool write_data(const vector<char>& _output, const string& _file_path, const bool& _rewrite) {
            check_and_create_file(_file_path);

            ofstream os(_file_path, (_rewrite ? ios::trunc : ios::app) | ios::binary);
            if (!os.is_open()) {
                LOG_WARN("[emu] Couldn't write ", _file_path);
                return false;
            }

            for (const auto& n : _output) {
                os << n;
            }

            os.close();
            return true;
        }

        void check_and_create_config_folders() {
            check_and_create_path(Config::CONFIG_FOLDER);
        }

        void check_and_create_config_files() {
            check_and_create_file(Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);
        }

        void check_and_create_log_folders() {
            check_and_create_path(Config::LOG_FOLDER);
        }

        void check_and_create_shader_folders() {
            check_and_create_path(Config::SHADER_FOLDER);
            check_and_create_path(Config::SPIR_V_FOLDER);
        }

        void check_and_create_save_folders() {
            check_and_create_path(Config::SAVE_FOLDER);
        }

        void check_and_create_rom_folder() {
            check_and_create_path(Config::ROM_FOLDER);
        }

        const unordered_map<Emulation::info_types, std::string> INFO_TYPES_MAP = {
            { Emulation::TITLE, "title" },
            { Emulation::CONSOLE,"console" },
            { Emulation::FILE_NAME,"file_name" },
            { Emulation::FILE_PATH,"file_path" },
            { Emulation::GAME_VER,"game_ver" }
        };

        void games_from_string(vector<Emulation::BaseCartridge*>& _games, const vector<string>& _config_games) {
            string line;
            _games.clear();

            string title = "";
            string file_name = "";
            string file_path = "";
            string console = "";
            string version = "";
            Config::console_ids id = Config::CONSOLE_NONE;

            bool entries_found = false;

            for (int i = 0; i < (int)_config_games.size(); i++) {
                if (_config_games[i].compare("") == 0) { continue; }
                line = Helpers::trim(_config_games[i]);

                // find start of entry
                if (line.find("[") == 0 && line.find("]") == line.length() - 1) {
                    if (entries_found) {
                        _games.emplace_back(Emulation::BaseCartridge::existing_game(title, file_name, file_path, id, version));
                        title = file_name = file_path = version = "";
                        id = Config::CONSOLE_NONE;
                    }
                    entries_found = true;

                    title = Helpers::split_string(Helpers::split_string(line, "[").back(), "]").front();
                }
                // filter entry parameters
                else if (_config_games[i].compare("") != 0 && entries_found) {
                    auto parameter = Helpers::split_string(line, "=");

                    if ((int)parameter.size() != 2) {
                        LOG_WARN("[emu] Error in line ", (i + 1));
                        continue;
                    }

                    parameter[0] = Helpers::trim(parameter[0]);
                    parameter[1] = Helpers::trim(parameter[1]);

                    if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::CONSOLE)) == 0) {
                        for (const auto& [key, value] : Config::FILE_EXTS) {
                            if (value.second.compare(parameter.back()) == 0) {
                                id = key;
                            }
                        }
                    } else if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::FILE_NAME)) == 0) {
                        file_name = parameter.back();
                    } else if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::FILE_PATH)) == 0) {
                        file_path = parameter.back();
                    } else if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::GAME_VER)) == 0) {
                        version = parameter.back();
                    }
                }
            }

            for (const auto& [key, value] : Config::FILE_EXTS) {
                if (value.second.compare(console) == 0) {
                    id = key;
                }
            }

            auto cartridge = Emulation::BaseCartridge::existing_game(title, file_name, file_path, id, version);
            if (cartridge != nullptr) {
                _games.emplace_back(cartridge);
            }

            return;
        }

        void games_to_string(const vector<Emulation::BaseCartridge*>& _games, vector<string>& _config_games) {
            _config_games.clear();
            for (const auto& n : _games) {
                _config_games.emplace_back("");
                _config_games.emplace_back("[" + n->title + "]");
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::FILE_NAME) + "=" + n->fileName);
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::FILE_PATH) + "=" + n->filePath);
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::CONSOLE) + "=" + Config::FILE_EXTS.at(n->console).second);
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::GAME_VER) + "=" + n->version);
            }
        }
    }
}
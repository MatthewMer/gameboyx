#include "config_io.h"
#include "logger.h"

#include <fstream>
#include <string>

using namespace std;

static bool check_file_exist(const char* config_path);
static bool split_line(vector<char*> param, const char* c_line);

const char* title = "title";
const char* file_name = "file_name";
const char* game_ver = "game_ver";
const char* cart_type = "cart_type";
const char* licensee = "licensee";
const char* dest_code = "dest_code";
const char* is_gbc = "is_gbc";
const char* is_sgb = "is_sgb";
const char* chksum = "chksum";


bool read_games_from_config(vector<game>& games, const char* config_path) {
    bool config_exist = check_file_exist(config_path);

    ofstream(config_path, ios_base::app).close();
    fstream fs(config_path, ios_base::in);

    if (fs.is_open()) {
        string line;
        int i, n;
        bool begin_entry = false;
        game entry;

        games.clear();

        for (i = 0, n = 0; getline(fs, line); i++) {
            const char* c_line = line.c_str();
            vector<char*> param;

            if (c_line[0] == '[' && c_line[strlen(c_line) - 1] == ']') {
                if (begin_entry) games.push_back(entry);
                entry = game();
                entry.title = new char[strlen(c_line) - 2];
                memcpy(entry.title, &c_line[1], strlen(c_line) - 2);
                begin_entry = true;
                n++;
            }
            else if (split_line(param, c_line)) {
                if (strcmp(param[0], licensee) == 0) {
                    entry.licensee = param[1];
                }
                else if (strcmp(param[0], cart_type) == 0) {
                    entry.cart_type = param[1];
                }
                else if (strcmp(param[0], is_gbc) == 0) {
                    if (strcmp(param[1], "true") == 0) entry.is_gbc = true;
                    else if (strcmp(param[1], "false") == 0) entry.is_gbc = false;
                    else LOG_ERROR("Faulty entry (semantics) on line ", i, ": \"", c_line, "\"");
                }
                else if (strcmp(param[0], is_sgb) == 0) {
                    if (strcmp(param[1], "true") == 0) entry.is_sgb = true;
                    else if (strcmp(param[1], "false") == 0) entry.is_sgb = false;
                    else LOG_ERROR("Faulty entry (semantics) on line ", i, ": \"", c_line, "\"");
                }
                else if (strcmp(param[0], dest_code) == 0) {
                    entry.dest_code = param[1];
                }
                else if (strcmp(param[0], game_ver) == 0) {
                    entry.game_ver = param[1];
                }
                else if (strcmp(param[0], chksum) == 0) {
                    entry.chksum = param[1];
                }
            }
            else if(strcmp(c_line, "") != 0) {
                LOG_ERROR("Faulty entry (syntax) on line ", i, ": \"", c_line, "\"");
            }
        }
        games.push_back(entry);

        if(!config_exist){
            LOG_WARN("No game config found. Created \"games.ini\"");
        }
        else {
            if (n == 0) LOG_INFO("Empty config file \"games.ini\" found");
            else LOG_INFO(n, " game entries loaded from \"games.ini\"");
        }

        fs.close();
        return true;
    }
    else {
        LOG_ERROR("Can't open config file \"games.ini\"");
        return false;
    }
}

bool write_game_to_config(game& new_game, const char* config_path) {
    ofstream fs(config_path, ios_base::out | ios_base::app);

    if (!fs.is_open()) return false;

    fs << "[" << new_game.title << "]" << endl;
    fs << file_name << "=" << new_game.file_name << endl;
    fs << game_ver << "=" << new_game.game_ver << endl;
    fs << cart_type << "=" << new_game.cart_type << endl;
    fs << licensee << "=" << new_game.licensee << endl;
    fs << dest_code << "=" << new_game.dest_code << endl;
    fs << file_name << "=" << new_game.file_name << endl;
    fs << is_gbc << "=" << (new_game.is_gbc ? "true" : "false") << endl;
    fs << is_sgb << "=" << (new_game.is_sgb ? "true" : "false") << endl;
    fs << chksum << "=" << new_game.chksum << endl;
    fs << endl;

    fs.close();

    LOG_INFO("New title \"", new_game.title, "\" added to games.ini");
    
    return true;
}

bool delete_game_from_config(game& new_game, const char* config_path) {
    if (check_file_exist(config_path)) {

        return true;
    }
    else {
        return false;
    }
}

static bool check_file_exist(const char* config_path) {
    if (ifstream(config_path).is_open()) {
        return true;
    }
    else {
        return false;
    }
}

static bool split_line(vector<char*> param, const char* c_line) {
    int i;
    for (i = 0; c_line[i] != '='; i++) {
        if (c_line[i] == '\0') return false;
    }
    
    param[0] = new char[i + 1];
    memcpy(param[0], c_line, i);
    param[0][i] = '\0';
    if (param[0][0] == '\0') return false;

    param[1] = new char[strlen(c_line) - i + 1];
    memcpy(param[1], &c_line[i + 1], strlen(c_line) - i);
    param[1][strlen(c_line) - i] = '\0';
    if (param[1][0] == '\0') return false;

    return true;
}
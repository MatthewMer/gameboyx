#include "games_config_io.h"
#include "logger.h"

#include <fstream>
#include <string>

using namespace std;

static bool check_file_exist(const string& config_path);
static bool split_line(vector<string>& param, const char* c_line);

const string title = "title";
const string file_name = "file_name";
const string game_ver = "game_ver";
const string cart_type = "cart_type";
const string licensee = "licensee";
const string dest_code = "dest_code";
const string is_gbc = "is_gbc";
const string is_sgb = "is_sgb";
const string file_path = "file_path";


bool read_games_from_config(vector<game_info>& games, const string& config_path) {
    bool config_exist = check_file_exist(config_path.c_str());

    ofstream(config_path, ios_base::app).close();
    fstream fs(config_path, ios_base::in);

    if (fs.is_open()) {
        string line;
        int i; int n;
        auto entry = game_info("");

        games.clear();

        for (i = 0, n = 0; getline(fs, line); i++) {
            auto c_line = line.c_str();
            vector<string> param;

            if (c_line[0] == '[' && c_line[strlen(c_line) - 1] == ']') {
                if(entry.title.compare("") != 0) games.push_back(entry);

                auto c_title = (char*)malloc(strlen(c_line) - 2);
                memcpy(c_title, &c_line[1], strlen(c_line) - 2);
                entry = game_info(c_title);
                n++;
            }
            else if (split_line(param, c_line)) {
                if (param[0].compare(licensee) == 0) {
                    entry.licensee = param[1];
                }
                else if (param[0].compare(cart_type) == 0) {
                    entry.cart_type = param[1];
                }
                else if (param[0].compare(is_gbc) == 0) {
                    if (param[1].compare("true") == 0) entry.is_gbc = true;
                    else if (param[1].compare("false") == 0) entry.is_gbc = false;
                    else LOG_ERROR("Faulty entry (semantics) on line ", i, ": \"", c_line, "\"");
                }
                else if (param[0].compare(is_sgb) == 0) {
                    if (param[1].compare("true") == 0) entry.is_sgb = true;
                    else if (param[1].compare("false") == 0) entry.is_sgb = false;
                    else LOG_ERROR("Faulty entry (semantics) on line ", i, ": \"", c_line, "\"");
                }
                else if (param[0].compare(dest_code) == 0) {
                    entry.dest_code = param[1];
                }
                else if (param[0].compare(game_ver) == 0) {
                    entry.game_ver = param[1];
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

bool write_game_to_config(const game_info& game_ctx, const string& config_path) {
    ofstream fs(config_path, ios_base::out | ios_base::app);

    LOG_INFO("Writing new data to games.ini");

    if (!fs.is_open()) return false;

    fs << endl;
    fs << "[" << game_ctx.title << "]" << endl;
    fs << file_name << "=" << game_ctx.file_name << endl;
    fs << file_path << "=" << game_ctx.file_path << endl;
    fs << game_ver << "=" << game_ctx.game_ver << endl;
    fs << is_gbc << "=" << (game_ctx.is_gbc ? "true" : "false") << endl;
    fs << is_sgb << "=" << (game_ctx.is_sgb ? "true" : "false") << endl;
    fs << cart_type << "=" << game_ctx.cart_type << endl;
    fs << licensee << "=" << game_ctx.licensee << endl;
    fs << dest_code << "=" << game_ctx.dest_code << endl;

    fs.close();

    LOG_INFO("New title \"", game_ctx.title, "\" added to games.ini");
    
    return true;
}

bool delete_game_from_config(const game_info& game_ctx, const string& config_path) {
    if (check_file_exist(config_path)) {

        return true;
    }
    else {
        return false;
    }
}

static bool check_file_exist(const string& config_path) {
    if (ifstream(config_path).is_open()) {
        return true;
    }
    else {
        return false;
    }
}

static bool split_line(vector<string>& param, const char* c_line) {
    string line_copy(c_line);

    string delimiter = "=";
    int count = 0;
    while (line_copy.find(delimiter) != string::npos) {
        param.push_back(line_copy.substr(0, line_copy.find(delimiter)));
        line_copy.erase(0, line_copy.find(delimiter) + delimiter.length());
        count++;
    }

    return count == 2;
}
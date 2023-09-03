#include "config_io.h"

#include "game_info.h"
#include "logger.h"
#include "helper_functions.h"
#include "config.h"

#include <fstream>
#include <string>

using namespace std;

bool read_games_from_config(vector<game_info>& games, const string& games_config_path) {
    if (!check_and_create_file(games_config_path)) { return false; }

    string full_config_path = get_current_path() + games_config_path;

    ifstream is(full_config_path, ios::beg);
    if (!is) { return false; }

    game_info game_ctx;
    string line;

    for (int line_count = 1; getline(is, line); line_count++) {
        if (line.compare("") == 0) continue;

        line = trim(line);

        // find start of entry
        if (line.find("[") == 0 && line.find("]") == line.length() - 1) {
            if (game_ctx.title.compare("") != 0) {
                games.push_back(game_ctx);
            }

            game_ctx = game_info(line.substr(1, line.length() - 2));
        } 
        // filter entry parameters
        else if (game_ctx.title.compare("") != 0) {
            const auto parameter = split_string(line, "=");

            if (parameter.size() != 2) {
                LOG_WARN("Faulty parameter on line ", line_count);
                continue;
            }

            switch (get_info_type_enum(trim(parameter[0]))) {
            case TITLE:
                game_ctx.title = trim(parameter[1]);
                break;
            case LICENSEE:
                game_ctx.licensee = trim(parameter[1]);
                break;
            case CART_TYPE:
                game_ctx.cart_type = trim(parameter[1]);
                break;
            case IS_CGB: {
                string value = trim(parameter[1]);
                bool result = (value.compare(parameter_true) == 0 ? true : false);
                if (!result && value.compare(parameter_false) != 0) {
                    LOG_WARN("Faulty boolean on line ", line_count, ", fallback to false");
                }
                game_ctx.is_cgb = result;
            }
                break;
            case IS_SGB: {
                string value = trim(parameter[1]);
                bool result = (value.compare(parameter_true) == 0 ? true : false);
                if (!result && value.compare(parameter_false) != 0) {
                    LOG_WARN("Faulty boolean on line ", line_count, ", fallback to false");
                }
                game_ctx.is_sgb = result;
            }
                break;
            case DEST_CODE:
                game_ctx.dest_code = trim(parameter[1]);
                break;
            case GAME_VER:
                game_ctx.game_ver = trim(parameter[1]);
                break;
            case CHKSUM_PASSED: {
                string value = trim(parameter[1]);
                bool result = (value.compare(parameter_true) == 0 ? true : false);
                if (!result && value.compare(parameter_false) != 0) {
                    LOG_WARN("Faulty boolean on line ", line_count, ", fallback to false");
                }
                game_ctx.chksum_passed = result;
            }
                break;
            case FILE_NAME:
                game_ctx.file_name = trim(parameter[1]);
                break;
            case FILE_PATH:
                game_ctx.file_path = trim(parameter[1]);
                break;
            case NONE_INFO_TYPE:
            default:
                LOG_ERROR("Unknown internal info type for line ", line_count);
                continue;
                break;
            }
        }
    }
    games.push_back(game_ctx);

    LOG_INFO(games.size(), " game entries found in .", games_config_path);

    return true;
}



bool write_game_to_config(const game_info& game_ctx, const string& config_path) {
    ofstream os(config_path, ios_base::out | ios_base::app);

    LOG_INFO("Writing new data to games.ini");

    if (!os.is_open()) return false;

    os << endl;
    os << "[" << game_ctx.title << "]" << endl;
    os << get_info_type_string(FILE_NAME) << "=" << game_ctx.file_name << endl;
    os << get_info_type_string(FILE_PATH) << "=" << game_ctx.file_path << endl;
    os << get_info_type_string(GAME_VER) << "=" << game_ctx.game_ver << endl;
    os << get_info_type_string(IS_CGB) << "=" << (game_ctx.is_cgb ? parameter_true : parameter_false) << endl;
    os << get_info_type_string(IS_SGB) << "=" << (game_ctx.is_sgb ? parameter_true : parameter_false) << endl;
    os << get_info_type_string(CART_TYPE) << "=" << game_ctx.cart_type << endl;
    os << get_info_type_string(LICENSEE) << "=" << game_ctx.licensee << endl;
    os << get_info_type_string(DEST_CODE) << "=" << game_ctx.dest_code << endl;

    os.close();

    return true;
}

bool delete_game_from_config(vector<game_info>& games, const game_info& game_ctx, const string& config_path) {
    if (!check_and_create_file(config_path + games_config_file)) { return false; }


    return true;
}

bool write_new_game(const game_info& game_ctx) {
    string current_path = get_current_path();
    string s_path_config = current_path + config_folder;
    return write_game_to_config(game_ctx, s_path_config + games_config_file);
}

bool check_and_create_folders() {
    if (!check_and_create_path(rom_folder)) { return false; }
    if (!check_and_create_path(config_folder)) { return false; }
    return true;
}

bool check_and_create_files() {
    if (!check_and_create_file(config_folder + games_config_file)) { return false; }
    return true;
}
#include "config_io.h"
#include "game_info.h"
#include "logger.h"
#include "helper_functions.h"

#include <fstream>
#include <string>

using namespace std;

bool read_games_from_config(vector<game_info>& games, const string& config_path) {
    if (!check_and_create_file(config_path)) {
        ofstream(config_path).close();
        return false;
    }

    ifstream is(config_path, ios::beg);
    if (!is) { return false; }

    game_info game_ctx;
    string line;
    while (getline(is, line)) {
        if (line.compare("") == 0) continue;

        // TODO: rewrite games config read
        auto vec_line = split_string(line, "=");
    }


    
}

bool write_game_to_config(const game_info& game_ctx, const string& config_path) {
    ofstream os(config_path, ios_base::out | ios_base::app);

    LOG_INFO("Writing new data to games.ini");

    if (!os.is_open()) return false;

    os << endl;
    os << "[" << game_ctx.title << "]" << endl;
    os << s_file_name << "=" << game_ctx.file_name << endl;
    os << s_file_path << "=" << game_ctx.file_path << endl;
    os << s_game_ver << "=" << game_ctx.game_ver << endl;
    os << s_is_cgb << "=" << (game_ctx.is_cgb ? "true" : "false") << endl;
    os << s_is_sgb << "=" << (game_ctx.is_sgb ? "true" : "false") << endl;
    os << s_cart_type << "=" << game_ctx.cart_type << endl;
    os << s_licensee << "=" << game_ctx.licensee << endl;
    os << s_dest_code << "=" << game_ctx.dest_code << endl;

    os.close();

    return true;
}

bool delete_game_from_config(const game_info& game_ctx, const string& config_path) {
    if (!check_and_create_file(config_path)) return false;


    return true;
}

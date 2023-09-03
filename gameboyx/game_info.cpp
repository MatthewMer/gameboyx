#include "game_info.h"

using namespace std;

string get_licensee(const u8& new_licensee, const string& licensee_code) {
    if (new_licensee == 0x33) {
        for (const auto& [code, licensee] : new_lic_map) {
            if (licensee_code.compare(code) == 0) {
                return licensee;
            }
        }
        return n_a;
    }
    else {
        return old_lic;
    }
}

string get_cart_type(const u8& cart_type) {
    for (const auto& [code, type] : cart_type_map) {
        if (code == cart_type) {
            return type;
        }
    }
    return n_a;
}

string get_dest_code(const u8& dest_code) {
    for (const auto& [code, dest] : dest_code_map) {
        if (code == dest_code) {
            return dest;
        }
    }
    return n_a;
}

string get_full_file_path(const game_info& game_ctx) {
    return (game_ctx.file_path + game_ctx.file_name);
}

bool check_ext(const string& file) {
    string delimiter = ".";
    int ext_start = (int)file.find(delimiter) + 1;
    string file_ext = file.substr(ext_start, file.length() - ext_start);

    for (const auto& n : file_exts) {
        if (file_ext.compare(n[1]) == 0) return true;
    }

    return false;
}
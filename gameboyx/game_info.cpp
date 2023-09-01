#include "game_info.h"

using namespace std;

bool operator==(const game_info& n, const game_info& m)
{
    return (n.title == m.title &&
        n.licensee == m.licensee &&
        n.cart_type == m.cart_type &&
        n.dest_code == m.dest_code &&
        n.file_name == m.file_name &&
        n.file_path == m.file_path &&
        n.is_gbc == m.is_sgb &&
        n.is_gbc == m.is_gbc &&
        n.game_ver == m.game_ver &&
        n.chksum == m.chksum &&
        n.chksum_passed == m.chksum_passed);
}

string get_full_file_path(const game_info& game_ctx) {
	return (game_ctx.file_path + game_ctx.file_name);
}

string get_licensee(const byte& new_licensee, const string& licensee_code) {
    if (new_licensee == byte{ 0x33 }) {
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

string get_cart_type(const byte& cart_type) {
    for (const auto& [code, type] : cart_type_map) {
        if (code == cart_type) {
            return type;
        }
    }
    return n_a;
}

string get_dest_code(const byte& dest_code) {
    for (const auto& [code, dest] : dest_code_map) {
        if (code == dest_code) {
            return dest;
        }
    }
    return n_a;
}
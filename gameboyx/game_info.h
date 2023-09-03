#pragma once

#include "defs.h"
#include <vector>
#include <string>

const inline std::vector<std::vector<std::string>> file_exts = {
    {"Gameboy Color", "gbc"}, {"Gameboy", "gb"}
};

const inline std::string old_lic = "Old licensee";
const inline std::string n_a = "N/A";

const inline std::string parameter_true = "true";
const inline std::string parameter_false = "false";

enum info_types{
    NONE_INFO_TYPE,
    TITLE,
    LICENSEE,
    CART_TYPE,
    IS_CGB,
    IS_SGB,
    DEST_CODE,
    GAME_VER,
    CHKSUM_PASSED,
    FILE_NAME,
    FILE_PATH
};

struct game_info {
	std::string title = "";
    std::string licensee = "";
    std::string cart_type = "";
	bool is_cgb = false;
	bool is_sgb = false;
    std::string dest_code = "";
	std::string game_ver = "";
	bool chksum_passed = "";
    std::string file_name = "";
    std::string file_path = "";

    explicit game_info(const std::string& title) : title(title) {};
    game_info() = default;
};

inline bool operator==(const game_info& n, const game_info& m) {
    return n.title.compare(m.title) == 0 &&
        n.licensee.compare(m.licensee) == 0 &&
        n.cart_type.compare(m.cart_type) == 0 &&
        n.is_cgb == m.is_cgb &&
        n.is_sgb == m.is_sgb &&
        n.dest_code.compare(m.dest_code) == 0 &&
        n.game_ver.compare(m.game_ver) == 0 &&
        n.chksum_passed == m.chksum_passed &&
        n.file_name.compare(m.file_name) == 0 &&
        n.file_path.compare(m.file_path) == 0;
}

std::string get_full_file_path(const game_info& game_ctx);
std::string get_licensee(const u8& new_licensee, const std::string& licensee_code);
std::string get_cart_type(const u8& cart_type);
std::string get_dest_code(const u8& dest_code);
std::string get_info_type_string(const info_types& info_type);
info_types get_info_type_enum(const std::string& info_type_string);
bool check_game_info_integrity(const game_info& game_ctx);
bool check_ext(const std::string& file);
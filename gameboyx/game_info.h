#pragma once
/* ***********************************************************************************************************
    DESCRIPTION
*********************************************************************************************************** */
/*
*	game information used for GUI, reading and writing game related data to config and selecting appropriate hardware
*   in VHardwareMgr class. Will get changed to fit every type of console, currently tied to the gameboy (color).
*/

#include "defs.h"
#include <vector>
#include <string>

inline const std::vector<std::vector<std::string>> FILE_EXTS = {
    {"Gameboy Color", "gbc"}, {"Gameboy", "gb"}
};

inline const std::string OLD_LIC = "Old licensee";
inline const std::string N_A = "N/A";

inline const std::string PARAMETER_TRUE = "true";
inline const std::string PARAMETER_FALSE = "false";

enum info_types {
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

    //std::string date_last_played = "";

    explicit constexpr game_info(const std::string& _title) : title(_title) {};
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

std::string get_full_file_path(const game_info& _game_ctx);
std::string get_licensee(const u8& _new_licensee, const std::string& _licensee_code);
std::string get_cart_type(const u8& _cart_type);
std::string get_dest_code(const u8& _dest_code);
std::string get_info_type_string(const info_types& _info_type);
info_types get_info_type_enum(const std::string& _info_type_string);
bool filter_parameter_game_info_enum(game_info& _game_ctx, const std::vector<std::string>& _parameter);
bool check_game_info_integrity(const game_info& _game_ctx);
bool check_ext(const std::string& _file);
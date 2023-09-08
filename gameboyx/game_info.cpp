#include "game_info.h"

using namespace std;

const std::vector<std::pair<u8, std::string>> CART_TYPE_MAP{
    { 0x00,"ROM ONLY"},
    { 0x01,"MBC1"},
    { 0x02,"MBC1+RAM"},
    { 0x03,"MBC1+RAM+BATTERY"},
    { 0x05,"MBC2"},
    { 0x06,"MBC2+BATTERY"},
    { 0x08,"ROM+RAM"},
    { 0x09,"ROM+RAM+BATTERY"},
    { 0x0B,"MMM01"},
    { 0x0C,"MMM01+RAM"},
    { 0x0D,"MMM01+RAM+BATTERY"},
    { 0x0F,"MBC3+TIMER+BATTERY"},
    { 0x10,"MBC3+TIMER+RAM+BATTERY"},
    { 0x11,"MBC3"},
    { 0x12,"MBC3+RAM"},
    { 0x13,"MBC3+RAM+BATTERY"},
    { 0x19,"MBC5"},
    { 0x1A,"MBC5+RAM"},
    { 0x1B,"MBC5+RAM+BATTERY"},
    { 0x1C,"MBC5+RUMBLE"},
    { 0x1D,"MBC5+RUMBLE+RAM"},
    { 0x1E,"MBC5+RUMBLE+RAM+BATTERY"},
    { 0x20,"MBC6"},
    { 0x22,"MBC7+SENSOR+RUMBLE+RAM+BATTERY"},
    { 0xFC,"POCKET CAMERA"},
    { 0xFD,"BANDAI TAMA5"},
    { 0xFE,"HuC3"},
    { 0xFF,"HuC1+RAM+BATTERY"}
};

const std::vector<std::pair<std::string, std::string>> NEW_LIC_MAP{           // only when old is set to 0x33
    {"00", "None"},
    {"01", "Nintendo R&D1"},
    {"08", "Capcom"},
    {"13", "Electronic Arts"},
    {"18", "Hudson Soft"},
    {"19", "b-ai"},
    {"20", "kss"},
    {"22", "pow"},
    {"24", "PCM Complete"},
    {"25", "san-x"},
    {"28", "Kemco Japan"},
    {"29", "seta"},
    {"30", "Viacom"},
    {"31", "Nintendo"},
    {"32", "Bandai"},
    {"33", "Ocean/Acclaim"},
    {"34", "Konami"},
    {"35", "Hector"},
    {"37", "Taito"},
    {"38", "Hudson"},
    {"39", "Banpresto"},
    {"41", "Ubi Soft"},
    {"42", "Atlus"},
    {"44", "Malibu"},
    {"46", "angel"},
    {"47", "Bullet-Proof"},
    {"49", "irem"},
    {"50", "Absolute"},
    {"51", "Acclaim"},
    {"52", "Activision"},
    {"53", "American sammy"},
    {"54", "Konami"},
    {"55", "Hi tech entertainment"},
    {"56", "LJN"},
    {"57", "Matchbox"},
    {"58", "Mattel"},
    {"59", "Milton Bradley"},
    {"60", "Titus"},
    {"61", "Virgin"},
    {"64", "LucasArts"},
    {"67", "Ocean"},
    {"69", "Electronic Arts"},
    {"70", "Infogrames"},
    {"71", "Interplay"},
    {"72", "Broderbund"},
    {"73", "sculptured"},
    {"75", "sci"},
    {"78", "THQ"},
    {"79", "Accolade"},
    {"80", "misawa"},    
    {"83", "lozc"},
    {"86", "Tokuma Shoten Intermedia"},
    {"87", "Tsukuda Original"},
    {"91", "Chunsoft"},
    {"92", "Video system"},
    {"93", "Ocean/Acclaim"},
    {"95", "Varie"},
    {"96", "Yonezawa/s’pal"},
    {"97", "Kaneko"},
    {"99", "Pack in soft"},
    {"A4", "Konami (Yu-Gi-Oh!)"}
};

const std::vector<std::pair<u8, std::string>> DEST_CODE_MAP{
    { 0x00, "Japan"},
    { 0x01, "Western world"}
};

const std::vector<std::pair<info_types, std::string>> INFO_TYPES_MAP = {
    { TITLE, "title" },
    { LICENSEE,"licensee" },
    { CART_TYPE,"cart_type" },
    { IS_CGB,"is_cgb" },
    { IS_SGB,"is_sgb" },
    { DEST_CODE,"dest_code" },
    { GAME_VER,"game_ver" },
    { CHKSUM_PASSED,"chksum_passed" },
    { FILE_NAME,"file_name" },
    { FILE_PATH,"file_path" }
};

string get_licensee(const u8& _new_licensee, const string& _licensee_code) {
    if (_new_licensee == 0x33) {
        for (const auto& [code, licensee] : NEW_LIC_MAP) {
            if (_licensee_code.compare(code) == 0) {
                return licensee;
            }
        }
        return N_A;
    }
    else {
        return OLD_LIC;
    }
}

string get_cart_type(const u8& _cart_type) {
    for (const auto& [code, type] : CART_TYPE_MAP) {
        if (code == _cart_type) {
            return type;
        }
    }
    return N_A;
}

string get_dest_code(const u8& _dest_code) {
    for (const auto& [code, dest] : DEST_CODE_MAP) {
        if (code == _dest_code) {
            return dest;
        }
    }
    return N_A;
}

string get_full_file_path(const game_info& _game_ctx) {
    return (_game_ctx.file_path + _game_ctx.file_name);
}

bool check_ext(const string& _file) {
    string delimiter = ".";
    int ext_start = (int)_file.find(delimiter) + 1;
    string file_ext = _file.substr(ext_start, _file.length() - ext_start);

    for (const auto& n : FILE_EXTS) {
        if (file_ext.compare(n[1]) == 0) return true;
    }

    return false;
}

string get_info_type_string(const info_types& _info_type) {
    for (const auto& [type, s_type] : INFO_TYPES_MAP) {
        if (_info_type == type) return s_type;
    }
    
    return string("");
}

info_types get_info_type_enum(const std::string& _info_type_string) {
    for (const auto& [type, s_type] : INFO_TYPES_MAP) {
        if (_info_type_string.compare(s_type) == 0) return type;
    }
    
    return NONE_INFO_TYPE;
}

bool check_game_info_integrity(const game_info& _game_ctx) {
    if (_game_ctx.title.compare("") != 0 &&
        _game_ctx.licensee.compare("") != 0 &&
        _game_ctx.cart_type.compare("") != 0 &&
        _game_ctx.dest_code.compare("") != 0 &&
        _game_ctx.game_ver.compare("") != 0 &&
        _game_ctx.file_name.compare("") != 0 &&
        _game_ctx.file_path.compare("") != 0)
    {
        return true;
    }
    else {
        return false;
    }
}
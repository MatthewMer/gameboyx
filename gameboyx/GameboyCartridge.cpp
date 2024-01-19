#include "GameboyCartridge.h"

#include "general_config.h"
#include "gameboy_defines.h"
#include "logger.h"
#include "data_io.h"
#include "helper_functions.h"

#include <fstream>

using namespace std;

const std::vector<std::pair<u8, std::string>> CART_TYPE_MAP = {
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

const std::vector<std::pair<std::string, std::string>> NEW_LIC_MAP = {           // only when old is set to 0x33
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

const std::vector<std::pair<u8, std::string>> DEST_CODE_MAP = {
    { 0x00, "Japan"},
    { 0x01, "Western world"}
};

inline const std::string OLD_LIC = "Old licensee";

std::string get_licensee(const u8& _new_licensee, const std::string& _licensee_code) {
    if (_new_licensee == 0x33) {
        for (const auto& [code, licensee] : NEW_LIC_MAP) {
            if (code.compare(_licensee_code) == 0) {
                return licensee;
            }
        }
        return N_A;
    } else {
        return OLD_LIC;
    }
}

std::string get_cart_type(const u8& _cart_type) {
    for (const auto& [code, type] : CART_TYPE_MAP) {
        if (code == _cart_type) {
            return type;
        }
    }
    return N_A;
}



bool GameboyCartridge::ReadRom() {
    auto vec_rom = vector<char>();

    if (!read_data(vec_rom, filePath + fileName)) {
        LOG_ERROR("[emu] error while reading rom");
        return false;
    }

    vecRom.clear();
    vecRom = vector<u8>(vec_rom.begin(), vec_rom.end());

    // read header info -----
    LOG_INFO("[emu] Reading header info");

    // cgb/sgb flags
    bool is_cgb = vecRom[ROM_HEAD_CGBFLAG] & 0x80;
    bool is_sgb = vecRom[ROM_HEAD_SGBFLAG] == 0x03;

    if (!((is_cgb && console == GBC) || (!is_cgb && console == GB))) {
        LOG_WARN("[emu] console type mismatch");
        LOG_WARN("[emu] setting to type expected by header");

        if (is_cgb) {
            console = GBC;
        } else {
            console = GB;
        }
    }

    // title
    title = "";
    int title_size_max = (is_cgb ? ROM_HEAD_TITLE_SIZE_CGB : ROM_HEAD_TITLE_SIZE_GB);
    for (int i = 0; vecRom[ROM_HEAD_TITLE + i] != 0x00 && i < title_size_max; i++) {
        title += (char)vecRom[ROM_HEAD_TITLE + i];
    }

    // checksum
    u8 chksum_expected = vecRom[ROM_HEAD_CHKSUM];
    u8 chksum_calulated = 0;
    for (int i = ROM_HEAD_TITLE; i < ROM_HEAD_CHKSUM; i++) {
        chksum_calulated -= (vecRom[i] + 1);
    }
    LOG_INFO("[emu] checksum passed: ", (chksum_expected == chksum_calulated ? PARAMETER_TRUE : PARAMETER_FALSE));

    // game version
    version = to_string(vecRom[ROM_HEAD_VERSION]);

    return true;
}
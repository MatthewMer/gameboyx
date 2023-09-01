#pragma once

#include "defs.h"
#include <vector>
#include <string>

const static std::vector<std::pair<const std::byte, const std::string>> cart_type_map{
    {std::byte{0x00},"ROM ONLY"},
    {std::byte{0x01},"MBC1"},
    {std::byte{0x02},"MBC1+RAM"},
    {std::byte{0x03},"MBC1+RAM+BATTERY"},
    {std::byte{0x05},"MBC2"},
    {std::byte{0x06},"MBC2+BATTERY"},
    {std::byte{0x08},"ROM+RAM"},
    {std::byte{0x09},"ROM+RAM+BATTERY"},
    {std::byte{0x0B},"MMM01"},
    {std::byte{0x0C},"MMM01+RAM"},
    {std::byte{0x0D},"MMM01+RAM+BATTERY"},
    {std::byte{0x0F},"MBC3+TIMER+BATTERY"},
    {std::byte{0x10},"MBC3+TIMER+RAM+BATTERY"},
    {std::byte{0x11},"MBC3"},
    {std::byte{0x12},"MBC3+RAM"},
    {std::byte{0x13},"MBC3+RAM+BATTERY"},
    {std::byte{0x19},"MBC5"},
    {std::byte{0x1A},"MBC5+RAM"},
    {std::byte{0x1B},"MBC5+RAM+BATTERY"},
    {std::byte{0x1C},"MBC5+RUMBLE"},
    {std::byte{0x1D},"MBC5+RUMBLE+RAM"},
    {std::byte{0x1E},"MBC5+RUMBLE+RAM+BATTERY"},
    {std::byte{0x20},"MBC6"},
    {std::byte{0x22},"MBC7+SENSOR+RUMBLE+RAM+BATTERY"},
    {std::byte{0xFC},"POCKET CAMERA"},
    {std::byte{0xFD},"BANDAI TAMA5"},
    {std::byte{0xFE},"HuC3"},
    {std::byte{0xFF},"HuC1+RAM+BATTERY"}
};

const static std::vector<std::pair<const std::string, const std::string>> new_lic_map{           // only when old is set to 0x33
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

const static std::string old_lic = "Old licensee";

const static std::string n_a = "N/A";

const static std::vector<std::pair<std::byte, std::string>> dest_code_map{
    {std::byte{0x00}, "Japan"},
    {std::byte{0x01}, "Western world"}
};

struct game_info {
	std::string title;
    std::string licensee;
    std::string cart_type;
	bool is_gbc;
	bool is_sgb;
    std::string dest_code;
	std::string game_ver;
	std::string chksum;
	bool chksum_passed;
    std::string file_name;
    std::string file_path;

    explicit game_info(const std::string& title) : title(title) {};
    game_info() = default;
};

bool operator==(const game_info& n, const game_info& m);
std::string get_full_file_path(const game_info& game_ctx);
std::string get_licensee(const std::byte& new_licensee, const std::string& licensee_code);
std::string get_cart_type(const std::byte& cart_type);
std::string get_dest_code(const std::byte& dest_code);
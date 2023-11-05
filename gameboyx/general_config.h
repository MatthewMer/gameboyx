#pragma once

#include "imgui.h"
#include "defs.h"
#include <vector>
#include <string>
#include <unordered_map>

/* ***********************************************************************************************************
    STORAGE DEFINES
*********************************************************************************************************** */
#define _DEBUG_GBX

inline const std::string ROM_FOLDER = "/rom/";

inline const std::string CONFIG_FOLDER = "/config/";
inline const std::string GAMES_CONFIG_FILE = "games.ini";

inline const std::string LOG_FOLDER = "/logs/";
inline const std::string DEBUG_INSTR_LOG = "_instructions.log";

inline const std::string SHADER_FOLDER = "/shader/";
//inline const std::string SHADER_BYTECODE_FOLDER = "/shaderbytecode/";

/* ***********************************************************************************************************
    GRAPHICS BACKEND
*********************************************************************************************************** */
#define GBX_RELEASE					"PRE-ALPHA"
#define GBX_VERSION_MAJOR			0
#define GBX_VERSION_MINOR			0
#define GBX_VERSION_PATCH           0
#define GBX_AUTHOR					"MatthewMer"

inline const u16 ID_NVIDIA = 0x10DE;
inline const u16 ID_AMD = 0x1002;

inline const std::vector<std::pair<u32, std::string>> VENDOR_IDS = {
    {0x1002, "AMD"},
    {0x10DE, "NVIDIA"},
    {0x1043, "ASUS"},
    {0x196D, "Club 3D"},
    {0x1092, "Diamond Multimedia"},
    {0x18BC, "GeCube"},
    {0x1458, "Gigabyte"},
    {0x17AF, "HIS"},
    {0x16F3, "Jetway"},
    {0x1462, "MSI"},
    {0x1DA2, "Sapphire"},
    {0x148C, "PowerColor"},
    {0x1545, "VisionTek"},
    {0x1682, "XFX"},
    {0x1025, "Acer"},
    {0x106B, "Apple"},
    {0x1028, "Dell"},
    {0x107B, "Gateway"},
    {0x103C, "HP"},
    {0x17AA, "Lenovo"},
    {0x104D, "Sony"},
    {0x1179, "Toshiba"}
};

inline std::string get_vendor(const u16& _ven_id) {
    for (const auto& n : VENDOR_IDS) {
        if (n.first == _ven_id) { return n.second; }
    }

    return "";
}

/* ***********************************************************************************************************
    IMGUI EMULATOR
*********************************************************************************************************** */
#define GUI_WIN_WIDTH               1280
#define GUI_WIN_HEIGHT              720

inline const std::string APP_TITLE = "GameboyX";

inline const std::vector<std::pair<std::string, float>> GAMES_COLUMNS = { 
    {"", 1 / 12.f},
    {"Console", 1 / 12.f},
    {"Title", 1 / 6.f},
    {"File path", 1 / 3.f},
    {"File name", 4 / 12.f},
};

inline const ImVec2 debug_instr_win_size(500.f, 0.f);
inline const std::vector<float> DEBUG_INSTR_COLUMNS = {
    1.f,
    7.f,
    7.f
};
inline const std::vector<float> DEBUG_REGISTER_COLUMNS = {
    1.f,
    2.f,
    1.f,
    2.f
};
inline const std::vector<float> DEBUG_FLAG_COLUMNS = {
    1.f,
    2.f,
    1.f,
    2.f
};

inline const ImVec2 hw_info_win_size(250.f, 0.f);
inline const std::vector<float> HW_INFO_COLUMNS = {
    1.f,
    1.f
};

inline const ImVec2 debug_mem_win_size(550.f, 0.f);
inline const std::vector<std::pair<std::string, float>> DEBUG_MEM_COLUMNS{
    {"", 2.f},
    {"00", 1.f},
    {"01", 1.f},
    {"02", 1.f},
    {"03", 1.f},
    {"04", 1.f},
    {"05", 1.f},
    {"06", 1.f},
    {"07", 1.f},
    {"08", 1.f},
    {"09", 1.f},
    {"0a", 1.f},
    {"0b", 1.f},
    {"0c", 1.f},
    {"0d", 1.f},
    {"0e", 1.f},
    {"0f", 1.f},
};

inline const std::vector<std::string> GAMES_CONSOLES = { "Gameboy", "Gameboy Color" };

#define BG_CHANNEL_COL .1f
inline const ImVec4 IMGUI_BG_COLOR(BG_CHANNEL_COL, BG_CHANNEL_COL, BG_CHANNEL_COL, 1.f);

inline const ImVec4 IMGUI_RED_COL(1.f, .0f, .0f, 1.f);
inline const ImVec4 IMGUI_GREEN_COL(.0f, 1.f, .0f, 1.f);
inline const ImVec4 IMGUI_BLUE_COLOR(.0f, .0f, 1.f, 1.f);

inline const float OVERLAY_DISTANCE = 10.0f;

#define FPS_SAMPLES_NUM             100

#define DEBUG_INSTR_LINES           21
#define DEBUG_MEM_LINES             32
#define DEBUG_MEM_ELEM_PER_LINE     0x10

inline const ImGuiWindowFlags MAIN_WIN_FLAGS =
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoBringToFrontOnFocus;

inline const ImGuiTableFlags MAIN_TABLE_FLAGS =
    ImGuiTableFlags_ScrollY |
    ImGuiTableFlags_BordersInnerV;

inline const ImGuiWindowFlags WIN_CHILD_FLAGS =
    ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_AlwaysAutoResize;

inline const ImGuiWindowFlags WIN_OVERLAY_FLAGS = 
    ImGuiWindowFlags_NoDecoration | 
    ImGuiWindowFlags_AlwaysAutoResize | 
    ImGuiWindowFlags_NoFocusOnAppearing | 
    ImGuiWindowFlags_NoNav |
    ImGuiWindowFlags_NoMove;

inline const ImGuiTableFlags TABLE_FLAGS =
    ImGuiTableFlags_BordersInnerV |
    ImGuiTableFlags_BordersOuterH;

inline const ImGuiTableFlags TABLE_FLAGS_BORDER_INNER_H =
    ImGuiTableFlags_BordersInnerV |
    ImGuiTableFlags_BordersOuterH |
    ImGuiTableFlags_BordersInnerH;

inline const ImGuiTableFlags TABLE_FLAGS_NO_BORDER_OUTER_H = ImGuiTableFlags_BordersInnerV;

inline const ImGuiTableColumnFlags TABLE_COLUMN_FLAGS_NO_HEADER =
    ImGuiTableColumnFlags_NoHeaderLabel |
    ImGuiTableColumnFlags_NoResize |
    ImGuiTableColumnFlags_WidthStretch;

inline const ImGuiTableColumnFlags TABLE_COLUMN_FLAGS =
    ImGuiTableColumnFlags_NoResize |
    ImGuiTableColumnFlags_WidthStretch;

inline const ImGuiInputTextFlags INPUT_INT_HEX_FLAGS =
    ImGuiInputTextFlags_CharsHexadecimal |
    ImGuiInputTextFlags_CharsUppercase |
    ImGuiInputTextFlags_EnterReturnsTrue;

inline const ImGuiInputTextFlags INPUT_INT_FLAGS = ImGuiInputTextFlags_EnterReturnsTrue;

inline const ImGuiSelectableFlags SEL_FLAGS = ImGuiSelectableFlags_SpanAllColumns;
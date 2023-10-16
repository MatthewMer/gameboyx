#pragma once

#include "imgui.h"
#include <vector>
#include <string>

/* ***********************************************************************************************************
    GBX DEFINES
*********************************************************************************************************** */
#define _DEBUG_GBX

#define GBX_RELEASE					"PRE-ALPHA"
#define GBX_VERSION_MAJOR			0
#define GBX_VERSION_MINOR			0
#define GBX_VERSION_PATCH           0
#define GBX_AUTHOR					"MatthewMer"

/* ***********************************************************************************************************
    IMGUI GBX CONSTANTS/DEFINES
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
    7.f,
    1.f,
    7.f
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

#define BG_COL 0.1f
#define CLR_COL 0.0f
inline const ImVec4 IMGUI_BG_COLOR(BG_COL, BG_COL, BG_COL, 1.0f);
inline const ImVec4 IMGUI_CLR_COLOR(CLR_COL, CLR_COL, CLR_COL, 1.0f);

#define DEBUG_INSTR_LINES           21
#define DEBUG_MEM_LINES             21
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
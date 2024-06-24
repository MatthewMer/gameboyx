#pragma once
/* ***********************************************************************************************************
    DESCRIPTION
*********************************************************************************************************** */
/*
*	contains defines and variabkle definitions used at various spots, mainly related to the imgui GUI layout, smaller backends
*   or overrall information that doesn't change and could be used multiple times without being directly tied to a specific source file
*/

#include "../submodules/imgui/imgui.h"
#include "defs.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <algorithm>

namespace Config {
    /* ***********************************************************************************************************
        STORAGE DEFINES
    *********************************************************************************************************** */
#ifdef GBX_INSTALLER
    inline std::string replace_char(std::string _in, char _o, char _n) {
        std::string res = _in;
        std::replace(res.begin(), res.end(), _o, _n);
        return res;
    }

    inline const std::string APPDATA = replace_char(std::string(getenv("APPDATA")), '\\', '/');
    inline const std::string APPDATA_GBX = APPDATA + std::string("/GameboyX/");

    inline const std::string ROM_FOLDER = APPDATA_GBX + "rom/";
    inline const std::string CONFIG_FOLDER = APPDATA_GBX + "config/";
    inline const std::string LOG_FOLDER = APPDATA_GBX + "logs/";
    inline const std::string SHADER_FOLDER = APPDATA_GBX + "shader/";
    inline const std::string SPIR_V_FOLDER = APPDATA_GBX + "shader/cache/";
    inline const std::string SAVE_FOLDER = APPDATA_GBX + "save/";
#else
    inline const std::string ROM_FOLDER = "rom/";
    inline const std::string CONFIG_FOLDER = "config/";
    inline const std::string LOG_FOLDER = "logs/";
    inline const std::string SHADER_FOLDER = "shader/";
    inline const std::string SPIR_V_FOLDER = "shader/cache/";
    inline const std::string SAVE_FOLDER = "save/";
#endif

    inline const std::string GAMES_CONFIG_FILE = "games.ini";
    inline const std::string DEBUG_INSTR_LOG = "_instructions.log";

    inline const std::string CONTROL_FOLDER = "control/";
    inline const std::string CONTROL_DB = "gamecontrollerdb.txt";

    inline const std::string SAVE_EXT = ".sav";

    inline const std::string ICON_FOLDER = "icon/";
    inline const std::string ICON_FILE = "gameboyx.bmp";

    inline const std::string BOOT_FOLDER = "boot/";

    inline const std::string FONT_FOLDER = "font/";
    inline const std::string FONT_MAIN = "LTSuperiorMono-Medium.otf";

    inline const std::string CURSOR_FOLDER = "cursor/";
    inline const std::string CURSOR_MAIN = "cursor.bmp";

    /* ***********************************************************************************************************
        GRAPHICS
    *********************************************************************************************************** */
    inline const u32 APP_MAX_FRAMERATE = 500;
    inline const u32  APP_MIN_FRAMERATE = 30;

    /* ***********************************************************************************************************
        AUDIO
    *********************************************************************************************************** */
    inline const float APP_MAX_VOLUME = 1.f;
    inline const float APP_MIN_VOLUME = .0f;

    inline const float APP_MAX_LFE = 5.f;
    inline const float APP_MIN_LFE = .0f;

    inline const float APP_REVERB_DECAY_DEFAULT = .3f;
    inline const float APP_REVERB_DELAY_DEFAULT = .03f;

    /* ***********************************************************************************************************
        IMGUI EMULATOR
    *********************************************************************************************************** */
    //#define _DEBUG_GBX

    inline const std::string  GBX_RELEASE = "BETA";
    inline const u32  GBX_VERSION_MAJOR = 1;
    inline const u32  GBX_VERSION_MINOR = 0;
    inline const u32  GBX_VERSION_PATCH = 0;
    inline const std::string  GBX_AUTHOR = "MatthewMer";

    inline const u32 GUI_WIN_WIDTH = 1280;
    inline const u32 GUI_WIN_HEIGHT = 720;
    inline const u32 GUI_WIN_WIDTH_MIN = 720;
    inline const u32 GUI_WIN_HEIGHT_MIN = 480;

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

    inline const ImVec2 graph_settings_win_size(400.f, 0.f);
    inline const std::vector<float> GRAPH_SETTINGS_COLUMNS = {
        1.f,
        1.f
    };

    inline const ImVec2 network_settings_win_size(400.f, 0.f);
    inline const ImGuiInputTextFlags INPUT_IP_FLAGS = ImGuiInputTextFlags_AutoSelectAll;
#define IP_ADDR_MIN 0
#define IP_ADDR_MAX 255
#define PORT_MIN 1024
#define PORT_MAX 65535

    inline const ImVec2 graph_debug_win_size(600.f, 0.f);

    inline const std::vector<std::string> GAMES_CONSOLES = { "Gameboy", "Gameboy Color" };

#define BG_CHANNEL_COL .1f
    inline const ImVec4 IMGUI_BG_COLOR(BG_CHANNEL_COL, BG_CHANNEL_COL, BG_CHANNEL_COL, 1.f);

    inline const ImVec4 IMGUI_RED_COL(1.f, .0f, .0f, 1.f);
    inline const ImVec4 IMGUI_GREEN_COL(.0f, 1.f, .0f, 1.f);
    inline const ImVec4 IMGUI_BLUE_COLOR(.0f, .0f, 1.f, 1.f);

    inline const float OVERLAY_DISTANCE = 10.0f;

#define FPS_SAMPLES_NUM             50

#define DEBUG_INSTR_LINES           21
#define DEBUG_MEM_LINES             32
#define DEBUG_MEM_ELEM_PER_LINE     0x10

    inline const std::unordered_map<int, std::string> EMULATION_SPEEDS = {
        {1, "x1"},
        {2, "x2"},
        {3, "x3"},
        {4, "x4"},
        {5, "x5"}
    };

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
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoScrollWithMouse;

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
}
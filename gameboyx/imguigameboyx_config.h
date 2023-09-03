#pragma once

#include "imgui.h"
#include <vector>

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
    CONFIG IO AND ROM CONSTANTS
*********************************************************************************************************** */
inline const std::string ROM_FOLDER = "\\rom\\";
inline const std::string CONFIG_FOLDER = "\\config\\";
inline const std::string GAMES_CONFIG_FILE = "games.ini";

/* ***********************************************************************************************************
    IMGUI GBX CONSTANTS/DEFINES
*********************************************************************************************************** */
#define GUI_BG_BRIGHTNESS			0.9f
#define GUI_WIN_WIDTH               1280
#define GUI_WIN_HEIGHT              720

inline const std::string APP_TITLE = "GameboyX";

inline const std::vector<std::pair<std::string, float>> GAMES_COLUMNS = { 
    {"Console", 6.f},
    {"Title", 6.f},
    {"File path", 3.f}, 
    {"File name", 3.f} 
};
inline const std::vector<std::string> GAMES_CONSOLES = { "Gameboy", "Gameboy Color" };


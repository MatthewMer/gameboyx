/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <vector>

#include "GuiMgr.h"
#include "logger.h"
#include "general_config.h"
#include "data_io.h"
#include "HardwareMgr.h"

using namespace std;

/* ***********************************************************************************************************
    PROTOTYPES
*********************************************************************************************************** */
void create_fs_hierarchy();

/* ***********************************************************************************************************
 *
 *  MAIN PROCEDURE
 *
*********************************************************************************************************** */
int main(int, char**)
{
    create_fs_hierarchy();

    // todo: implement a config loader that loads configuration data and pass the data to the different application components
    //          and probably add a namespace that contains all the settings structs instead of passing them around
    Backend::graphics_settings s_graphics_settings = {};
    s_graphics_settings.framerateTarget = 144;
    s_graphics_settings.fpsUnlimited = false;
    s_graphics_settings.presentModeFifo = false;
    s_graphics_settings.tripleBuffering = false;
    s_graphics_settings.app_title = Config::APP_TITLE;
    s_graphics_settings.font = Config::FONT_FOLDER + Config::FONT_MAIN;
    s_graphics_settings.icon = Config::ICON_FOLDER + Config::ICON_FILE;
    s_graphics_settings.shader_folder = Config::SHADER_FOLDER;
    s_graphics_settings.win_height = Config::GUI_WIN_HEIGHT;
    s_graphics_settings.win_height_min = Config::GUI_WIN_HEIGHT_MIN;
    s_graphics_settings.win_width = Config::GUI_WIN_WIDTH;
    s_graphics_settings.win_height_min = Config::GUI_WIN_WIDTH_MIN;

    int sampling_rate_max = 0;
    for (const auto& [key, val] : Backend::Audio::SAMPLING_RATES) {
        if (val.first > sampling_rate_max) { sampling_rate_max = val.first; }
    }

    Backend::audio_settings s_audio_settings = {};
    s_audio_settings.master_volume = .5f;
    s_audio_settings.lfe = 1.f;
    s_audio_settings.sampling_rate = sampling_rate_max;
    s_audio_settings.delay = Config::APP_REVERB_DELAY_DEFAULT;
    s_audio_settings.decay = Config::APP_REVERB_DECAY_DEFAULT;
    s_audio_settings.low_frequencies = true;
    s_audio_settings.high_frequencies = true;

    Backend::control_settings s_control_settings = {};
    s_control_settings.mouse_always_visible = false;
    s_control_settings.bmp_custom_cursor = Config::CURSOR_FOLDER + Config::CURSOR_MAIN;

    Backend::HardwareMgr::InitHardware(s_graphics_settings, s_audio_settings, s_control_settings);
    if (Backend::HardwareMgr::GetError() != Backend::HW_ERROR::NONE)   { return -1; }
    
    GUI::GuiMgr* gui_mgr = GUI::GuiMgr::getInstance();

    // Main loop
    LOG_INFO("[emu] application up and running");

    bool running = true;
    while (running)
    {
        Backend::HardwareMgr::ProcessEvents(running);
        gui_mgr->ProcessInput();

        Backend::HardwareMgr::ProcessTimedEvents();

        if (Backend::HardwareMgr::CheckFrame()) {
            Backend::HardwareMgr::NextFrame();
            gui_mgr->ProcessGUI();
            Backend::HardwareMgr::RenderFrame();
        }
    }
    
    gui_mgr->resetInstance();
    Backend::HardwareMgr::ShutdownHardware();

    return 0;
}

/* ***********************************************************************************************************
    MISC
*********************************************************************************************************** */
void create_fs_hierarchy() {
    GUI::IO::check_and_create_config_folders();
    GUI::IO::check_and_create_config_files();
    GUI::IO::check_and_create_log_folders();
    GUI::IO::check_and_create_shader_folders();
    GUI::IO::check_and_create_save_folders();
    GUI::IO::check_and_create_rom_folder();
}
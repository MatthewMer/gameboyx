/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include <imgui.h>
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <SDL.h>
#include <vector>

#include "GuiMgr.h"
#include "logger.h"
#include "general_config.h"
#include "data_io.h"
#include "HardwareMgr.h"

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define SDL_MAIN_HANDLED

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

    int sampling_rate_max = 0;
    for (const auto& [key, val] : Config::SAMPLING_RATES) {
        if (val.first > sampling_rate_max) { sampling_rate_max = val.first; }
    }

    Backend::audio_settings s_audio_settings = {};
    s_audio_settings.master_volume = .5f;
    s_audio_settings.lfe = 1.f;
    s_audio_settings.sampling_rate = sampling_rate_max;
    s_audio_settings.delay = APP_REVERB_DELAY_DEFAULT;
    s_audio_settings.decay = APP_REVERB_DECAY_DEFAULT;

    Backend::control_settings s_control_settings = {};
    s_control_settings.mouse_always_visible = false;

    if (!Backend::HardwareMgr::InitHardware(s_graphics_settings, s_audio_settings, s_control_settings))   { return -1; }

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
    Backend::FileIO::check_and_create_config_folders();
    Backend::FileIO::check_and_create_config_files();
    Backend::FileIO::check_and_create_log_folders();
    Backend::FileIO::check_and_create_shader_folders();
    Backend::FileIO::check_and_create_save_folders();
    Backend::FileIO::check_and_create_rom_folder();
}
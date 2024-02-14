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

    if (!HardwareMgr::InitHardware())   { return -1; }

    GuiMgr* gui_mgr = GuiMgr::getInstance();

    // Main loop
    LOG_INFO("[emu] application up and running");

    bool running = true;
    while (running)
    {
        HardwareMgr::ProcessInput(running);
        gui_mgr->ProcessGUI();

        HardwareMgr::NextFrame();
        gui_mgr->DrawGUI();
        HardwareMgr::RenderFrame();
    }
    
    VHardwareMgr::getInstance()->ShutdownHardware();
    HardwareMgr::ShutdownHardware();

    return 0;
}

/* ***********************************************************************************************************
    MISC
*********************************************************************************************************** */
void create_fs_hierarchy() {
    check_and_create_config_folders();
    check_and_create_config_files();
    check_and_create_log_folders();
    check_and_create_shader_folders();
    check_and_create_save_folders();
    check_and_create_rom_folder();
}
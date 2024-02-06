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
#include "GameboyCartridge.h"
#include "VHardwareMgr.h"
#include "data_containers.h"
#include "GraphicsMgr.h"
#include "AudioMgr.h"
#include "data_io.h"

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define SDL_MAIN_HANDLED

/* ***********************************************************************************************************
    PROTOTYPES
*********************************************************************************************************** */
bool sdl_graphics_start(GraphicsMgr* _graphics_mgr);
void sdl_graphics_shutdown(GraphicsMgr* _graphics_mgr);

void sdl_toggle_full_screen(SDL_Window* _window);

bool imgui_init(GraphicsMgr* _graphics_mgr);
void imgui_shutdown(GraphicsMgr* _graphics_mgr);
void imgui_nextframe(GraphicsMgr* _graphics_mgr);

void create_fs_hierarchy();

/* ***********************************************************************************************************
 *
 *  MAIN PROCEDURE
 *
*********************************************************************************************************** */
int main(int, char**)
{
    create_fs_hierarchy();
    auto machine_info = machine_information();
    auto game_stat = game_status();
    auto graphics_info = graphics_information();
    auto audio_info = audio_information();

    // init sdl
    SDL_Window* window = nullptr;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("[SDL]", SDL_GetError());
        return -1;
    }
    else {
        LOG_INFO("[SDL] initialized");
    }

    // Create window with Vulkan graphics context
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow(APP_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GUI_WIN_WIDTH, GUI_WIN_HEIGHT, window_flags);
    if (!window) {
        LOG_ERROR("[SDL]", SDL_GetError());
        return -1;
    }
    else {
        LOG_INFO("[SDL] window created");
    }

    SDL_SetWindowMinimumSize(window, GUI_WIN_WIDTH_MIN, GUI_WIN_HEIGHT_MIN);

    auto* graphics_mgr = GraphicsMgr::getInstance(window, graphics_info, game_stat);
    if (!sdl_graphics_start(graphics_mgr)) { return -2; }
    if (!imgui_init(graphics_mgr)) { return -3; }

    graphics_mgr->EnumerateShaders();

    AudioMgr* audio_mgr = AudioMgr::getInstance(audio_info);
    if (audio_mgr != nullptr) {
        audio_mgr->InitAudio(false);
    } else {
        LOG_ERROR("[emu] initialize audio backend");
    }

    GuiMgr* gbx_gui = GuiMgr::getInstance(machine_info, game_stat, graphics_info);
    VHardwareMgr* vhwmgr_obj = nullptr;

    /*
    while (!graphics_info.shaders_compilation_finished) {               // gets probably changed to get done when application is up and running -> output loading bar
        graphics_mgr->CompileNextShader();
    }
    */

    // Main loop
    LOG_INFO("[emu] application up and running");

    u32 win_min;

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                running = false;

            SDL_Keycode key;
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                key = event.key.keysym.sym;
                switch (key) {
                case SDLK_F3:
                case SDLK_LSHIFT:
                    gbx_gui->EventKeyDown(key);
                    break;
                default:
                    if (game_stat.game_running) {
                        if (!vhwmgr_obj->EventKeyDown(key)) {
                            gbx_gui->EventKeyDown(key);
                        }
                    }
                    else {
                        gbx_gui->EventKeyDown(key);
                    }
                    break;
                }
                break;
            case SDL_KEYUP:
                key = event.key.keysym.sym;
                switch (key) {
                case SDLK_ESCAPE:
                    if (game_stat.game_running) {
                        game_stat.pending_game_stop = true;
                    }
                    break;
                case SDLK_F1:
                case SDLK_F3:
                case SDLK_F10:
                case SDLK_LSHIFT:
                    gbx_gui->EventKeyUp(key);
                    break;
                case SDLK_F11:
                    sdl_toggle_full_screen(window);
                    break;
                default:
                    if (game_stat.game_running) {
                        if (!vhwmgr_obj->EventKeyUp(key)) {
                            gbx_gui->EventKeyUp(key);
                        }
                    } else {
                        gbx_gui->EventKeyUp(key);
                    }
                    break;
                }
                break;
            case SDL_MOUSEWHEEL:
                gbx_gui->EventMouseWheel(event.wheel.y);
                break;
            default:
                break;
            }
        }

        // start/reset game
        if (game_stat.pending_game_start || game_stat.request_reset) {
            machine_info.reset_machine_information();
            gbx_gui->SetGameToStart();

            VHardwareMgr::resetInstance();
            vhwmgr_obj = VHardwareMgr::getInstance(machine_info, graphics_mgr, graphics_info, audio_mgr, audio_info);

            if (VHardwareMgr::GetErrors() != 0x00) {
                LOG_ERROR("[emu] initializing virtual hardware");
                game_stat.pending_game_stop = true;
            } else if (vhwmgr_obj) {
                gbx_gui->GameStarted();
                game_stat.game_running = true;
            } else {
                LOG_ERROR("[emu] unexpected error");
                game_stat.pending_game_stop = true;
            }

            game_stat.request_reset = false;
            game_stat.pending_game_start = false;
        }

        // stop game
        if (game_stat.pending_game_stop) {
            VHardwareMgr::resetInstance();

            game_stat.game_running = false;
            game_stat.pending_game_stop = false;
            machine_info.reset_machine_information();

            gbx_gui->GameStopped();
        }

        // run virtual hardware
        if (game_stat.game_running && vhwmgr_obj != nullptr) {
            vhwmgr_obj->ProcessHardware();
        }

        win_min = SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;
        if (!win_min) {
            // new imgui frame
            imgui_nextframe(graphics_mgr);

            gbx_gui->ProcessGUI();

            // render results
            graphics_mgr->RenderFrame();
        }
    }
    
    imgui_shutdown(graphics_mgr);
    sdl_graphics_shutdown(graphics_mgr);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

/* ***********************************************************************************************************
    SDL FUNCTIONS
*********************************************************************************************************** */
bool sdl_graphics_start(GraphicsMgr* _graphics_mgr) {
    if (!_graphics_mgr->InitGraphics()) { return false; }
    if (!_graphics_mgr->StartGraphics()) { return false; }
    return true;
}

void sdl_graphics_shutdown(GraphicsMgr* _graphics_mgr) {
    _graphics_mgr->StopGraphics();
    _graphics_mgr->ExitGraphics();
}

void sdl_toggle_full_screen(SDL_Window* _window) {
    if (SDL_GetWindowFlags(_window) & SDL_WINDOW_FULLSCREEN) {
        SDL_SetWindowFullscreen(_window, 0);
    }
    else {
        SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

/* ***********************************************************************************************************
    IMGUI FUNCTIONS
*********************************************************************************************************** */
bool imgui_init(GraphicsMgr* _graphics_mgr) {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    return _graphics_mgr->InitImgui();
}

void imgui_shutdown(GraphicsMgr* _graphics_mgr) {
    _graphics_mgr->DestroyImgui();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void imgui_nextframe(GraphicsMgr* _graphics_mgr) {
    _graphics_mgr->NextFrameImGui();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
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
/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "imgui.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <SDL.h>
#include <vector>

#include "ImGuiGameboyX.h"
#include "logger.h"
#include "imguigameboyx_config.h"
#include "Cartridge.h"
#include "VHardwareMgr.h"
#include "information_structs.h"
#include "VulkanMgr.h"

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define SDL_MAIN_HANDLED

/* ***********************************************************************************************************
    PROTOTYPES
*********************************************************************************************************** */
bool sdl_init_vulkan(VulkanMgr& _vk_mgr, SDL_Window* _window);
bool sdl_vulkan_start(VulkanMgr& _vk_mgr);
void sdl_shutdown(VulkanMgr& _vk_mgr, SDL_Window* _window);

void sdl_toggle_full_screen(SDL_Window* _window);

bool imgui_init(VulkanMgr& _vk_mgr);
void imgui_shutdown(VulkanMgr& _vk_mgr);
void imgui_nextframe(VulkanMgr& _vk_mgr);

/* ***********************************************************************************************************
 *
 *  MAIN PROCEDURE
 *
*********************************************************************************************************** */
int main(int, char**)
{
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

    auto vk_mgr = VulkanMgr(window);
    if (!sdl_init_vulkan(vk_mgr, window)) { return -2; }

    if (!sdl_vulkan_start(vk_mgr)) { return -3; }

    if (!imgui_init(vk_mgr)) { return -4; }

    auto machine_info = machine_information();
    auto game_stat = game_status();
    ImGuiGameboyX* gbx_gui = ImGuiGameboyX::getInstance(machine_info, game_stat);
    VHardwareMgr* vhwmgr_obj = nullptr;

    // Main loop
    LOG_INFO("Initialization completed");

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
                case SDLK_LSHIFT:
                    gbx_gui->EventKeyDown(key);
                    break;
                default:
                    if (game_stat.game_running) {
                        vhwmgr_obj->EventKeyDown(key);
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
                    game_stat.pending_game_stop = true;
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
                        vhwmgr_obj->EventKeyUp(key);
                    }
                    else {
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

        // game start/stop
        if (game_stat.pending_game_start || game_stat.request_reset) {
            machine_info.reset_machine_information();

            VHardwareMgr::resetInstance();
            vhwmgr_obj = VHardwareMgr::getInstance(gbx_gui->GetGameStartContext(), machine_info);

            game_stat.request_reset = false;
            game_stat.pending_game_start = false;

            game_stat.game_running = true;

            gbx_gui->GameStartCallback();
        }
        if (game_stat.pending_game_stop) {
            VHardwareMgr::resetInstance();
            gbx_gui->GameStopped();

            game_stat.game_running = false;
            game_stat.pending_game_stop = false;
            machine_info.reset_machine_information();
        }

        // run virtual hardware
        if (game_stat.game_running) {
            vhwmgr_obj->ProcessNext();
        }
        
        // new frame
        imgui_nextframe(vk_mgr);

        gbx_gui->ProcessGUI();

        // render results
        vk_mgr.RenderFrame();
    }
    
    imgui_shutdown(vk_mgr);
    sdl_shutdown(vk_mgr, window);

    return 0;
}

/* ***********************************************************************************************************
    SDL FUNCTIONS
*********************************************************************************************************** */
bool sdl_init_vulkan(VulkanMgr& _vk_mgr, SDL_Window* _window) {
    uint32_t sdl_extension_count;
    SDL_Vulkan_GetInstanceExtensions(_window, &sdl_extension_count, nullptr);
    auto sdl_extensions = vector<const char*>(sdl_extension_count);
    SDL_Vulkan_GetInstanceExtensions(_window, &sdl_extension_count, sdl_extensions.data());

    auto device_extensions = vector<const char*>() = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    if (!_vk_mgr.InitVulkan(sdl_extensions, device_extensions)) { return false; }

    return true;
}

bool sdl_vulkan_start(VulkanMgr& _vk_mgr) {
    if (!_vk_mgr.InitSurface()) {
        return false;
    }
    if (!_vk_mgr.InitSwapchain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        return false;
    }
    if (!_vk_mgr.InitRenderPass()) {
        return false;
    }
    if (!_vk_mgr.InitFrameBuffers()) {
        return false;
    }
    if (!_vk_mgr.InitCommandBuffers()) {
        return false;
    }

    return true;
}

void sdl_shutdown(VulkanMgr& _vk_mgr, SDL_Window* _window) {
    _vk_mgr.DestroyCommandBuffer();
    _vk_mgr.DestroyFrameBuffers();
    _vk_mgr.DestroyRenderPass();
    _vk_mgr.DestroySwapchain(false);
    _vk_mgr.DestroySurface();
    _vk_mgr.ExitVulkan();

    SDL_DestroyWindow(_window);
    SDL_Quit();
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
bool imgui_init(VulkanMgr& _vk_mgr) {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!_vk_mgr.InitImgui()) { return false; }

    return true;
}

void imgui_shutdown(VulkanMgr& _vk_mgr) {
    _vk_mgr.DestroyImgui();

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void imgui_nextframe(VulkanMgr& _vk_mgr) {
    _vk_mgr.NextFrameImGui();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}
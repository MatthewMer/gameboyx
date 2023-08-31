#include "imgui_gameboy_x.h"
#include "imgui.h"
#include "config.h"
#include "nfd.h"
#include "logger.h"

#include <filesystem>
namespace fs = std::filesystem;

const char* rom_folder = "\\rom";

ImGuiGameboyX::ImGuiGameboyX() {
    show_gui = true;
    show_main_menu_bar = true;
    show_game_select = false;
    show_win_about = false;
}

void ImGuiGameboyX::ShowGUI() {
    if (show_gui) {
        if (show_main_menu_bar) ShowMainMenuBar();
        if (show_win_about) ShowWindowAbout();
    }
    if (show_game_select) {
        ShowGameSelect();
    }
}

void ImGuiGameboyX::ShowMainMenuBar() {

    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!");

    if (show_main_menu_bar)
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("Add ROM", NULL, &show_game_select);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                ImGui::MenuItem("Show menu bar", NULL, &show_main_menu_bar);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Control")) {

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Graphics")) {

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("About", NULL, &show_win_about);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
}

void ImGuiGameboyX::ShowWindowAbout() {
    if (show_win_about) {
        ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
            ;

        if (ImGui::Begin("About", &show_win_about, win_flags)) {
            ImGui::Text("GameboyX Emulator");
            ImGui::Text("Version: %s %d.%d", GBX_RELEASE, GBX_VERSION, GBX_VERSION_SUB);
            ImGui::Text("Author: %s", GBX_AUTHOR);
            ImGui::Spacing();
        }
        ImGui::End();
    }
}

void ImGuiGameboyX::ShowGameSelect() {
    nfdchar_t* out_path = NULL;

    const char* c_diag_path = fs::current_path().string().c_str();
    char* c_rom_path = new char[strlen(c_diag_path) + strlen(rom_folder) + 1];
    memcpy(c_rom_path, c_diag_path, strlen(c_diag_path));
    memcpy(&c_rom_path[strlen(c_diag_path)], rom_folder, strlen(rom_folder) + 1);

    if (!fs::is_directory(c_rom_path) || !fs::exists(c_rom_path)) {
        fs::create_directory(c_rom_path);
    }

    nfdchar_t* rom_path = new nfdchar_t[strlen(c_diag_path) + strlen(rom_folder) + 1];
    memcpy(rom_path, c_rom_path, strlen(c_rom_path));
        
    nfdresult_t result = NFD_OpenDialog(NULL, rom_path, &out_path);
    
    if (result == NFD_OKAY) {
        if (out_path != nullptr) {
            char* c_rom_file = new char[strlen(out_path) + 1];
            memcpy(c_rom_file, out_path, strlen(out_path) + 1);

            // TODO: check if passed rom file is valid and add to config and vec<game> 
            LOG_WARN(c_rom_file);
        }
    }
    else {
        LOG_ERROR("Couldn't open file dialogue");
    }

    show_game_select = false;
}

/*
static char* c_substr(int _start, size_t _num, char* c_str) {

}*/
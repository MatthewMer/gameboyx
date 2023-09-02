/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "ImGuiGameboyX.h"
#include "imgui.h"
#include "config.h"
#include "nfd.h"
#include "logger.h"
#include "games_config_io.h"
#include <filesystem>

using namespace std;
namespace fs = filesystem;

/* ***********************************************************************************************************
    CONSTANTS
*********************************************************************************************************** */
const static string rom_folder = "\\rom\\";
const static string config_folder = "\\config\\";
const string games_config_file = "games.ini";

const static vector<vector<string>> file_exts = {
    {"Gameboy Color", "gbc"}, {"Gameboy", "gb"}
};

/* ***********************************************************************************************************
    PROTOTYPES
*********************************************************************************************************** */
static bool check_ext(const string& file);
static vector<string> split_path(const string& path);
static bool check_path(const string& path);

/* ***********************************************************************************************************
    IMGUIGAMEBOYX FUNCTIONS
*********************************************************************************************************** */
ImGuiGameboyX* ImGuiGameboyX::instance = nullptr;

ImGuiGameboyX* ImGuiGameboyX::getInstance() {
    if (instance == nullptr) {
        instance = new ImGuiGameboyX();
    }
    return instance;
}

ImGuiGameboyX::ImGuiGameboyX() {
    NFD_Init();
}

void ImGuiGameboyX::ShowGUI() {
    if (show_gui) {
        if (show_main_menu_bar) ShowMainMenuBar();
        if (show_win_about) ShowWindowAbout();
    }
    if (show_new_game_dialog) {
        ShowNewGameDialog();
    }
}

void ImGuiGameboyX::ShowMainMenuBar() {

    IM_ASSERT(ImGui::GetCurrentContext() != nullptr && "Missing dear imgui context. Refer to examples app!");

    if (show_main_menu_bar)
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("Add ROM", nullptr, &show_new_game_dialog);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                ImGui::MenuItem("Show menu bar", nullptr, &show_main_menu_bar);
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
                ImGui::MenuItem("About", nullptr, &show_win_about);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
}

void ImGuiGameboyX::ShowWindowAbout() {
    if (show_win_about) {
        if (const ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse; ImGui::Begin("About", &show_win_about, win_flags)) {
            ImGui::Text("GameboyX Emulator");
            ImGui::Text("Version: %s %d.%d", GBX_RELEASE, GBX_VERSION, GBX_VERSION_SUB);
            ImGui::Text("Author: %s", GBX_AUTHOR);
            ImGui::Spacing();
        }
        ImGui::End();
    }
}

void ImGuiGameboyX::ShowNewGameDialog() {
    string s_path_rom_folder = fs::current_path().string() + rom_folder;

    if (!check_path(s_path_rom_folder)) {
        LOG_ERROR("Couldn't create rom folder");
        show_new_game_dialog = false;
        return;
    }

    string s_path_config = fs::current_path().string() + config_folder;

    if (!check_path(s_path_config)) {
        LOG_ERROR("Couldn't create config folder");
        show_new_game_dialog = false;
        return;
    }

    nfdchar_t* out_path = nullptr;
    auto filter_items = (nfdfilteritem_t*)malloc(sizeof(nfdfilteritem_t) * file_exts.size());
    for (int i = 0; i < file_exts.size(); i++) {
        filter_items[i] = {file_exts[i][0].c_str(), file_exts[i][1].c_str() };
    }

    auto result = NFD_OpenDialog(&out_path, filter_items, 2, s_path_rom_folder.c_str());
    if (result == NFD_OKAY) {
        if (out_path != nullptr) {
            string path_to_rom(out_path);
            auto vec_path_to_rom = split_path(path_to_rom);

            if (!check_ext(vec_path_to_rom.back())) return;

            auto game_ctx = game_info();
            game_ctx.file_path = "";
            for (int i = 0; i < vec_path_to_rom.size() - 1; i++) {
                game_ctx.file_path += vec_path_to_rom[i] + "/";
            }
            game_ctx.file_name = vec_path_to_rom.back();

            vector<u8> vec_rom;
            if (!Cartridge::read_rom_to_buffer(game_ctx, vec_rom)) {
                show_new_game_dialog = false;
                return;
            }

            LOG_WARN(s_path_rom_folder, ",", game_ctx.file_path);

            if (s_path_rom_folder.compare(game_ctx.file_path) != 0) {
                if (!Cartridge::copy_rom_to_rom_folder(game_ctx, vec_rom, s_path_rom_folder)) {
                    LOG_ERROR("Couldn't copy rom file to /rom. Fallback to given path");
                }
            }

            if (!Cartridge::read_header_info(game_ctx, vec_rom)) {
                show_new_game_dialog = false;
                return;
            }

            for (const game_info& n : this->games) {
                if (n == game_ctx) {
                    LOG_WARN("Game already added");
                    show_new_game_dialog = false;
                    return;
                }
            }

            if (!write_game_to_config(game_ctx, s_path_config + games_config_file)) return;
            this->games.push_back(game_ctx);
        }
        else {
            LOG_ERROR("No path (nullptrptr)");
            return;
        }
    }
    else if (result != NFD_CANCEL) {
        LOG_ERROR("Couldn't open file dialog");
    }

    show_new_game_dialog = false;
}

void ImGuiGameboyX::ShowGameSelect() {

}

bool ImGuiGameboyX::ReadGamesFromConfig() {
    return read_games_from_config(this->games, fs::current_path().string() + config_folder + games_config_file);
}

/* ***********************************************************************************************************
    IMGUIGAMEBOYX SDL FUNCTIONS
*********************************************************************************************************** */
void ImGuiGameboyX::KeyDown(SDL_Keycode key) {
    switch (key) {
    case SDLK_F10:
        show_main_menu_bar = !show_main_menu_bar;
        break;
    default:
        break;
    }
}

void ImGuiGameboyX::KeyUp(SDL_Keycode key) {
    switch (key) {
    case SDLK_F10:

        break;
    default:
        break;
    }
}

/* ***********************************************************************************************************
    STATIC FUNCTIONS
*********************************************************************************************************** */


static bool check_ext(const string& file) {
    string delimiter = ".";
    int ext_start = (int)file.find(delimiter) + 1;
    string file_ext = file.substr(ext_start, file.length() - ext_start);

    for (const auto& n : file_exts) {
        if (file_ext.compare(n[1]) == 0) return true;
    }

    LOG_ERROR("Wrong file extension");
    return false;
}

static vector<string> split_path(const string& path) {
    vector<string> path_split;
    string path_copy = path;

    const string delimiter = "\\";

    while (int pos = path_copy.find(delimiter) != string::npos) {
        path_split.push_back(path_copy.substr(0, pos));
        path_copy.erase(0, pos + delimiter.length());
    }
    path_split.push_back(path_copy);

    return path_split;
}

static bool check_path(const string& path) {
    if (!fs::is_directory(path) || !fs::exists(path)) {
        fs::create_directory(path);
    }

    return fs::is_directory(path) || fs::exists(path);
}
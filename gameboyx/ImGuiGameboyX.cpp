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
const static string rom_folder = "\\rom";

const static vector<string> file_exts = {
    "gbc"
};

const string config_path = "games.ini";

/* ***********************************************************************************************************
    PROTOTYPES
*********************************************************************************************************** */
static bool check_ext(const string& file);
static vector<string> split_path(const string& path);

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

void ImGuiGameboyX::ShowGUI() {
    if (show_gui) {
        if (show_main_menu_bar) ShowMainMenuBar();
        if (show_win_about) ShowWindowAbout();
    }
    if (show_new_game_dialogue) {
        ShowNewGameDialogue();
    }
}

void ImGuiGameboyX::ShowMainMenuBar() {

    IM_ASSERT(ImGui::GetCurrentContext() != nullptr && "Missing dear imgui context. Refer to examples app!");

    if (show_main_menu_bar)
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("Add ROM", nullptr, &show_new_game_dialogue);
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
        if (const auto win_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse; ImGui::Begin("About", &show_win_about, win_flags)) {
            ImGui::Text("GameboyX Emulator");
            ImGui::Text("Version: %s %d.%d", GBX_RELEASE, GBX_VERSION, GBX_VERSION_SUB);
            ImGui::Text("Author: %s", GBX_AUTHOR);
            ImGui::Spacing();
        }
        ImGui::End();
    }
}

void ImGuiGameboyX::ShowNewGameDialogue() {
    nfdchar_t* out_path = nullptr;

    string path = fs::current_path().string() + rom_folder;

    if (!fs::is_directory(path) || !fs::exists(path)) {
        fs::create_directory(path);
    }

    auto c_path = path.c_str();

    nfdresult_t result = NFD_OpenDialog(nullptr, c_path, &out_path);
    if (result == NFD_OKAY) {
        if (out_path != nullptr) {
            string c_path_to_rom(out_path);
            auto path_to_rom = split_path(c_path_to_rom);

            if (!check_ext(path_to_rom[path_to_rom.size()-1])) return;

            auto game_ctx = game_info("");
            game_ctx.file_path = "";
            for (int i = 0; i < path_to_rom.size() - 2; i++) {
                game_ctx.file_path += path_to_rom[i] + "\\";
            }
            game_ctx.file_name = path_to_rom[path_to_rom.size() - 1];

            vector<byte> vec_read_rom;
            Cartridge::read_header_info(game_ctx, vec_read_rom);

            for (const game_info& n : this->games) {
                if (n == game_ctx) {
                    LOG_WARN("Game already added");
                    show_new_game_dialogue = false;
                    return;
                }
            }

            if (!write_game_to_config(game_ctx, config_path)) return;
            this->games.push_back(game_ctx);
        }
        else {
            LOG_ERROR("No path (nullptrptr)");
            return;
        }
    }
    else if (result != NFD_CANCEL) {
        LOG_ERROR("Couldn't open file dialogue");
    }

    show_new_game_dialogue = false;
}

void ImGuiGameboyX::ShowGameSelect() {

}

bool ImGuiGameboyX::ReadGamesFromConfig() {
    return read_games_from_config(this->games, config_path);
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
    int ext_start = file.find(delimiter) + 1;
    string file_ext = file.substr(ext_start, file.length() - ext_start);

    for (const auto& n : file_exts) {
        if (file_ext.compare(n) == 0) return true;
    }

    LOG_ERROR("Wrong file extension");
    return false;
}

static vector<string> split_path(const string& path) {
    vector<string> path_split;
    string path_copy = path;

    string delimiter = "\\";
    while (path_copy.find(delimiter) != string::npos) {
        path_split.push_back(path_copy.substr(0, path_copy.find(delimiter)));
        path_copy.erase(0, path_copy.find(delimiter) + delimiter.length());
    }

    return path_split;
}
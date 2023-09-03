#include "ImGuiGameboyX.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "Cartridge.h"
#include "imgui.h"
#include "config_io.h"
#include "nfd.h"
#include "logger.h"
#include "helper_functions.h"
#include "imguigameboyx_config.h"

using namespace std;

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
    check_and_create_folders();
    if (check_and_create_files()) {
        if (!read_games_from_config(this->games, CONFIG_FOLDER + GAMES_CONFIG_FILE)) {
            LOG_ERROR("Error while reading games.ini");
        }
    }
}

void ImGuiGameboyX::ShowGUI() {
    IM_ASSERT(ImGui::GetCurrentContext() != nullptr && "Missing dear imgui context. Refer to examples app!");

    if (this->showGui) {
        if (this->showWinAbout) ShowWindowAbout();
        if (this->showGameSelect) ShowGameSelect();
    }
    if (this->showMainMenuBar) ShowMainMenuBar();
    if (this->showNewGameDialog) ShowNewGameDialog();
}

void ImGuiGameboyX::ShowMainMenuBar() {
    if (this->showMainMenuBar)
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("Add ROM", nullptr, &this->showNewGameDialog);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                ImGui::MenuItem("Show menu bar", nullptr, &this->showMainMenuBar);
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
                ImGui::MenuItem("About", nullptr, &this->showWinAbout);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
}

void ImGuiGameboyX::ShowWindowAbout() {
    if (this->showWinAbout) {
        const ImGuiWindowFlags win_flags =
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoCollapse;

        if (; ImGui::Begin("About", &this->showWinAbout, win_flags)) {
            ImGui::Text("GameboyX Emulator");
            ImGui::Text("Version: %s %d.%d", GBX_RELEASE, GBX_VERSION_MAJOR, GBX_VERSION_MINOR, GBX_VERSION_PATCH);
            ImGui::Text("Author: %s", GBX_AUTHOR);
            ImGui::Spacing();
        }
        ImGui::End();
    }
}

void ImGuiGameboyX::ShowNewGameDialog() {
    if (!check_and_create_folders()) {
        LOG_ERROR("Required subfolders don't exist");
        this->showNewGameDialog = false;
        return;
    }

    nfdchar_t* out_path = nullptr;
    auto filter_items = (nfdfilteritem_t*)malloc(sizeof(nfdfilteritem_t) * FILE_EXTS.size());
    for (int i = 0; i < FILE_EXTS.size(); i++) {
        filter_items[i] = { FILE_EXTS[i][0].c_str(), FILE_EXTS[i][1].c_str() };
    }

    string current_path = get_current_path();
    string s_path_rom_folder = current_path + ROM_FOLDER;

    const auto result = NFD_OpenDialog(&out_path, filter_items, 2, s_path_rom_folder.c_str());
    if (result == NFD_OKAY) {
        if (out_path != nullptr) {
            string path_to_rom(out_path);
            auto game_ctx = game_info();

            if (!Cartridge::read_new_game(game_ctx, path_to_rom)) {
                showNewGameDialog = false;
                return;
            }

            for (const auto& n : games) {
                if (game_ctx == n) {
                    LOG_WARN("Game already added ! Process aborted");
                    showNewGameDialog = false;
                    return;
                }
            }

            if (!write_new_game(game_ctx)) {
                showNewGameDialog = false;
                return;
            }

            this->games.push_back(game_ctx);
        }
        else {
            LOG_ERROR("No path (nullptr)");
            return;
        }
    }
    else if (result != NFD_CANCEL) {
        LOG_ERROR("Couldn't open file dialog");
    }

    showNewGameDialog = false;
}

void ImGuiGameboyX::ShowGameSelect() {
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(main_viewport->Size.x, main_viewport->Size.y));

    const ImGuiWindowFlags win_flags = 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (!ImGui::Begin("", nullptr, win_flags)) {
        ImGui::End();
        return;
    }

    const ImGuiTableFlags table_flags =
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersInnerV;

    const int column_num = GAMES_COLUMNS.size();

    if (ImGui::BeginTable("game_list", column_num, table_flags, ImVec2(.0f, .0f), 3.f)) {

        const ImGuiTableColumnFlags col_flags = 0;
        
        for (int i = 0; const auto& [label, divisor] : GAMES_COLUMNS) {
            ImGui::TableSetupColumn(label.c_str(), col_flags, main_viewport->Size.x / divisor);
            ImGui::TableSetupScrollFreeze(i++, 0);
        }
        ImGui::TableSetupColumn("", col_flags, main_viewport->Size.x / column_num);
        ImGui::TableSetupScrollFreeze(column_num - 1, 0);
        ImGui::TableHeadersRow();

        // TODO: make entries selectable (and double click)
        ImGuiSelectableFlags sel_flags =
            ImGuiSelectableFlags_SpanAllColumns |
            ImGuiSelectableFlags_AllowDoubleClick;

        for (int i = 0; const auto & game : this->games) {
            ImGui::TableNextRow();
            ImGui::Selectable("", false, sel_flags);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(game.is_cgb ? GAMES_CONSOLES[1].c_str() : GAMES_CONSOLES[0].c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(game.title.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(game.file_path.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(game.file_name.c_str());
        }

        ImGui::EndTable();
    }
    ImGui::EndMenu();
}

/* ***********************************************************************************************************
    IMGUIGAMEBOYX SDL FUNCTIONS
*********************************************************************************************************** */
void ImGuiGameboyX::KeyDown(SDL_Keycode _key) {
    switch (_key) {
    case SDLK_F10:
        showMainMenuBar = !showMainMenuBar;
        break;
    default:
        break;
    }
}

void ImGuiGameboyX::KeyUp(SDL_Keycode _key) {
    switch (_key) {
    default:
        break;
    }
}
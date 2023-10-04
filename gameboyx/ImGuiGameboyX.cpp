#include "ImGuiGameboyX.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "Cartridge.h"
#include "imgui.h"
#include "data_io.h"
#include "nfd.h"
#include "logger.h"
#include "helper_functions.h"
#include "guigameboyx_config.h"
#include "io_config.h"

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define DEBUG_INSTR_ELEMENTS        20

/* ***********************************************************************************************************
    PROTOTYPES
*********************************************************************************************************** */
static void create_fs_hierarchy();

/* ***********************************************************************************************************
    IMGUIGAMEBOYX FUNCTIONS
*********************************************************************************************************** */
ImGuiGameboyX* ImGuiGameboyX::instance = nullptr;

ImGuiGameboyX* ImGuiGameboyX::getInstance(machine_information& _machine_info, game_status& _game_status) {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }

    instance = new ImGuiGameboyX(_machine_info, _game_status);
    return instance;
}

void ImGuiGameboyX::resetInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
    instance = nullptr;
}

ImGuiGameboyX::ImGuiGameboyX(machine_information& _machine_info, game_status& _game_status) : machineInfo(_machine_info), gameState(_game_status){
    NFD_Init();
    create_fs_hierarchy();
    if (read_games_from_config(this->games, CONFIG_FOLDER + GAMES_CONFIG_FILE)) {
        InitGamesGuiCtx();
    }
}

/* ***********************************************************************************************************
    GUI PROCESS ENTRY POINT
*********************************************************************************************************** */
void ImGuiGameboyX::ProcessGUI() {
    IM_ASSERT(ImGui::GetCurrentContext() != nullptr && "Missing dear imgui context. Refer to examples app!");

    if (showMainMenuBar) ShowMainMenuBar();
    if (machineInfo.instruction_debug_enabled) ShowDebugInstructions();
    if (showWinAbout) ShowWindowAbout();
    if (machineInfo.track_hardware_info) ShowHardwareInfo();

    if (!gameState.game_running) {
        // gui elements
        if (showGameSelect) ShowGameSelect();
        if (showNewGameDialog) ShowNewGameDialog();
        
        // actions
        if (deleteGames) ActionDeleteGames();
        if (gameState.pending_game_start) {
            ActionStartGame(gameSelectedIndex);
        }

        // special funktions
        ActionProcessSpecialKeys();
    }
}

/* ***********************************************************************************************************
    GUI BUILDER
*********************************************************************************************************** */
void ImGuiGameboyX::ShowMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        
        if (ImGui::BeginMenu("Games")) {
            if (gameState.game_running) {
                ImGui::MenuItem("Stop game", nullptr, &gameState.pending_game_stop);
            }
            else {
                ImGui::MenuItem("Add new game", nullptr, &showNewGameDialog);
                ImGui::MenuItem("Remove game(s)", nullptr, &deleteGames);
                ImGui::MenuItem("Start game", nullptr, &gameState.pending_game_start);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings")) {
            ImGui::MenuItem("Show menu bar", nullptr, &showMainMenuBar);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Control")) {

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Graphics")) {

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Debug Instructions", nullptr, &machineInfo.instruction_debug_enabled);
            ImGui::MenuItem("Hardware Info", nullptr, &machineInfo.track_hardware_info);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About", nullptr, &showWinAbout);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void ImGuiGameboyX::ShowWindowAbout() {
    const ImGuiWindowFlags win_flags =
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("About", &showWinAbout, win_flags)) {
        ImGui::Text("GameboyX Emulator");
        ImGui::Text("Version: %s %d.%d", GBX_RELEASE, GBX_VERSION_MAJOR, GBX_VERSION_MINOR, GBX_VERSION_PATCH);
        ImGui::Text("Author: %s", GBX_AUTHOR);
        ImGui::Spacing();
    }
    ImGui::End();
}

void ImGuiGameboyX::ShowDebugInstructions() {

    if (gameState.game_running) {
        // if cpu was running -> automatic scrolling
        CurrentPCAutoScroll();
    }

    const ImGuiWindowFlags win_flags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::SetNextWindowSize(debug_win_size);

    if (ImGui::Begin("Debug Instructions", &machineInfo.instruction_debug_enabled, win_flags)) {
        if (gameState.game_running && !debugAutoRun) {
            if (ImGui::Button("Next Instruction")) {
                machineInfo.pause_execution = CheckBreakPoint();
            }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::Button("Next Instruction");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (gameState.game_running) {
            if (ImGui::Button("Jump to PC")) {
                ActionScrollToCurrentPC();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                ActionRequestReset();
            }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::Button("Jump to PC");
            ImGui::SameLine();
            ImGui::Button("Reset");
            ImGui::EndDisabled();
        }

        ImGui::Checkbox("Auto run", &debugAutoRun);
        if (debugAutoRun) {
            machineInfo.pause_execution = CheckBreakPoint();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Send to *_instructions.log", &machineInfo.instruction_logging);

        const ImGuiTableFlags table_flags = 
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_BordersOuterH;

        const ImGuiTableColumnFlags column_flags =
            ImGuiTableColumnFlags_NoHeaderLabel |
            ImGuiTableColumnFlags_NoResize |
            ImGuiTableColumnFlags_WidthFixed;

        const int column_num_instr = DEBUG_INSTR_COLUMNS.size();

        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::TextColored(style.Colors[ImGuiCol_TabActive], "Instructions:");
        if (ImGui::BeginTable("instruction_debug", column_num_instr, table_flags)) {

            for (int i = 0; i < column_num_instr; i++) {
                ImGui::TableSetupColumn("", column_flags, DEBUG_INSTR_COLUMNS[i] * (debug_win_size.x - (style.WindowPadding.x * 2)));
            }

            if (!machineInfo.program_buffer.empty()) {
                // print instructions
                const ImGuiSelectableFlags sel_flags = ImGuiSelectableFlags_SpanAllColumns;

                Vec2 current_index = debugScrollStartIndex;
                Vec2 current_end_index = debugScrollEndIndex;

                bool overflow = current_index.x < current_end_index.x;
                int start_bank_size = machineInfo.program_buffer[current_index.x].size();

                for (; current_index.x != current_end_index.x ? true : current_index.y <= current_end_index.y; current_index.y++) {
                    if (overflow) {
                        if (current_index.y == start_bank_size) {
                            current_index.y = 0;
                            current_index.x++;

                            overflow = false;
                        }
                    }

                    ImGui::TableNextColumn();

                    if (debugCurrentBreakpointSet) {
                        if (current_index == debugCurrentBreakpoint) {
                            ImGui::TextColored(style.Colors[ImGuiCol_HeaderHovered], ">>>");
                        }
                    }
                    ImGui::TableNextColumn();

                    if (current_index.x == debugInstrIndex.x) {
                        ImGui::Selectable(get<2>(machineInfo.program_buffer[current_index.x][current_index.y]).c_str(), get<0>(machineInfo.program_buffer[current_index.x][current_index.y]) == debugInstrIndex.y, sel_flags);
                    }
                    else {
                        ImGui::Selectable(get<2>(machineInfo.program_buffer[current_index.x][current_index.y]).c_str(), false, sel_flags);
                    }
                    
                    // process actions
                    if (ImGui::IsItemHovered()) {
                        if (sdlScrollUp) {
                            sdlScrollUp = false;
                            if (sdlkShiftDown) { ActionDebugScrollUp(DEBUG_INSTR_ELEMENTS); }
                            else { ActionDebugScrollUp(1); }
                        }
                        else if (sdlScrollDown) {
                            sdlScrollDown = false;
                            if (sdlkShiftDown) { ActionDebugScrollDown(DEBUG_INSTR_ELEMENTS); }
                            else { ActionDebugScrollDown(1); }
                        }
                        if (ImGui::IsMouseClicked(0)) { SetBreakPoint(current_index); }
                    }
                    ImGui::TableNextColumn();

                    ImGui::TextUnformatted(get<3>(machineInfo.program_buffer[current_index.x][current_index.y]).c_str());
                    ImGui::TableNextRow();
                }
            }
            else {
                for (int i = 0; i < DEBUG_INSTR_ELEMENTS; i++) {
                    ImGui::TableNextColumn();

                    ImGui::TableNextColumn();

                    ImGui::TextUnformatted("");
                    ImGui::TableNextColumn();

                    ImGui::TextUnformatted("");
                    ImGui::TableNextRow();
                }
            }
        }
        ImGui::EndTable();

        if (gameState.game_running) { 
            if (ImGui::InputInt("ROM Bank", &debugScrollStartIndex.x)) {
                ActionBankSwitch();
            }
            const ImGuiInputTextFlags bank_addr_flags =
                ImGuiInputTextFlags_CharsHexadecimal |
                ImGuiInputTextFlags_CharsUppercase |
                ImGuiInputTextFlags_EnterReturnsTrue;
            if (ImGui::InputInt("ROM Address", &debugAddrToSearch, 1, 100, bank_addr_flags)) {
                ActionBankJumpToAddr();
            }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::InputInt("ROM Bank", &debugScrollStartIndex.x);
            ImGui::InputInt("ROM Address", &debugAddrToSearch, 1, 100, 0);
            ImGui::EndDisabled();
        }
        

        const int column_num_registers = DEBUG_REGISTER_COLUMNS.size();

        ImGui::TextColored(style.Colors[ImGuiCol_TabActive], "Registers:");

        if (!machineInfo.register_values.empty()) {
            if (ImGui::BeginTable("registers_debug", column_num_registers, table_flags)) {
                for (int i = 0; i < column_num_registers; i++) {
                    ImGui::TableSetupColumn("", column_flags, DEBUG_REGISTER_COLUMNS[i] * (debug_win_size.x - (style.WindowPadding.x * 2)));
                }

                ImGui::TableNextColumn();
                for (int i = 0; const auto & n : machineInfo.register_values) {
                    ImGui::TextUnformatted(n.first.c_str());
                    ImGui::TableNextColumn();

                    ImGui::TextUnformatted(n.second.c_str());

                    if (i++ % 2) { ImGui::TableNextRow(); }
                    ImGui::TableNextColumn();
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void ImGuiGameboyX::ShowHardwareInfo() {
    const ImGuiWindowFlags win_flags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::SetNextWindowSize(hw_info_win_size);

    if (ImGui::Begin("Hardware Info", &machineInfo.track_hardware_info, win_flags)) {

        static const ImGuiTableColumnFlags column_flags =
            ImGuiTableColumnFlags_NoHeaderLabel |
            ImGuiTableColumnFlags_NoResize;

        static const ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV;

        if (ImGui::BeginTable("hardware_info", 2, table_flags)) {
            ImGuiStyle& style = ImGui::GetStyle();

            static const int column_num = HW_INFO_COLUMNS.size();

            for (int i = 0; i < column_num; i++) {
                ImGui::TableSetupColumn("", column_flags, HW_INFO_COLUMNS[i] * (hw_info_win_size.x - (style.WindowPadding.x * 2)));
            }

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("CPU Clock");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted((to_string(machineInfo.current_frequency) + " MHz").c_str());
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("ROM Banks");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(to_string(machineInfo.rom_bank_num).c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("ROM Selected");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(to_string(machineInfo.rom_bank_selected).c_str());
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("RAM Banks");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(to_string(machineInfo.ram_bank_num).c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("RAM Selected");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(machineInfo.ram_bank_selected < machineInfo.ram_bank_num ? to_string(machineInfo.ram_bank_selected).c_str() : "-");
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("RTC Register");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(machineInfo.ram_bank_selected >= machineInfo.ram_bank_num ? to_string(machineInfo.ram_bank_selected).c_str() : "-");
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("WRAM Banks");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(to_string(machineInfo.wram_bank_num).c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("WRAM Selected");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(to_string(machineInfo.wram_bank_selected).c_str());
            
            ImGui::PopStyleVar();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void ImGuiGameboyX::ShowNewGameDialog() {
    check_and_create_config_folders();

    string current_path = get_current_path();
    string s_path_rom_folder = current_path + ROM_FOLDER;
    
    auto filter_items = new nfdfilteritem_t[sizeof(nfdfilteritem_t) * FILE_EXTS.size()];
    for (int i = 0; i < FILE_EXTS.size(); i++) {
        filter_items[i] = { FILE_EXTS[i][0].c_str(), FILE_EXTS[i][1].c_str() };
    }

    nfdchar_t* out_path = nullptr;
    const auto result = NFD_OpenDialog(&out_path, filter_items, 2, s_path_rom_folder.c_str());
    delete[] filter_items;

    if (result == NFD_OKAY) {
        if (out_path != nullptr) {
            string path_to_rom(out_path);
            if (!ActionAddGame(path_to_rom)) { return; }
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

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IMGUI_BG_COLOR);
    if (ImGui::Begin("games_select", nullptr, win_flags)) {

        static const ImGuiTableFlags table_flags =
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersInnerV;

        static const int column_num = GAMES_COLUMNS.size();

        static const ImGuiTableColumnFlags col_flags = ImGuiTableColumnFlags_WidthFixed;

        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 10, 10 });

        if (ImGui::BeginTable("game_list", column_num, table_flags)) {

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, style.CellPadding.y * 2));

            for (int i = 0; const auto & [label, fraction] : GAMES_COLUMNS) {
                ImGui::TableSetupColumn(label.c_str(), col_flags, main_viewport->Size.x * fraction);
                ImGui::TableSetupScrollFreeze(i, 0);
                i++;
            }
            ImGui::TableHeadersRow();

            for (int i = 0; const auto & game : games) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                // TODO: icon
                ImGui::TableNextColumn();

                const ImGuiSelectableFlags sel_flags = ImGuiSelectableFlags_SpanAllColumns;
                ImGui::Selectable((game.is_cgb ? GAMES_CONSOLES[1].c_str() : GAMES_CONSOLES[0].c_str()), gamesSelected[i], sel_flags);

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    for (int j = 0; j < gamesSelected.size(); j++) {
                        gamesSelected[i] = i == j;
                    }

                    ActionStartGame(i);
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                    if (sdlkShiftDown) {
                        static int lower, upper;
                        if (i <= gameSelectedIndex) {
                            lower = i;
                            upper = gameSelectedIndex;
                        }
                        else {
                            lower = gameSelectedIndex;
                            upper = i;
                        }

                        for (int j = 0; j < gamesSelected.size(); j++) {
                            gamesSelected[j] = ((j >= lower) && (j <= upper)) || (sdlkCtrlDown ? gamesSelected[j] : false);
                        }
                    }
                    else {
                        for (int j = 0; j < gamesSelected.size(); j++) {
                            gamesSelected[j] = (i == j ? !gamesSelected[j] : (sdlkCtrlDown ? gamesSelected[j] : false));
                        }
                        gameSelectedIndex = i;
                        if (!sdlkCtrlDown) {
                            gamesSelected[i] = true;
                        }
                    }
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) {
                    // TODO: context menu
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(game.title.c_str());
                ImGui::SetItemTooltip(game.title.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(game.file_path.c_str());
                ImGui::SetItemTooltip(game.file_path.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(game.file_name.c_str());
                ImGui::SetItemTooltip(game.file_name.c_str());

                i++;
            }
            ImGui::PopStyleVar();
        }
        ImGui::PopStyleVar();
        ImGui::EndTable();
        ImGui::End();
    }
    ImGui::PopStyleColor();
}

/* ***********************************************************************************************************
    ACTIONS TO READ/WRITE/PROCESS DATA ETC.
*********************************************************************************************************** */
bool ImGuiGameboyX::ActionAddGame(const string& _path_to_rom) {
    auto game_ctx = game_info();

    if (!Cartridge::read_new_game(game_ctx, _path_to_rom)) {
        showNewGameDialog = false;
        return false;
    }

    for (const auto& n : games) {
        if (game_ctx == n) {
            LOG_WARN("Game already added ! Process aborted");
            showNewGameDialog = false;
            return false;
        }
    }

    if (const vector<game_info> new_game = { game_ctx }; !write_games_to_config(new_game, CONFIG_FOLDER + GAMES_CONFIG_FILE, false)) {
        showNewGameDialog = false;
        return false;
    }

    AddGameGuiCtx(game_ctx);
    return true;
}

void ImGuiGameboyX::ActionDeleteGames() {
    auto index = vector<int>();
    for (int i = 0; const auto & n : gamesSelected) {
        if (n) index.push_back(i);
        i++;
    }

    auto games_delete = DeleteGamesGuiCtx(index);

    if (games_delete.size() > 0) {
        delete_games_from_config(games_delete, CONFIG_FOLDER + GAMES_CONFIG_FILE);
    }

    InitGamesGuiCtx();

    deleteGames = false;
}

void ImGuiGameboyX::ActionProcessSpecialKeys() {
    if (sdlkCtrlDown) {
        if (sdlkADown && sdlkCtrlDownFirst) {
            for (int i = 0; i < gamesSelected.size(); i++) {
                gamesSelected[i] = true;
            }
        }
    }
    if (sdlkDelDown) {
        ActionDeleteGames();
        sdlkDelDown = false;
    }
}

void ImGuiGameboyX::ActionStartGame(int _index) {
    gameState.pending_game_start = true;
    gameState.game_to_start = _index;
    firstInstruction = true;
}

void ImGuiGameboyX::ActionEndGame() {
    if (gameState.game_running) {
        for (int i = 0; i < gamesSelected.size(); i++) {
            gamesSelected[i] = false;
        }
        gamesSelected[gameState.game_to_start] = true;
    }
    gameState.pending_game_stop = false;
    ResetDebugInstr();
}

void ImGuiGameboyX::ActionRequestReset() {
    gameState.request_reset = true;
}

void ImGuiGameboyX::ActionDebugScrollUp(const int& _num) {
    static bool full_scroll = true;

    if (debugScrollStartIndex.x > 0 || debugScrollStartIndex.y > 0) {
        debugScrollStartIndex.y -= _num;
        if (debugScrollStartIndex.y < 0) {
            if (debugScrollStartIndex.x == 0) {
                debugScrollStartIndex.y = 0;
                full_scroll = false;
            }
            else {
                debugScrollStartIndex.x--;
                debugScrollStartIndex.y += machineInfo.program_buffer[debugScrollStartIndex.x].size();
            }
        }

        if (full_scroll) {
            debugScrollEndIndex.y -= _num;
            if (debugScrollEndIndex.y < 0) {
                debugScrollEndIndex.x--;
                debugScrollEndIndex.y += machineInfo.program_buffer[debugScrollEndIndex.x].size();
            }
        }
        else {
            debugScrollEndIndex.y = debugScrollStartIndex.y + DEBUG_INSTR_ELEMENTS - 1;
        }
    }

    DebugSearchAddrSet();
}

void ImGuiGameboyX::ActionDebugScrollDown(const int& _num) {
    static bool full_scroll = true;

    if (debugScrollEndIndex.x < machineInfo.program_buffer.size() - 1 || debugScrollEndIndex.y < machineInfo.program_buffer[debugScrollEndIndex.x].size() - 1) {
        debugScrollEndIndex.y += _num;
        if (debugScrollEndIndex.y >= machineInfo.program_buffer[debugScrollEndIndex.x].size()) {
            if (debugScrollEndIndex.x == machineInfo.program_buffer.size() - 1) {
                debugScrollEndIndex.y = machineInfo.program_buffer[debugScrollEndIndex.x].size() - 1;
                full_scroll = false;
            }
            else {
                debugScrollEndIndex.y -= machineInfo.program_buffer[debugScrollEndIndex.x].size();
                debugScrollEndIndex.x++;
            }
        }
        
        
        if (full_scroll) {
            debugScrollStartIndex.y += _num;
            if (debugScrollStartIndex.y >= machineInfo.program_buffer[debugScrollStartIndex.x].size()) {
                debugScrollStartIndex.y -= machineInfo.program_buffer[debugScrollStartIndex.x].size();
                debugScrollStartIndex.x++;
            }
        }
        else {
            debugScrollStartIndex.y = debugScrollEndIndex.y - (DEBUG_INSTR_ELEMENTS - 1);
        }
    }

    DebugSearchAddrSet();
}

void ImGuiGameboyX::ActionBankSwitch() {
    if (debugScrollStartIndex.x < 0) { debugScrollStartIndex.x = 0; }
    if (debugScrollStartIndex.x >= machineInfo.rom_bank_num) { debugScrollStartIndex.x = machineInfo.rom_bank_num - 1; }

    debugScrollStartIndex.y = 0;

    debugScrollEndIndex.x = debugScrollStartIndex.x;
    debugScrollEndIndex.y = debugScrollStartIndex.y + DEBUG_INSTR_ELEMENTS - 1;

    DebugSearchAddrSet();
}

void ImGuiGameboyX::ActionBankJumpToAddr() {
    // limit given address to address space of current bank
    if (debugAddrToSearch < get<1>(machineInfo.program_buffer[debugScrollStartIndex.x][0]))
    {
        debugAddrToSearch = get<1>(machineInfo.program_buffer[debugScrollStartIndex.x][0]);
    }
    if ((int)((float)debugAddrToSearch / machineInfo.rom_bank_size) >
        (int)((float)get<1>(machineInfo.program_buffer[debugScrollStartIndex.x].back()) / machineInfo.rom_bank_size))
    {
        debugAddrToSearch = get<1>(machineInfo.program_buffer[debugScrollStartIndex.x].back());
    }

    int index;
    int prev_addr, next_addr;
    int bank_size = machineInfo.program_buffer[debugScrollStartIndex.x].size();
    for (index = 0; index < bank_size - 1; index++) {
        prev_addr = get<1>(machineInfo.program_buffer[debugScrollStartIndex.x][index]);

        if (index + 1 == bank_size) { next_addr = bank_size; }
        else { next_addr = get<1>(machineInfo.program_buffer[debugScrollStartIndex.x][index + 1]); }

        if (prev_addr <= debugAddrToSearch &&
            next_addr > debugAddrToSearch)
        {
            break;
        }
    }

    if (index > debugScrollStartIndex.y) {
        ActionDebugScrollDown(index - debugScrollStartIndex.y);
    }
    else if (index < debugScrollStartIndex.y) {
        ActionDebugScrollUp(debugScrollStartIndex.y - index);
    }
}

void ImGuiGameboyX::ActionScrollToCurrentPC() {
    for (const auto& n : machineInfo.program_buffer[debugInstrIndex.x]) {
        if (get<1>(n) == machineInfo.current_pc) {
            debugInstrIndex.y = get<0>(n);

            BankScrollAddrSet(debugInstrIndex.x, debugInstrIndex.y);
            break;
        }
    }
}

/* ***********************************************************************************************************
    HELPER FUNCTIONS FOR MANAGING MEMBERS OF GUI OBJECT
*********************************************************************************************************** */
void ImGuiGameboyX::AddGameGuiCtx(const game_info& _game_ctx) {
    games.push_back(_game_ctx);
    gamesSelected.clear();
    for (const auto& game : games) {
        gamesSelected.push_back(false);
    }
    gamesSelected.back() = true;
    gameSelectedIndex = gamesSelected.size() - 1;
}

vector<game_info> ImGuiGameboyX::DeleteGamesGuiCtx(const vector<int>& _index) {
    auto result = vector<game_info>();

    for (int i = _index.size() - 1; i >= 0; i--) {
        const game_info game_ctx = games[_index[i]];
        result.push_back(game_ctx);
        games.erase(games.begin() + _index[i]);
        gamesSelected.erase(gamesSelected.begin() + _index[i]);
    }

    if (result.size() > 0) {
        gameSelectedIndex = 0;
    }

    return result;
}

void ImGuiGameboyX::InitGamesGuiCtx() {
    for (const auto& n : games) {
        gamesSelected.push_back(false);
    }
}

void ImGuiGameboyX::ResetDebugInstr() {
    debugInstrIndex = Vec2(0, 0);
    debugScrollStartIndex = Vec2(0, 0);
    debugScrollEndIndex = Vec2(0, 0);
    debugAddrToSearch = 0;
    debugCurrentBreakpoint = Vec2(0, 0);
    debugCurrentBreakpointSet = false;
}

void ImGuiGameboyX::DebugSearchAddrSet() {
    debugAddrToSearch = get<1>(machineInfo.program_buffer[debugScrollStartIndex.x][debugScrollStartIndex.y]);
}

void ImGuiGameboyX::BankScrollAddrSet(const int& _bank, const int& _index) {
    int debug_start;
    int debug_end;

    debug_start = _index - (DEBUG_INSTR_ELEMENTS / 2);
    if (_bank == 0 && debug_start < 0) {
        debug_start = 0;
    }

    debug_end = debug_start + DEBUG_INSTR_ELEMENTS - 1;
    if (_bank == machineInfo.rom_bank_num - 1 && debug_end > machineInfo.program_buffer[machineInfo.rom_bank_num - 1].size() - 1) {
        debug_end = machineInfo.program_buffer[machineInfo.rom_bank_num - 1].size() - 1;
    }

    if (debug_start < 0) {
        debugScrollStartIndex = Vec2(_bank - 1, machineInfo.program_buffer[_bank - 1].size() + debug_start);
    }
    else {
        debugScrollStartIndex = Vec2(_bank, debug_start);
    }

    if (debug_end > machineInfo.program_buffer[_bank].size() - 1) {
        debugScrollEndIndex = Vec2(_bank + 1, debug_end - (machineInfo.program_buffer[_bank].size() - 1));
    }
    else {
        debugScrollEndIndex = Vec2(_bank, debug_end);
    }

    DebugSearchAddrSet();
}

void ImGuiGameboyX::CurrentPCAutoScroll() {
    if (machineInfo.current_pc != machineInfo.last_pc) {
        machineInfo.last_pc = machineInfo.current_pc;
        debugInstrIndex.x = machineInfo.current_rom_bank;

        ActionScrollToCurrentPC();
    }
}

bool ImGuiGameboyX::CheckBreakPoint() {
    if (debugCurrentBreakpointSet) {
        return debugInstrIndex == debugCurrentBreakpoint;
    }
    else {
        return false;
    }
}

void ImGuiGameboyX::SetBreakPoint(const Vec2& _current_index) {
    if (debugCurrentBreakpointSet) {
        if (debugCurrentBreakpoint == _current_index) {
            debugCurrentBreakpointSet = false;
        }
        else {
            debugCurrentBreakpoint = _current_index;
        }
    }
    else {
        debugCurrentBreakpoint = _current_index;
        debugCurrentBreakpointSet = true;
    }
}

/* ***********************************************************************************************************
    COMMUNICATION WITH MAIN
*********************************************************************************************************** */
game_info& ImGuiGameboyX::GetGameStartContext() {
    //game_running = true;
    //pending_game_start = false;
    return games[gameState.game_to_start];
}

void ImGuiGameboyX::GameStopped() {
    ActionEndGame();
}

/* ***********************************************************************************************************
    IMGUIGAMEBOYX SDL FUNCTIONS
*********************************************************************************************************** */
void ImGuiGameboyX::EventKeyDown(const SDL_Keycode& _key) {
    switch (_key) {
    case SDLK_a:
        sdlkADown = true;
        break;
    case SDLK_LSHIFT:
        sdlkShiftDown = true;
        break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        sdlkCtrlDown = true;
        sdlkCtrlDownFirst = !sdlkADown;
        break;
    case SDLK_DELETE:
        sdlkDelDown = true;
        break;
    default:
        break;
    }
}

void ImGuiGameboyX::EventKeyUp(const SDL_Keycode& _key) {
    switch (_key) {
    case SDLK_a:
        sdlkADown = false;
        break;
    case SDLK_F10:
        showMainMenuBar = !showMainMenuBar;
        break;
    case SDLK_LSHIFT:
        sdlkShiftDown = false;
        break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        sdlkCtrlDown = false;
        break;
    case SDLK_DELETE:
        sdlkDelDown = false;
        break;
    default:
        break;
    }
}

void ImGuiGameboyX::EventMouseWheel(const Sint32& _wheel_y) {
    if (_wheel_y > 0) {
        sdlScrollUp = true;
    }
    else if (_wheel_y < 0) {
        sdlScrollDown = true;
    }
}

/* ***********************************************************************************************************
    HELPER FUNCTIONS
*********************************************************************************************************** */
static void create_fs_hierarchy() {
    check_and_create_config_folders();
    check_and_create_config_files();
    check_and_create_log_folders();
    Cartridge::check_and_create_rom_folder();
}
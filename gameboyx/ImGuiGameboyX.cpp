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
#include "imguigameboyx_config.h"
#include "io_config.h"
#include <format>
#include <cmath>

using namespace std;

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

ImGuiGameboyX::ImGuiGameboyX(machine_information& _machine_info, game_status& _game_status) : machineInfo(_machine_info), gameState(_game_status) {
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

    if (showMainMenuBar) { ShowMainMenuBar(); }
    if (machineInfo.instruction_debug_enabled) { ShowDebugInstructions(); }
    if (showWinAbout) { ShowWindowAbout(); }
    if (machineInfo.track_hardware_info) { ShowHardwareInfo(); }
    if (showMemoryInspector) { ShowDebugMemoryInspector(); }

    if (gameState.game_running) {
        if (machineInfo.instruction_debug_enabled) {
            if (machineInfo.instruction_logging) { WriteInstructionLog(); }
        }
    }
    else {
        // gui elements
        if (showGameSelect) { ShowGameSelect(); }
        if (showNewGameDialog) { ShowNewGameDialog(); }

        // actions
        if (deleteGames) { ActionDeleteGames(); }
        if (gameState.pending_game_start) { ActionStartGame(gameSelectedIndex); }

        // special functions
        ProcessMainMenuKeys();
    }

    ResetEventMouseWheel();
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
            ImGui::MenuItem("Hardware Info", nullptr, &machineInfo.track_hardware_info);
            ImGui::MenuItem("Debug Instructions", nullptr, &machineInfo.instruction_debug_enabled);
            ImGui::MenuItem("Memory Inspector", nullptr, &showMemoryInspector);
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
    if (ImGui::Begin("About", &showWinAbout, WIN_CHILD_FLAGS)) {
        ImGui::Text("GameboyX Emulator");
        ImGui::Text("Version: %s %d.%d", GBX_RELEASE, GBX_VERSION_MAJOR, GBX_VERSION_MINOR, GBX_VERSION_PATCH);
        ImGui::Text("Author: %s", GBX_AUTHOR);
        ImGui::Spacing();
    }
    ImGui::End();
}

void ImGuiGameboyX::ShowDebugInstructions() {
    ImGui::SetNextWindowSize(debug_instr_win_size);

    if (ImGui::Begin("Debug Instructions", &machineInfo.instruction_debug_enabled, WIN_CHILD_FLAGS)) {
        if (gameState.game_running) {
            if (CheckCurrentPCAutoScroll() || CheckScroll(machineInfo.program_buffer))
            { 
                SetBankAndAddress(machineInfo.program_buffer, dbgInstrBank, dbgInstrAddress);
            }
        }

        if (gameState.game_running && !dbgInstrAutorun) {
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
            if (ImGui::Button("Jump to PC")) { ActionScrollToCurrentPC(); }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) { ActionRequestReset(); }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::Button("Jump to PC");
            ImGui::SameLine();
            ImGui::Button("Reset");
            ImGui::EndDisabled();
        }

        ImGui::Checkbox("Auto run", &dbgInstrAutorun);
        if (dbgInstrAutorun) {
            machineInfo.pause_execution = CheckBreakPoint();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Send to *_instructions.log", &machineInfo.instruction_logging);

        ImGui::Separator();
        ImGui::TextColored(GUI_STYLE.Colors[ImGuiCol_TabActive], "Instructions:");
        if (ImGui::BeginTable("instruction_debug", dbgInstrColNum, TABLE_FLAGS)) {

            for (int i = 0; i < dbgInstrColNum; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, DEBUG_INSTR_COLUMNS[i]);
            }
            ImGui::TableHeadersRow();

            if (gameState.game_running) {

                bank_index& current_index = machineInfo.program_buffer.GetCurrentIndex();
                while (machineInfo.program_buffer.GetNextEntry(dbgInstrCurrentEntry)) {
                    ImGui::TableNextColumn();

                    if (dbgInstrBreakpointSet && current_index == dbgInstrCurrentBreakpoint) {
                        ImGui::TextColored(GUI_STYLE.Colors[ImGuiCol_HeaderHovered], ">>>");
                    }
                    ImGui::TableNextColumn();

                    ImGui::Selectable(dbgInstrCurrentEntry.first.c_str(), current_index == dbgInstrInstructionIndex, SEL_FLAGS);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                        SetBreakPoint(current_index);
                    }
                    ImGui::TableNextColumn();

                    ImGui::TextUnformatted(dbgInstrCurrentEntry.second.c_str());
                    ImGui::TableNextRow();
                }
            }
            else {
                for (int i = 0; i < DEBUG_INSTR_LINES; i++) {
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
            if (ImGui::InputInt("ROM Bank", &dbgInstrBank)) {
                ActionBankSwitch(machineInfo.program_buffer, dbgInstrBank);
            }
            if (ImGui::InputInt("ROM Address", &dbgInstrAddress, 1, 100, INPUT_INT_HEX_FLAGS)) {
                ActionSearchAddress(machineInfo.program_buffer, dbgInstrAddress);
            }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::InputInt("ROM Bank", &dbgInstrBank);
            ImGui::InputInt("ROM Address", &dbgInstrAddress, 1, 100, 0);
            ImGui::EndDisabled();
        }

        ImGui::Separator();
        ImGui::TextColored(GUI_STYLE.Colors[ImGuiCol_TabActive], "Registers:");
        if (!machineInfo.register_values.empty()) {
            if (ImGui::BeginTable("registers_debug", dbgInstrColNumRegs, TABLE_FLAGS)) {
                for (int i = 0; i < dbgInstrColNumRegs; i++) {
                    ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, DEBUG_REGISTER_COLUMNS[i]);
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

void ImGuiGameboyX::ShowDebugMemoryInspector() {
    ImGui::SetNextWindowSize(debug_mem_win_size);

    if (ImGui::Begin("Memory Inspector", &showMemoryInspector, WIN_CHILD_FLAGS)) {
        if (ImGui::BeginTabBar("memory_inspector_tabs", 0)) {

            if (int i = 0;  gameState.game_running) {
                for (auto& [name, tables] : machineInfo.memory_buffer) {
                    if (ImGui::BeginTabItem(name.c_str())) {
                        ImGui::InputInt("Memory Bank", &dbgMemBankIndex[i]);

                        ScrollableTable<memory_data>& table = tables[dbgMemBankIndex[i]];
                        int i;

                        if (ImGui::BeginTable(name.c_str(), dbgMemColNum, TABLE_FLAGS)) {
                            for (int i = 0; i < dbgMemColNum; i++) {
                                ImGui::TableSetupColumn(DEBUG_MEM_COLUMNS[i].first.c_str(), TABLE_COLUMN_FLAGS, DEBUG_MEM_COLUMNS[i].second);
                            }
                            ImGui::TableHeadersRow();

                            while (table.GetNextEntry(dbgMemCurrentEntry)) {
                                ImGui::TableNextColumn();

                                ImGui::TextColored(GUI_STYLE.Colors[ImGuiCol_HeaderHovered], get<0>(dbgMemCurrentEntry).c_str());
                                ImGui::TableNextColumn();

                                u8* ref = get<2>(dbgMemCurrentEntry);
                                for (i = 0; i < get<1>(dbgMemCurrentEntry); i++) {
                                    ImGui::TextUnformatted(format("{:x}", ref[i]).c_str());
                                    ImGui::TableNextColumn();
                                }
                                for (; i < DEBUG_MEM_ELEM_PER_LINE; i++) {
                                    ImGui::TextUnformatted("");
                                    ImGui::TableNextColumn();
                                }

                                ImGui::TableNextRow();
                            }

                            ImGui::EndTable();
                        }

                        ImGui::EndTabItem();
                        i++;
                    }
                }
            }
            else {
                if (ImGui::BeginTabItem("NONE")) {
                    ImGui::BeginDisabled();
                    ImGui::InputInt("Bank", &i);
                    ImGui::EndDisabled();

                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void ImGuiGameboyX::ShowHardwareInfo() {
    ImGui::SetNextWindowSize(hw_info_win_size);

    if (ImGui::Begin("Hardware Info", &machineInfo.track_hardware_info, WIN_CHILD_FLAGS)) {
        if (ImGui::BeginTable("hardware_info", 2, TABLE_FLAGS_NO_BORDER_OUTER_H)) {
            static const int column_num = HW_INFO_COLUMNS.size();

            for (int i = 0; i < column_num; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, HW_INFO_COLUMNS[i]);
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
    ImGui::SetNextWindowPos(MAIN_VIEWPORT->WorkPos);
    ImGui::SetNextWindowSize(MAIN_VIEWPORT->Size);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IMGUI_BG_COLOR);
    if (ImGui::Begin("games_select", nullptr, MAIN_WIN_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 10, 10 });

        if (ImGui::BeginTable("game_list", mainColNum, MAIN_TABLE_FLAGS)) {

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GUI_STYLE.ItemSpacing.x, GUI_STYLE.CellPadding.y * 2));

            for (int i = 0; const auto & [label, fraction] : GAMES_COLUMNS) {
                ImGui::TableSetupColumn(label.c_str(), TABLE_COLUMN_FLAGS, MAIN_VIEWPORT->Size.x * fraction);
                ImGui::TableSetupScrollFreeze(i, 0);
                i++;
            }
            ImGui::TableHeadersRow();

            for (int i = 0; const auto & game : games) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                // TODO: icon
                ImGui::TableNextColumn();

                ImGui::Selectable((game.is_cgb ? GAMES_CONSOLES[1].c_str() : GAMES_CONSOLES[0].c_str()), gamesSelected[i], SEL_FLAGS);

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

// ***** ADD/REMOVE GAMES *****
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

// ***** START/END GAME CONTROL *****
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

    // reset debug windows
    ResetDebugInstr();
    ResetMemInspector();
}

void ImGuiGameboyX::ActionRequestReset() {
    gameState.request_reset = true;
    dbgInstrLastPC = -1;
}

// ***** ACTIONS FOR DEBUG INSTRUCTION WINDOW *****

void ImGuiGameboyX::ActionBankSwitch(ScrollableTableBase& _table_obj, int& _bank) {
    _table_obj.SearchBank(_bank);
}

void ImGuiGameboyX::ActionSearchAddress(ScrollableTableBase& _table_obj, int& _address) {
    _table_obj.SearchAddress(_address);
}

void ImGuiGameboyX::ActionScrollToCurrentPC() {
    machineInfo.program_buffer.SearchBank(dbgInstrInstructionIndex.bank);
    machineInfo.program_buffer.SearchAddress(dbgInstrLastPC);
}

// ***** ACTIONS FOR MEMORY INSPECTOR WINDOW *****

/* ***********************************************************************************************************
    HELPER FUNCTIONS FOR MANAGING MEMBERS OF GUI OBJECT
*********************************************************************************************************** */

// ***** MANAGE GAME ENTRIES OF GUI OBJECT *****
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

// ***** UNIVERSIAL CHECKER FOR SCROLL ACTION RELATED TO ANY WINDOW WITH CUSTOM SCROLLABLE TABLE *****
bool ImGuiGameboyX::CheckScroll(ScrollableTableBase& _table_obj) {
    if ((ImGui::IsWindowHovered() || ImGui::IsWindowFocused())) {
        if (sdlScrollDown || sdlScrollUp) {
            if (sdlScrollUp) {
                sdlScrollUp = false;
                if (sdlkShiftDown) { _table_obj.ScrollUpPage(); }
                else { _table_obj.ScrollUp(1); }
            }
            else if (sdlScrollDown) {
                sdlScrollDown = false;
                if (sdlkShiftDown) { _table_obj.ScrollDownPage(); }
                else { _table_obj.ScrollDown(1); }
            }
        }

        return true;
    }

    return false;
}

// ***** program counter auto scroll *****
bool ImGuiGameboyX::CheckCurrentPCAutoScroll() {
    if (machineInfo.current_pc != dbgInstrLastPC) {

        dbgInstrLastPC = machineInfo.current_pc;
        dbgInstrInstructionIndex.bank = machineInfo.current_rom_bank;

        ActionScrollToCurrentPC();

        dbgInstrInstructionIndex.index = machineInfo.program_buffer.GetIndexByAddress(dbgInstrLastPC).index;

        return true;
    }

    return false;
}

// ***** DEBUG INSTRUCTION PROCESS BREAKPOINT *****
bool ImGuiGameboyX::CheckBreakPoint() {
    if (dbgInstrBreakpointSet) {
        return dbgInstrInstructionIndex == dbgInstrCurrentBreakpoint;
    }
    else {
        return false;
    }
}

void ImGuiGameboyX::SetBreakPoint(const bank_index& _current_index) {
    if (dbgInstrBreakpointSet) {
        if (dbgInstrCurrentBreakpoint == _current_index) {
            dbgInstrBreakpointSet = false;
        }
        else {
            dbgInstrCurrentBreakpoint = _current_index;
        }
    }
    else {
        dbgInstrCurrentBreakpoint = _current_index;
        dbgInstrBreakpointSet = true;
    }
}

void ImGuiGameboyX::SetBankAndAddress(ScrollableTableBase& _tyble_obj, int& _bank, int& _address){
    bank_index centre = _tyble_obj.GetCurrentIndexCentre();
    _bank = centre.bank;
    _address = _tyble_obj.GetAddressByIndex(centre);
}

/* ***********************************************************************************************************
    SET/RESET AT GAME START/STOP
*********************************************************************************************************** */
// ***** SETUP/RESET FUNCTIONS FOR DEBUG WINDOWS *****
void ImGuiGameboyX::ResetDebugInstr() {
    dbgInstrInstructionIndex = bank_index(0, 0);
    dbgInstrCurrentBreakpoint = bank_index(0, 0);
    dbgInstrBreakpointSet = false;
}

void ImGuiGameboyX::ResetMemInspector() {
    dbgMemBankIndex.clear();
}

void ImGuiGameboyX::SetupMemInspectorIndex() {
    ResetMemInspector();

    for (const auto& n : machineInfo.memory_buffer) {
        dbgMemBankIndex.emplace_back(0);
    }
}

/* ***********************************************************************************************************
    LOG DATA IO
*********************************************************************************************************** */
// ***** WRITE EXECUTED INSTRUCTIONS + RESULTS TO LOG FILE *****
void ImGuiGameboyX::WriteInstructionLog() {
    if (machineInfo.current_instruction.compare("") != 0) {
        string output = machineInfo.current_instruction;
        write_to_debug_log(output, LOG_FOLDER + games[gameSelectedIndex].title + DEBUG_INSTR_LOG, firstInstruction);
        firstInstruction = false;

        machineInfo.current_instruction = "";
    }
}

/* ***********************************************************************************************************
    MAIN DIRECT COMMUNICATION
*********************************************************************************************************** */
game_info& ImGuiGameboyX::GetGameStartContext() {
    //game_running = true;
    //pending_game_start = false;
    return games[gameState.game_to_start];
}

void ImGuiGameboyX::GameStopped() {
    ActionEndGame();
}

void ImGuiGameboyX::GameStartCallback() {
    ResetDebugInstr();
    SetupMemInspectorIndex();
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

void ImGuiGameboyX::ResetEventMouseWheel() {
    sdlScrollUp = false;
    sdlScrollDown = false;
}

void ImGuiGameboyX::ProcessMainMenuKeys() {
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

/* ***********************************************************************************************************
    HELPER FUNCTIONS
*********************************************************************************************************** */
static void create_fs_hierarchy() {
    check_and_create_config_folders();
    check_and_create_config_files();
    check_and_create_log_folders();
    Cartridge::check_and_create_rom_folder();
}
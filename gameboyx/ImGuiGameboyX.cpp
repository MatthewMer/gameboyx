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
#include "general_config.h"
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

ImGuiGameboyX* ImGuiGameboyX::getInstance(machine_information& _machine_info, game_status& _game_status, graphics_information& _graphics_info) {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }

    instance = new ImGuiGameboyX(_machine_info, _game_status, _graphics_info);
    return instance;
}

void ImGuiGameboyX::resetInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
    instance = nullptr;
}

ImGuiGameboyX::ImGuiGameboyX(machine_information& _machine_info, game_status& _game_status, graphics_information& _graphics_info) 
    : machineInfo(_machine_info), gameState(_game_status), graphicsInfo(_graphics_info){
    NFD_Init();
    if (read_games_from_config(this->games, CONFIG_FOLDER + GAMES_CONFIG_FILE)) {
        InitGamesGuiCtx();
    }
    graphicsFPSavg = GUI_IO.Framerate;
    graphicsFPScount = 1;
    for (int i = 0; i < FPS_SAMPLES_NUM; i++) {
        graphicsFPSfifo.push(.0f);
    }
    ActionSetEmulationSpeed(0);
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
    if (showImGuiDebug) { ImGui::ShowDebugLogWindow(&showImGuiDebug); }
    if (showGraphicsInfo) { ShowGraphicsInfo(); }
    if (graphicsShowOverlay) { ShowGraphicsOverlay(); }

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
            if (ImGui::BeginMenu("Emulation speed", &showEmulationSpeedSelect)) {
                for (int index = 0; const auto& [key, value] : EMULATION_SPEEDS) {

                    bool& speed = emulationSpeedsEnabled[index].value;
                    ImGui::MenuItem(value.c_str(), nullptr, &speed);

                    if (speed && currentSpeedIndex != index) {
                        ActionSetEmulationSpeed(index);
                    }
                    index++;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Graphics")) {
            ImGui::MenuItem("Graphics Overlay", nullptr, &graphicsShowOverlay);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Hardware Info", nullptr, &machineInfo.track_hardware_info);
            ImGui::MenuItem("Instruction Debugger", nullptr, &machineInfo.instruction_debug_enabled);
            ImGui::MenuItem("Memory Inspector", nullptr, &showMemoryInspector);
            ImGui::MenuItem("ImGui Debug", nullptr, &showImGuiDebug);
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

    if (ImGui::Begin("Instruction Debugger", &machineInfo.instruction_debug_enabled, WIN_CHILD_FLAGS)) {
        if (gameState.game_running) {
            if (CheckCurrentPCAutoScroll())
            { 
                if (dbgInstrPCoutOfRange) {
                    SetBankAndAddressScrollableTable(machineInfo.program_buffer_tmp, dbgInstrBank, dbgInstrAddress);
                }
                else {
                    SetBankAndAddressScrollableTable(machineInfo.program_buffer, dbgInstrBank, dbgInstrAddress);
                }
            }

            if (dbgInstrPCoutOfRange && CheckScroll(machineInfo.program_buffer_tmp)){
                SetBankAndAddressScrollableTable(machineInfo.program_buffer_tmp, dbgInstrBank, dbgInstrAddress);
            }
            else if(CheckScroll(machineInfo.program_buffer)){
                SetBankAndAddressScrollableTable(machineInfo.program_buffer, dbgInstrBank, dbgInstrAddress);
            }
        }

        if (gameState.game_running) {
            if (ImGui::Button("Next Instruction")) { machineInfo.pause_execution = false; }
            ImGui::SameLine();
            if (ImGui::Button("Jump to PC")) {
                if (dbgInstrPCoutOfRange) { ActionScrollToCurrentPC(machineInfo.program_buffer_tmp); }
                else { ActionScrollToCurrentPC(machineInfo.program_buffer); }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) { ActionRequestReset(); }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::Button("Next Instruction");
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Button("Jump to PC");
            ImGui::SameLine();
            ImGui::Button("Reset");
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto run", &dbgInstrAutorun);
        if (dbgInstrAutorun) {
            if (machineInfo.pause_execution) {
                if (dbgInstrPCoutOfRange) {
                    machineInfo.pause_execution = (std::find(dbgInstrBreakpointsTmp.begin(), dbgInstrBreakpointsTmp.end(), dbgInstrInstructionIndex) != dbgInstrBreakpointsTmp.end());
                }
                else {
                    machineInfo.pause_execution = (std::find(dbgInstrBreakpoints.begin(), dbgInstrBreakpoints.end(), dbgInstrInstructionIndex) != dbgInstrBreakpoints.end());
                }
            }
        }

        ImGui::Separator();
        ImGui::TextColored(HIGHLIGHT_COLOR, "Instructions:");
        if (dbgInstrPCoutOfRange) {
            ImGui::SameLine();
            ImGui::TextColored(IMGUI_RED_COL, "PC set to RAM");
        }
        if (ImGui::BeginTable("instruction_debug", dbgInstrColNum, TABLE_FLAGS)) {

            for (int i = 0; i < dbgInstrColNum; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, DEBUG_INSTR_COLUMNS[i]);
            }

            if (gameState.game_running) {

                if (dbgInstrPCoutOfRange) {
                    bank_index& current_index = machineInfo.program_buffer_tmp.GetCurrentIndex();
                    while (machineInfo.program_buffer_tmp.GetNextEntry(dbgInstrCurrentEntry)) {
                        ImGui::TableNextColumn();

                        if (std::find(dbgInstrBreakpointsTmp.begin(), dbgInstrBreakpointsTmp.end(), dbgInstrInstructionIndex) != dbgInstrBreakpointsTmp.end()) {
                            ImGui::TextColored(IMGUI_RED_COL, ">>>");
                        }
                        ImGui::TableNextColumn();

                        ImGui::Selectable(dbgInstrCurrentEntry.first.c_str(), current_index == dbgInstrInstructionIndex, SEL_FLAGS);
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                            SetBreakPointTmp(current_index);
                        }
                        ImGui::TableNextColumn();

                        ImGui::TextUnformatted(dbgInstrCurrentEntry.second.c_str());
                        ImGui::TableNextRow();
                    }
                }
                else {
                    bank_index& current_index = machineInfo.program_buffer.GetCurrentIndex();
                    while (machineInfo.program_buffer.GetNextEntry(dbgInstrCurrentEntry)) {
                        ImGui::TableNextColumn();

                        if (std::find(dbgInstrBreakpoints.begin(), dbgInstrBreakpoints.end(), current_index) != dbgInstrBreakpoints.end()) {
                            ImGui::TextColored(IMGUI_RED_COL, ">>>");
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
            if (ImGui::InputInt("ROM Bank", &dbgInstrBank, 1, 100, INPUT_INT_FLAGS)) {
                if (dbgInstrPCoutOfRange) {
                    ActionBankSwitch(machineInfo.program_buffer_tmp, dbgInstrBank);
                }
                else {
                    ActionBankSwitch(machineInfo.program_buffer, dbgInstrBank);
                }
            }
            if (ImGui::InputInt("ROM Address", &dbgInstrAddress, 1, 100, INPUT_INT_HEX_FLAGS)) {
                if (dbgInstrPCoutOfRange) {
                    ActionSearchAddress(machineInfo.program_buffer_tmp, dbgInstrAddress);
                }
                else {
                    ActionSearchAddress(machineInfo.program_buffer, dbgInstrAddress);
                }
            }
        }
        else {
            ImGui::BeginDisabled();
            ImGui::InputInt("ROM Bank", &dbgInstrBank);
            ImGui::InputInt("ROM Address", &dbgInstrAddress);
            ImGui::EndDisabled();
        }

        ImGui::Separator();
        ImGui::TextColored(HIGHLIGHT_COLOR, "Registers:");
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
        ImGui::Separator();
        ImGui::TextColored(HIGHLIGHT_COLOR, "Flags and ISR:");
        if (!machineInfo.flag_values.empty()) {
            if (ImGui::BeginTable("flags_debug", dbgInstrColNumFlags, TABLE_FLAGS)) {
                for (int i = 0; i < dbgInstrColNumFlags; i++) {
                    ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, DEBUG_FLAG_COLUMNS[i]);
                }

                ImGui::TableNextColumn();
                for (int i = 0; const auto & n : machineInfo.flag_values) {
                    ImGui::TextUnformatted(n.first.c_str());
                    ImGui::TableNextColumn();

                    ImGui::TextUnformatted(n.second.c_str());

                    if (i++ % 2) { ImGui::TableNextRow(); }
                    ImGui::TableNextColumn();
                }
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::TextColored(HIGHLIGHT_COLOR, "Misc:");
        if (!machineInfo.misc_values.empty()) {
            if (ImGui::BeginTable("misc_debug", dbgInstrColNumFlags, TABLE_FLAGS)) {
                for (int i = 0; i < dbgInstrColNumFlags; i++) {
                    ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, DEBUG_FLAG_COLUMNS[i]);
                }

                ImGui::TableNextColumn();
                for (int i = 0; const auto & n : machineInfo.misc_values) {
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

            if (int i = 0; gameState.game_running) {
                for (auto& [name, tables] : machineInfo.memory_buffer) {
                    if (ImGui::BeginTabItem(name.c_str())) {
                        int tables_num = tables.size() - 1;
                        dbgMemCellAnyHovered = false;

                        if (tables_num > 0) {
                            if (ImGui::InputInt("Memory Bank", &dbgMemBankIndex[i], 1, 100, INPUT_INT_FLAGS)) {
                                if (dbgMemBankIndex[i] < 0) { dbgMemBankIndex[i] = 0; }
                                else if (dbgMemBankIndex[i] > tables_num) { dbgMemBankIndex[i] = tables_num; }
                            }
                        }
                        else {
                            ImGui::BeginDisabled();
                            ImGui::InputInt("Memory Bank", &dbgMemBankIndex[i], INPUT_INT_FLAGS);
                            ImGui::EndDisabled();
                        }

                        ScrollableTable<memory_data>& table = tables[dbgMemBankIndex[i]];
                        CheckScroll(table);

                        if (ImGui::BeginTable(name.c_str(), dbgMemColNum, TABLE_FLAGS)) {
                            ImGui::PushStyleColor(ImGuiCol_Text, HIGHLIGHT_COLOR);
                            for (int j = 0; j < dbgMemColNum; j++) {
                                ImGui::TableSetupColumn(DEBUG_MEM_COLUMNS[j].first.c_str(), TABLE_COLUMN_FLAGS, DEBUG_MEM_COLUMNS[j].second);
                            }
                            ImGui::TableHeadersRow();
                            ImGui::PopStyleColor();

                            int line = 0;
                            while (table.GetNextEntry(dbgMemCurrentEntry)) {
                                ImGui::TableNextColumn();

                                ImGui::TextColored(HIGHLIGHT_COLOR, get<MEM_ENTRY_ADDR>(dbgMemCurrentEntry).c_str());

                                
                                u8* ref = get<MEM_ENTRY_REF>(dbgMemCurrentEntry);
                                for (i = 0; i < get<MEM_ENTRY_LEN>(dbgMemCurrentEntry); i++) {
                                    ImGui::TableNextColumn();
                                    ImGui::Selectable(format("{:02x}", ref[i]).c_str(), dbgMemCellHovered ? i == dbgMemCursorPos.x || line == dbgMemCursorPos.y : false);
                                    if (ImGui::IsItemHovered()) {
                                        dbgMemCursorPos = Vec2(i, line);
                                        dbgMemCellAnyHovered |= true;
                                    }
                                }

                                ImGui::TableNextRow();
                                line++;
                            }
                            for (; line < DEBUG_MEM_LINES; line++) {
                                ImGui::TableNextColumn();
                                ImGui::TextColored(HIGHLIGHT_COLOR, "-");
                                ImGui::TableNextRow();
                            }

                            ImGui::EndTable();
                        }

                        dbgMemCellHovered = dbgMemCellAnyHovered;
                        ImGui::EndTabItem();
                    }
                    i++;
                }
            }
            else {
                if (ImGui::BeginTabItem("NONE")) {
                    ImGui::BeginDisabled();
                    ImGui::InputInt("Bank", &i);
                    ImGui::EndDisabled();

                    if (ImGui::BeginTable("no_memory", dbgMemColNum, TABLE_FLAGS)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, HIGHLIGHT_COLOR);
                        for (int j = 0; j < dbgMemColNum; j++) {
                            ImGui::TableSetupColumn(DEBUG_MEM_COLUMNS[j].first.c_str(), TABLE_COLUMN_FLAGS, DEBUG_MEM_COLUMNS[j].second);
                        }
                        ImGui::TableHeadersRow();
                        ImGui::PopStyleColor();

                        for (int lines = 0; lines < DEBUG_MEM_LINES; lines++) {
                            ImGui::TableNextColumn();
                            ImGui::TextColored(HIGHLIGHT_COLOR, "-");
                            ImGui::TableNextRow();
                        }

                        ImGui::EndTable();
                    }

                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}

void ImGuiGameboyX::ShowHardwareInfo() {
    ImGui::SetNextWindowSize(hw_info_win_size);

    if (ImGui::Begin("Hardware Info", &machineInfo.track_hardware_info, WIN_CHILD_FLAGS)) {
        if (ImGui::BeginTable("hardware_info", 2, TABLE_FLAGS_NO_BORDER_OUTER_H)) {

            for (int i = 0; i < hwInfoColNum; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, HW_INFO_COLUMNS[i]);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("CPU Clock");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted((to_string(machineInfo.current_frequency) + " MHz").c_str());

            int length = machineInfo.hardware_info.size();
            if (length > 0) { ImGui::TableNextRow(); }
            
            length--;
            for (int i = 0; const auto & [name, value] : machineInfo.hardware_info) {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(value.c_str());
                if (i != length) {
                    ImGui::TableNextRow();
                }
                i++;
            }

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

    auto filter_items = new nfdfilteritem_t[FILE_EXTS.size()];
    for (int i = 0; i < FILE_EXTS.size(); i++) {
        filter_items[i] = { FILE_EXTS[i][0].c_str(), FILE_EXTS[i][1].c_str() };
    }

    nfdchar_t* out_path = nullptr;
    const auto result = NFD_OpenDialog(&out_path, filter_items, FILE_EXTS.size(), s_path_rom_folder.c_str());
    delete[] filter_items;

    if (result == NFD_OKAY) {
        if (out_path != nullptr) {
            string path_to_rom(out_path);
            NFD_FreePath(out_path);
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

void ImGuiGameboyX::ShowGraphicsInfo() {
    if (ImGui::Begin("Graphics Info", &showGraphicsInfo)) {

        ImGui::End();
    }
}

void ImGuiGameboyX::ShowGraphicsOverlay() {
    ImVec2 window_pos = ImVec2((graphicsOverlayCorner & 1) ? GUI_IO.DisplaySize.x - OVERLAY_DISTANCE : OVERLAY_DISTANCE, (graphicsOverlayCorner & 2) ? GUI_IO.DisplaySize.y - OVERLAY_DISTANCE : OVERLAY_DISTANCE);
    ImVec2 window_pos_pivot = ImVec2((graphicsOverlayCorner & 1) ? 1.0f : 0.0f, (graphicsOverlayCorner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    ImGui::SetNextWindowBgAlpha(0.35f);

    float graphicsFPSmax = .0f;

    float fps = GUI_IO.Framerate;
    graphicsFPSavg += fps;
    graphicsFPSavg /= 2;
    graphicsFPScount++;

    graphicsFPSfifo.pop();
    graphicsFPSfifo.push(fps);
    
    // stablize output (roughly once per second)
    if (graphicsFPScount >= graphicsFPSavg) {
        graphicsFPScur = fps;
        graphicsFPScount = 0;
    }

    graphicsFPSfifoCopy = graphicsFPSfifo;
    for (int i = 0; i < FPS_SAMPLES_NUM; i++) {
        graphicsFPSsamples[i] = graphicsFPSfifoCopy.front();
        graphicsFPSfifoCopy.pop();

        if (graphicsFPSsamples[i] > graphicsFPSmax) { graphicsFPSmax = graphicsFPSsamples[i]; }
    }

    if (ImGui::Begin("graphics_overlay", &graphicsShowOverlay, WIN_OVERLAY_FLAGS))
    {
        if (ImGui::IsWindowHovered()) {
            if (ImGui::BeginTooltip()) {
                ImGui::Text("right-click to change position");
                ImGui::EndTooltip();
            }
        }

        ImGui::TextUnformatted(to_string(machineInfo.current_framerate).c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted("FPS (Emu)");

        ImGui::TextUnformatted(to_string(graphicsFPScur).c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted("FPS (App)");
        ImGui::PlotLines("", graphicsFPSsamples, IM_ARRAYSIZE(graphicsFPSsamples), 0, nullptr, .0f, graphicsFPSmax, ImVec2(0, 80.0f));
        
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Top-left", nullptr, graphicsOverlayCorner == 0)) graphicsOverlayCorner = 0;
            if (ImGui::MenuItem("Top-right", nullptr, graphicsOverlayCorner == 1)) graphicsOverlayCorner = 1;
            if (ImGui::MenuItem("Bottom-left", nullptr, graphicsOverlayCorner == 2)) graphicsOverlayCorner = 2;
            if (ImGui::MenuItem("Bottom-right", nullptr, graphicsOverlayCorner == 3)) graphicsOverlayCorner = 3;
            if (graphicsShowOverlay && ImGui::MenuItem("Close")) graphicsShowOverlay = false;
            ImGui::EndPopup();
        }

        ImGui::End();
    }
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
            LOG_WARN("[emu] Game already added ! Process aborted");
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
}

// ***** ACTIONS FOR DEBUG INSTRUCTION WINDOW *****

void ImGuiGameboyX::ActionBankSwitch(ScrollableTableBase& _table_obj, int& _bank) {
    _table_obj.SearchBank(_bank);
}

void ImGuiGameboyX::ActionSearchAddress(ScrollableTableBase& _table_obj, int& _address) {
    _table_obj.SearchAddress(_address);
}

void ImGuiGameboyX::ActionScrollToCurrentPC(ScrollableTableBase& _table_obj) {
    _table_obj.SearchBank(dbgInstrInstructionIndex.bank);
    _table_obj.SearchAddress(dbgInstrLastPC);
}

void ImGuiGameboyX::ActionSetEmulationSpeed(const int& _index) {
    emulationSpeedsEnabled[currentSpeedIndex].value = false;
    emulationSpeedsEnabled[_index].value = true;

    for (int i = 0; const auto & [key, value] : EMULATION_SPEEDS) {
        if (_index == i) {
            machineInfo.emulation_speed = key;
        }
        i++;
    }

    currentSpeedIndex = _index;
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
        if (machineInfo.current_rom_bank >= 0) {
            dbgInstrLastPC = machineInfo.current_pc;
            dbgInstrInstructionIndex.bank = machineInfo.current_rom_bank;
            dbgInstrPCoutOfRange = false;

            ActionScrollToCurrentPC(machineInfo.program_buffer);

            dbgInstrInstructionIndex.index = machineInfo.program_buffer.GetIndexByAddress(dbgInstrLastPC).index;
            return true;
        }
        else {
            dbgInstrLastPC = machineInfo.current_pc;
            dbgInstrInstructionIndex.bank = 0;
            dbgInstrPCoutOfRange = true;

            ActionScrollToCurrentPC(machineInfo.program_buffer_tmp);

            dbgInstrInstructionIndex.index = machineInfo.program_buffer_tmp.GetIndexByAddress(dbgInstrLastPC).index;

            return true;
        }
    }

    return false;
}

// ***** DEBUG INSTRUCTION PROCESS BREAKPOINT *****
void ImGuiGameboyX::SetBreakPoint(const bank_index& _current_index) {
    if (std::find(dbgInstrBreakpoints.begin(), dbgInstrBreakpoints.end(), _current_index) != dbgInstrBreakpoints.end()) {
        dbgInstrBreakpoints.remove(_current_index);
    } else {
        dbgInstrBreakpoints.emplace_back(_current_index);
    }
}

void ImGuiGameboyX::SetBreakPointTmp(const bank_index& _current_index) {
    if (std::find(dbgInstrBreakpointsTmp.begin(), dbgInstrBreakpointsTmp.end(), _current_index) != dbgInstrBreakpointsTmp.end()) {
        dbgInstrBreakpointsTmp.remove(_current_index);
    } else {
        dbgInstrBreakpointsTmp.emplace_back(_current_index);
    }
}

void ImGuiGameboyX::SetBankAndAddressScrollableTable(ScrollableTableBase& _tyble_obj, int& _bank, int& _address){
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
    dbgInstrLastPC = -1;
}

void ImGuiGameboyX::ResetMemInspector() {
    dbgMemBankIndex.clear();
    dbgMemCursorPos = Vec2(-1, -1);
    dbgMemCellHovered = false;
    dbgMemCellAnyHovered = false;
}

void ImGuiGameboyX::SetupMemInspectorIndex() {
    ResetMemInspector();

    for (const auto& n : machineInfo.memory_buffer) {
        dbgMemBankIndex.emplace_back(0);
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

    showGameSelect = true;
}

void ImGuiGameboyX::GameStarted() {
    ResetDebugInstr();
    SetupMemInspectorIndex();

    showGameSelect = false;
}

/* ***********************************************************************************************************
    IMGUIGAMEBOYX SDL FUNCTIONS
*********************************************************************************************************** */
void ImGuiGameboyX::EventKeyDown(const SDL_Keycode& _key) {
    switch (_key) {
    case SDLK_a:
        sdlkADown = true;
        break;
    case SDLK_F3:
        machineInfo.pause_execution = false;
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
    case SDLK_DOWN:
        for (int i = 0; i < gamesSelected.size(); i++) {
            gamesSelected[i] = false;
        }
        if (gameSelectedIndex < games.size() - 1) { gameSelectedIndex++; }
        gamesSelected[gameSelectedIndex] = true;
        break;
    case SDLK_UP:
        for (int i = 0; i < gamesSelected.size(); i++) {
            gamesSelected[i] = false;
        }
        if (gameSelectedIndex > 0) { gameSelectedIndex--; }
        gamesSelected[gameSelectedIndex] = true;
        break;
    case SDLK_RETURN:
        ActionStartGame(gameSelectedIndex);
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
    case SDLK_F1:
        ActionRequestReset();
        break;
    case SDLK_F3:
        machineInfo.pause_execution = true;
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
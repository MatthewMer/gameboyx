#include "GuiMgr.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "GameboyCartridge.h"
#include "imgui.h"
#include "data_io.h"
#include "nfd.h"
#include "logger.h"
#include "helper_functions.h"
#include "general_config.h"
#include <format>
#include <cmath>
#include <queue>

using namespace std;

/* ***********************************************************************************************************
    PROTOTYPES
*********************************************************************************************************** */
static void create_fs_hierarchy();

/* ***********************************************************************************************************
    IMGUIGAMEBOYX FUNCTIONS
*********************************************************************************************************** */
GuiMgr* GuiMgr::instance = nullptr;

GuiMgr* GuiMgr::getInstance() {
    if (instance == nullptr) {
        instance = new GuiMgr();
    }

    return instance;
}

void GuiMgr::resetInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

GuiMgr::GuiMgr() {
    graphics_settings graph_settings = {};
    HardwareMgr::GetGraphicsSettings(graph_settings);
    framerateTarget = graph_settings.framerateTarget;
    fpsUnlimited = graph_settings.fpsUnlimited;
    tripleBuffering = graph_settings.tripleBuffering;
    vsync = graph_settings.presentModeFifo;

    audio_settings aud_settings = {};
    HardwareMgr::GetAudioSettings(aud_settings);
    samplingRate = aud_settings.sampling_rate;
    samplingRateMax = aud_settings.sampling_rate_max;
    volume = aud_settings.master_volume;

    vhwmgr = VHardwareMgr::getInstance();

    // init explorer
    NFD_Init();

    // set fps data to default values
    graphicsFPSavg = GUI_IO.Framerate;
    graphicsFPScount = 1;
    for (int i = 0; i < FPS_SAMPLES_NUM; i++) {
        graphicsFPSfifo.push(.0f);
    }

    // set emulation speed to 1x
    ActionSetEmulationSpeed(0);

    // read games from config
    ReloadGamesGuiCtx();
}

GuiMgr::~GuiMgr() {
    if (gameRunning) {
        vhwmgr->ShutdownHardware();
    }
    VHardwareMgr::resetInstance();
}

/* ***********************************************************************************************************
    GUI PROCESS ENTRY POINT
*********************************************************************************************************** */
void GuiMgr::ProcessData() {
    // debug
    static bool debug_enabled = showInstrDebugger;
    if (debug_enabled != showInstrDebugger) {
        debug_enabled = showInstrDebugger;

        vhwmgr->SetDebugEnabled(debug_enabled);
    }

    // keys
    auto& inputs = HardwareMgr::GetKeys();

    while (!inputs.empty()) {
        pair<SDL_Keycode, SDL_EventType>& input = inputs.front();

        switch (input.second) {
        case SDL_KEYDOWN:
            EventKeyDown(input.first);
            break;
        case SDL_KEYUP:
            EventKeyUp(input.first);
            break;
        }

        inputs.pop();
    }

    windowActive = false;
    for (const auto& [key, value] : windowsActive) {
        if (value) {
            activeId = key;
            windowActive = true;
        }
    }

    windowHovered = false;
    for (const auto& [key, value] : windowsHovered) {
        if (value) {
            hoveredId = key;
            windowHovered = true;
        }
    }

    // mouse scroll - only used for tables (not optimal but works so far)
    const auto& scroll = HardwareMgr::GetScroll();
    EventMouseWheel(scroll);
}

void GuiMgr::ProcessGUI() {
    //IM_ASSERT(ImGui::GetCurrentContext() != nullptr && "Missing dear imgui context. Refer to examples app!");
    if (gameRunning) {
        if (showGraphicsOverlay || showHardwareInfo) { vhwmgr->GetFpsAndClock(virtualFramerate, virtualFrequency); }
        if (showHardwareInfo) { vhwmgr->GetHardwareInfo(hardwareInfo); }
        if (showInstrDebugger) {
            vhwmgr->GetInstrDebugFlags(regValues, flagValues, miscValues);
        }
    }

    if (showInstrDebugger) { ShowDebugInstructions(); }
    if (showWinAbout) { ShowWindowAbout(); }
    if (showHardwareInfo) { ShowHardwareInfo(); }
    if (showMemoryInspector) { ShowDebugMemoryInspector(); }
    if (showImGuiDebug) { ImGui::ShowDebugLogWindow(&showImGuiDebug); }
    if (showGraphicsInfo) { ShowGraphicsInfo(); }
    if (showGraphicsOverlay) { ShowGraphicsOverlay(); }
    if (!gameRunning) { ShowGameSelect(); }
    if (showNewGameDialog) { ShowNewGameDialog(); }
    if (showGraphicsSettings) { ShowGraphicsSettings(); }
    if (showAudioSettings) { ShowAudioSettings(); }
    if (showMainMenuBar) { ShowMainMenuBar(); }

    sdlScrollUp = false;
    sdlScrollDown = false;
}

/* ***********************************************************************************************************
    Show GUI Functions
*********************************************************************************************************** */
void GuiMgr::ShowMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("Games")) {
            if (gameRunning) {
                ImGui::MenuItem("Reset game", nullptr, &requestGameReset);
                if (requestGameReset) { ActionGameReset(); }
                ImGui::MenuItem("Stop game", nullptr, &requestGameStop);
                if (requestGameStop) { ActionGameStop(); }
            } else {
                ImGui::MenuItem("Add new game", nullptr, &showNewGameDialog);
                ImGui::MenuItem("Remove game(s)", nullptr, &requestDeleteGames);
                if (requestDeleteGames) { ActionDeleteGames(); }
                ImGui::MenuItem("Start game", nullptr, &requestGameStart);
                if (requestGameStart) { ActionGameStart(); }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings")) {
            // Emulation
            if (ImGui::BeginMenu("Emulation", &showEmulationMenu)) {
                ShowEmulationSpeeds();
                ImGui::EndMenu();
            }
            // Graphics
            if (ImGui::BeginMenu("Graphics", &showGraphicsMenu)) {
                ImGui::MenuItem("General", nullptr, &showGraphicsSettings);
                ImGui::MenuItem("Overlay", nullptr, &showGraphicsOverlay);
                ImGui::EndMenu();
            }
            // Audio
            if (ImGui::BeginMenu("Audio", &showAudioMenu)) {
                ImGui::MenuItem("General", nullptr, &showAudioSettings);
                ImGui::EndMenu();
            }
            // basic
            ImGui::MenuItem("Show menu bar", nullptr, &showMainMenuBar);
            if (!gameRunning && !showMainMenuBar) {
                showMainMenuBar = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Hardware Info", nullptr, &showHardwareInfo);
            ImGui::MenuItem("Instruction Debugger", nullptr, &showInstrDebugger);
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

void GuiMgr::ShowEmulationSpeeds() {
    if (ImGui::BeginMenu("Speed", &showEmulationSpeed)) {
        for (int i = 0; const auto & [key, value] : EMULATION_SPEEDS) {
            bool& speed = emulationSpeedsEnabled[i].value;
            ImGui::MenuItem(value.c_str(), nullptr, &speed);

            if (speed && currentSpeedIndex != i) {
                ActionSetEmulationSpeed(i);
            }
            i++;
        }
        ImGui::EndMenu();
    }
}

void GuiMgr::ShowWindowAbout() {
    if (ImGui::Begin("About", &showWinAbout, WIN_CHILD_FLAGS)) {
        CheckWindow(ABOUT);
        ImGui::Text("GameboyX Emulator");
        ImGui::Text("Version: %s %d.%d", GBX_RELEASE, GBX_VERSION_MAJOR, GBX_VERSION_MINOR, GBX_VERSION_PATCH);
        ImGui::Text("Author: %s", GBX_AUTHOR);
        ImGui::Spacing();
        ImGui::End();
    }
}

void GuiMgr::ShowDebugInstructions() {
    ImGui::SetNextWindowSize(debug_instr_win_size);

    if (ImGui::Begin("Instruction Debugger", &showInstrDebugger, WIN_CHILD_FLAGS)) {
        CheckWindow(DEBUG_INSTR);
        ShowDebugInstrButtonsHead();
        ShowDebugInstrTable();
        ShowDebugInstrSelect();
        ShowDebugInstrMiscData("Registers", debugInstrRegColNum, DEBUG_REGISTER_COLUMNS, regValues);
        ShowDebugInstrMiscData("Flags and ISR", debugInstrFlagColNum, DEBUG_FLAG_COLUMNS, flagValues);
        ShowDebugInstrMiscData("Misc data", debugInstrFlagColNum, DEBUG_FLAG_COLUMNS, miscValues);
        ImGui::End();
    }
}

void GuiMgr::ShowDebugInstrButtonsHead() {
    if (gameRunning) {
        if (ImGui::Button("Step")) { ActionContinueExecution(); }
        ImGui::SameLine();
        if (ImGui::Button("Jump to PC")) { ActionSetToCurrentPC(); }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) { ActionGameReset(); }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("Step");
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Button("Jump to PC");
        ImGui::SameLine();
        ImGui::Button("Reset");
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto run", &autoRun);
    static bool auto_run = autoRun;
    if (auto_run != autoRun) {
        auto_run = autoRun;
        autoRunInstructions.store(auto_run);
    }
}

void GuiMgr::ShowDebugInstrTable() {
    ImGui::Separator();
    ImGui::TextColored(HIGHLIGHT_COLOR, "Instructions:");
    if (pcSetToRam) {
        ImGui::SameLine();
        ImGui::TextColored(IMGUI_RED_COL, "PC set to RAM");
    }
    if (ImGui::BeginTable("instruction_debug", debugInstrColNum, TABLE_FLAGS)) {
        for (int i = 0; i < debugInstrColNum; i++) {
            ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, DEBUG_INSTR_COLUMNS[i]);
        }

        if (gameRunning) {
            instr_entry cur_entry;
            
            unique_lock<mutex> lock_debug_instr(mutDebugInstr);
            bool pc_set_to_ram = pcSetToRam.load();

            auto& table = (pc_set_to_ram ? debugInstrTableTmp : debugInstrTable);

            CheckScroll(DEBUG_INSTR, table);

            if (debugInstrAutoscroll.load()) {
                int current_pc = currentPc.load();
                int current_bank = currentBank.load();

                if (current_bank != lastBank) {
                    lastBank = current_bank;
                    table.SearchBank(lastBank);
                    lastPc = current_pc;
                    table.SearchAddress(lastPc);
                } else if (current_pc != lastPc) {
                    lastPc = currentPc;
                    table.SearchAddress(lastPc);
                }

                debugInstrCurrentInstrIndex = table.GetIndexByAddress(lastPc);
            }
            
            GetBankAndAddressTable(table, bankSelect, addrSelect);

            auto& tabel_breakpts = (pc_set_to_ram ? breakpointsTableTmp : breakpointsTable);
            auto& breakpts = (pc_set_to_ram ? breakpointsAddrTmp : breakpointsAddr);

            while (table.GetNextEntry(cur_entry)) {
                bank_index& current_index = table.GetCurrentIndex();

                ImGui::TableNextColumn();

                if (std::find(tabel_breakpts.begin(), tabel_breakpts.end(), current_index) != tabel_breakpts.end()) {
                    ImGui::TextColored(IMGUI_RED_COL, ">>>");
                }
                ImGui::TableNextColumn();

                ImGui::Selectable(cur_entry.first.c_str(), current_index == debugInstrCurrentInstrIndex, SEL_FLAGS);
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                    ActionSetBreakPoint(tabel_breakpts, current_index, breakpts, table);
                }
                ImGui::TableNextColumn();

                ImGui::TextUnformatted(cur_entry.second.c_str());
                ImGui::TableNextRow();
            }
        } else {
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
}

void GuiMgr::ShowDebugInstrSelect() {
    if (gameRunning) {
        auto& table = (pcSetToRam ? debugInstrTableTmp : debugInstrTable);

        if (ImGui::InputInt("Memory bank", &bankSelect, 1, 100, INPUT_INT_FLAGS)) {
            ActionSetToBank(table, bankSelect);
        }
        if (ImGui::InputInt("Memory address", &addrSelect, 0, 100, INPUT_INT_HEX_FLAGS)) {
            ActionSetToAddress(table, addrSelect);
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::InputInt("Memory bank", &bankSelect);
        ImGui::InputInt("Memory address", &addrSelect, 0);
        ImGui::EndDisabled();
    }
}

void GuiMgr::ShowDebugInstrMiscData(const char* _title, const int& _col_num, const std::vector<float>& _columns, const std::vector<reg_entry>& _data) {
    ImGui::Separator();
    ImGui::TextColored(HIGHLIGHT_COLOR, _title);
    if (!_data.empty()) {
        if (ImGui::BeginTable(_title, _col_num, TABLE_FLAGS)) {
            for (int i = 0; i < _col_num; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, _columns[i]);
            }

            ImGui::TableNextColumn();
            for (int i = 0; const auto & n : _data) {
                ImGui::TextUnformatted(n.first.c_str());
                ImGui::TableNextColumn();

                ImGui::TextUnformatted(n.second.c_str());

                if (i++ % 2) { ImGui::TableNextRow(); }
                ImGui::TableNextColumn();
            }
            ImGui::EndTable();
        }
    }
}


void GuiMgr::ShowDebugMemoryInspector() {
    ImGui::SetNextWindowSize(debug_mem_win_size);

    if (ImGui::Begin("Memory Inspector", &showMemoryInspector, WIN_CHILD_FLAGS)) {
        CheckWindow(DEBUG_MEM);
        if (ImGui::BeginTabBar("memory_inspector_tabs", 0)) {

            if (gameRunning) {
                for (int i = 0; auto & table : debugMemoryTables) {
                    ShowDebugMemoryTab(table);
                    i++;
                }
            } else {
                if (ImGui::BeginTabItem("NONE")) {
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

void GuiMgr::ShowDebugMemoryTab(Table<memory_entry>& _table) {
    string table_name = _table.name;

    if (ImGui::BeginTabItem(table_name.c_str())) {
        CheckScroll(DEBUG_MEM, _table);

        int bank;
        int address;
        GetBankAndAddressTable(_table, bank, address);

        if (ImGui::InputInt("Memory bank", &bank, 1, 100, INPUT_INT_FLAGS)) {
            ActionSetToBank(_table, bank);
        }
        if (ImGui::InputInt("Memory address", &address, 0, 100, INPUT_INT_HEX_FLAGS)) {
            ActionSetToAddress(_table, address);
        }

        int tables_num = (int)_table.size - 1;
        dbgMemCellAnyHovered = false;

        if (ImGui::BeginTable(table_name.c_str(), dbgMemColNum, TABLE_FLAGS)) {
            ImGui::PushStyleColor(ImGuiCol_Text, HIGHLIGHT_COLOR);
            for (int j = 0; j < dbgMemColNum; j++) {
                ImGui::TableSetupColumn(DEBUG_MEM_COLUMNS[j].first.c_str(), TABLE_COLUMN_FLAGS, DEBUG_MEM_COLUMNS[j].second);
            }
            ImGui::TableHeadersRow();
            ImGui::PopStyleColor();

            int line = 0;

            memory_entry current_entry;
            while (_table.GetNextEntry(current_entry)) {
                ImGui::TableNextColumn();

                ImGui::TextColored(HIGHLIGHT_COLOR, get<MEM_ENTRY_ADDR>(current_entry).c_str());

                // pointer to 16 bytes of table entry
                u8* data = get<MEM_ENTRY_REF>(current_entry);
                for (int i = 0; i < get<MEM_ENTRY_LEN>(current_entry); i++) {
                    ImGui::TableNextColumn();
                    ImGui::Selectable(format("{:02x}", data[i]).c_str(), dbgMemCellHovered ? i == dbgMemCursorPos.x || line == dbgMemCursorPos.y : false);
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
}

void GuiMgr::ShowHardwareInfo() {
    ImGui::SetNextWindowSize(hw_info_win_size);

    if (ImGui::Begin("Hardware Info", &showHardwareInfo, WIN_CHILD_FLAGS)) {
        CheckWindow(HW_INFO);
        if (ImGui::BeginTable("hardware_info", 2, TABLE_FLAGS_NO_BORDER_OUTER_H)) {

            for (int i = 0; i < hwInfoColNum; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER, HW_INFO_COLUMNS[i]);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("CPU Clock");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted((to_string(virtualFrequency) + " MHz").c_str());

            int length = (int)hardwareInfo.size();
            if (length > 0) { ImGui::TableNextRow(); }

            length--;
            for (int i = 0; const auto & [name, value] : hardwareInfo) {
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

void GuiMgr::ShowNewGameDialog() {
    check_and_create_config_folders();

    string current_path = get_current_path();
    string s_path_rom_folder = ROM_FOLDER;

    auto filter_items = std::vector<nfdfilteritem_t>();
    for (const auto& [key, value] : FILE_EXTS) {
        filter_items.emplace_back(value.first.c_str(), value.second.c_str());
    }

    nfdchar_t* out_path = nullptr;
    const auto result = NFD_OpenDialog(&out_path, filter_items.data(), (nfdfiltersize_t)FILE_EXTS.size(), s_path_rom_folder.c_str());

    if (result == NFD_OKAY) {
        if (out_path != nullptr) {
            string path_to_rom(out_path);
            NFD_FreePath(out_path);
            if (!ActionAddGame(path_to_rom)) { return; }
        } else {
            LOG_ERROR("No path (nullptr)");
            return;
        }
    } else if (result != NFD_CANCEL) {
        LOG_ERROR("Couldn't open file dialog");
    }

    showNewGameDialog = false;
}

void GuiMgr::ShowGameSelect() {
    ImGui::SetNextWindowPos(MAIN_VIEWPORT->WorkPos);
    ImGui::SetNextWindowSize(MAIN_VIEWPORT->Size);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IMGUI_BG_COLOR);
    if (ImGui::Begin("games_select", nullptr, MAIN_WIN_FLAGS)) {
        CheckWindow(GAME_SELECT);
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

                ImGui::Selectable(FILE_EXTS.at(game->console).first.c_str(), gamesSelected[i].value, SEL_FLAGS);
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                    ActionGamesSelect(i);
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(game->title.c_str());
                ImGui::SetItemTooltip(game->title.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(game->filePath.c_str());
                ImGui::SetItemTooltip(game->filePath.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(game->fileName.c_str());
                ImGui::SetItemTooltip(game->fileName.c_str());

                i++;
            }
            ImGui::PopStyleVar();
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::End();
    }
    ImGui::PopStyleColor();
}

void GuiMgr::ShowGraphicsInfo() {
    if (ImGui::Begin("Graphics Info", &showGraphicsInfo)) {

        ImGui::End();
    }
}

void GuiMgr::ShowGraphicsOverlay() {
    ImVec2 window_pos = ImVec2((graphicsOverlayCorner & 1) ? GUI_IO.DisplaySize.x - OVERLAY_DISTANCE : OVERLAY_DISTANCE, (graphicsOverlayCorner & 2) ? GUI_IO.DisplaySize.y - OVERLAY_DISTANCE : OVERLAY_DISTANCE);
    ImVec2 window_pos_pivot = ImVec2((graphicsOverlayCorner & 1) ? 1.0f : 0.0f, (graphicsOverlayCorner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    ImGui::SetNextWindowBgAlpha(0.35f);

    float graphicsFPSmax = .0f;

    float fps = GUI_IO.Framerate;
    graphicsFPSavg += fps;
    graphicsFPSavg /= 2;
    graphicsFPScount++;

    // stablize output (roughly once per second)
    if (graphicsFPScount >= graphicsFPSavg) {
        graphicsFPSfifo.pop();
        graphicsFPSfifo.push(graphicsFPSavg);

        graphicsFPSavg = 0;
        graphicsFPScur = fps;
        graphicsFPScount = 0;
    }

    graphicsFPSfifoCopy = graphicsFPSfifo;
    for (int i = 0; i < FPS_SAMPLES_NUM; i++) {
        graphicsFPSsamples[i] = graphicsFPSfifoCopy.front();
        graphicsFPSfifoCopy.pop();

        if (graphicsFPSsamples[i] > graphicsFPSmax) { graphicsFPSmax = graphicsFPSsamples[i]; }
    }

    if (ImGui::Begin("graphics_overlay", &showGraphicsOverlay, WIN_OVERLAY_FLAGS)) {
        if (ImGui::IsWindowHovered()) {
            if (ImGui::BeginTooltip()) {
                ImGui::Text("right-click to change position");
                ImGui::EndTooltip();
            }
        }

        ImGui::TextUnformatted(to_string(virtualFramerate).c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted("FPS (Emu)");

        ImGui::TextUnformatted(to_string(graphicsFPScur).c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted("FPS (App)");
        ImGui::PlotLines("", graphicsFPSsamples, IM_ARRAYSIZE(graphicsFPSsamples), 0, nullptr, .0f, graphicsFPSmax, ImVec2(0, 80.0f));

        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Top-left", nullptr, graphicsOverlayCorner == 0)) graphicsOverlayCorner = 0;
            if (ImGui::MenuItem("Top-right", nullptr, graphicsOverlayCorner == 1)) graphicsOverlayCorner = 1;
            if (ImGui::MenuItem("Bottom-left", nullptr, graphicsOverlayCorner == 2)) graphicsOverlayCorner = 2;
            if (ImGui::MenuItem("Bottom-right", nullptr, graphicsOverlayCorner == 3)) graphicsOverlayCorner = 3;
            if (showGraphicsOverlay && ImGui::MenuItem("Close")) showGraphicsOverlay = false;
            ImGui::EndPopup();
        }

        ImGui::End();
    }
}

void GuiMgr::ShowGraphicsSettings() {
    static bool was_unlimited = fpsUnlimited;
    static bool was_triple_buffering = tripleBuffering;
    static bool was_vsync = vsync;

    ImGui::SetNextWindowSize(graph_settings_win_size);

    if (ImGui::Begin("Graphics Settings", &showGraphicsSettings, WIN_CHILD_FLAGS)) {
        ImGui::TextColored(HIGHLIGHT_COLOR, "Application");
        if (ImGui::BeginTable("graphics app", 2, TABLE_FLAGS_NO_BORDER_OUTER_H)) {

            for (int i = 0; i < 2; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

            // application ******************************************
            ImGui::TableNextColumn();
            if (fpsUnlimited) { ImGui::BeginDisabled(); }
            ImGui::TextUnformatted("Framerate target");
            if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                    ImGui::Text("sets base application framerate target");
                    ImGui::EndTooltip();
                }
            }
            ImGui::TableNextColumn();

            if (ImGui::SliderInt("##fps", &framerateTarget, APP_MIN_FRAMERATE, APP_MAX_FRAMERATE)) {
                ActionSetFramerateTarget();
            }
            if (fpsUnlimited) { ImGui::EndDisabled(); }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Unlimited");
            if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                    ImGui::Text("disables framerate target");
                    ImGui::EndTooltip();
                }
            }
            ImGui::TableNextColumn();
            ImGui::Checkbox("##unlimited", &fpsUnlimited);
            if (was_unlimited != fpsUnlimited) {
                ActionSetFramerateTarget();
                was_unlimited = fpsUnlimited;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("VSync");
            if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                    ImGui::Text("enables vertical synchronization");
                    ImGui::EndTooltip();
                }
            }
            ImGui::TableNextColumn();
            ImGui::Checkbox("##vsync", &vsync);
            if (was_vsync != vsync) {
                ActionSetSwapchainSettings();
                was_triple_buffering = tripleBuffering;
                was_vsync = vsync;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (vsync) { ImGui::BeginDisabled(); }
            ImGui::TextUnformatted("Triple buffering");
            if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                    ImGui::Text("enables triple buffering for physical hardware");
                    ImGui::EndTooltip();
                }
            }
            ImGui::TableNextColumn();
            ImGui::Checkbox("##triple_buffering", &tripleBuffering);
            if (was_triple_buffering != tripleBuffering) {
                ActionSetSwapchainSettings();
                was_triple_buffering = tripleBuffering;
                was_vsync = vsync;
            }
            if (vsync) { ImGui::EndDisabled(); }

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }

        // emulation ******************************************
        ImGui::Separator();
        ImGui::TextColored(HIGHLIGHT_COLOR, "Emulation");
        if (ImGui::BeginTable("graphics emu", 2, TABLE_FLAGS_NO_BORDER_OUTER_H)) {

            for (int i = 0; i < 2; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Triple buffering");
            if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                    ImGui::Text("enables triple buffering for virtual hardware (requires game restart)");
                    ImGui::EndTooltip();
                }
            }
            ImGui::TableNextColumn();
            ImGui::Checkbox("##triple_buffering_emu", &tripleBufferingEmu);

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }
        ImGui::End();
    }
}

void GuiMgr::ShowAudioSettings() {
    ImGui::SetNextWindowSize(graph_settings_win_size);

    if (ImGui::Begin("Audio Settings", &showAudioSettings, WIN_CHILD_FLAGS)) {

        if (ImGui::BeginTable("audio general", 2, TABLE_FLAGS_NO_BORDER_OUTER_H)) {

            for (int i = 0; i < 2; i++) {
                ImGui::TableSetupColumn("", TABLE_COLUMN_FLAGS_NO_HEADER);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

            // application ******************************************
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Master volume");
            if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                    ImGui::Text("sets overall volume");
                    ImGui::EndTooltip();
                }
            }
            ImGui::TableNextColumn();

            if (ImGui::SliderFloat("##volume", &volume, APP_MIN_VOLUME, APP_MAX_VOLUME)) {
                ActionSetMasterVolume();
            }
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Sampling rate");
            if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                    ImGui::Text("sets the sampling rate used by the audio device");
                    ImGui::EndTooltip();
                }
            }
            ImGui::TableNextColumn();

            static const char* current_item = nullptr;
            if (current_item == nullptr) {
                for (const auto& [key, val] : SAMPLING_RATES) {
                    if (samplingRate == val.first) {
                        current_item = key;
                    }
                }
            }

            if (ImGui::BeginCombo("##samplingrate", current_item)) {
                for (const auto& [key, val] : SAMPLING_RATES) {
                    if (val.first <= samplingRateMax) {
                        const char* item = key;
                        bool selected = current_item == item;

                        if (ImGui::Selectable(item, selected)) {
                            current_item = item;
                            samplingRate = val.first;
                            ActionSetSamplingRate();
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }
        ImGui::End();
    }
}

/* ***********************************************************************************************************
    ACTIONS TO READ/WRITE/PROCESS DATA ETC.
*********************************************************************************************************** */
void GuiMgr::ActionGameStart() {
    if (!gameRunning) {
        StartGame(false);
        showMainMenuBar = false;
    }
    requestGameStart = false;
}

void GuiMgr::ActionGameStop() {
    if (gameRunning) {
        vhwmgr->ShutdownHardware();
        gameRunning = false;
        showMainMenuBar = true;
        ResetGUI();
    }
    requestGameStop = false;
}

void GuiMgr::ActionGameReset() {
    if (gameRunning) {
        StartGame(true);
    }
    requestGameReset = false;
}

void GuiMgr::ActionContinueExecution() {
    if (gameRunning) {
        nextInstruction.store(true);
        autoRun = false;
        autoRunInstructions.store(false);
    }
}

void GuiMgr::ActionAutoExecution() {
    if (gameRunning) {
        unique_lock<mutex> lock_debug_breakpoints(mutDebugBreakpoints);
        auto& current_table_breakpoints = pcSetToRam.load() ? breakpointsTableTmp : breakpointsTable;

        if (std::find(current_table_breakpoints.begin(), current_table_breakpoints.end(), debugInstrCurrentInstrIndex) != current_table_breakpoints.end()) {
            autoRun = true;
            nextInstruction.store(true);
        } else {
            autoRun = !autoRun;
        }

        autoRunInstructions.store(autoRun);
    }
}

void GuiMgr::ActionGamesSelect(const int& _index) {
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        for (int j = 0; j < gamesSelected.size(); j++) {
            gamesSelected[_index].value = _index == j;
            if (_index == j) { gameSelectedIndex = _index; }
            ActionGameStart();
        }
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        if (sdlkShiftDown) {
            static int lower, upper;
            if (_index <= gameSelectedIndex) {
                lower = _index;
                upper = gameSelectedIndex;
            } else {
                lower = gameSelectedIndex;
                upper = _index;
            }

            for (int j = 0; j < gamesSelected.size(); j++) {
                gamesSelected[j].value = ((j >= lower) && (j <= upper)) || (sdlkCtrlDown ? gamesSelected[j].value : false);
            }
        } else {
            for (int j = 0; j < gamesSelected.size(); j++) {
                gamesSelected[j].value = (_index == j ? !gamesSelected[j].value : (sdlkCtrlDown ? gamesSelected[j].value : false));
            }
            gameSelectedIndex = _index;
            if (!sdlkCtrlDown) {
                gamesSelected[_index].value = true;
            }
        }
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) {
        // TODO: context menu
    }
}

void GuiMgr::ActionGamesSelectAll() {
    for (auto& n : gamesSelected) {
        n.value = true;
    }
}

void GuiMgr::ActionSetToBank(TableBase& _table_obj, int& _bank) {
    _table_obj.SearchBank(_bank);
}

void GuiMgr::ActionSetToAddress(TableBase& _table_obj, int& _address) {
    _table_obj.SearchAddress(_address);
}

void GuiMgr::ActionSetToCurrentPC() {
    if (gameRunning) {
        auto& table = (pcSetToRam ? debugInstrTableTmp : debugInstrTable);

        table.SearchBank(debugInstrCurrentInstrIndex.bank);
        table.SearchAddress(lastPc);
    }
}

void GuiMgr::ActionSetEmulationSpeed(const int& _index) {
    emulationSpeedsEnabled[currentSpeedIndex].value = false;
    emulationSpeedsEnabled[_index].value = true;

    for (int i = 0; const auto & [key, value] : EMULATION_SPEEDS) {
        if (_index == i) {
            currentSpeed = key;
            vhwmgr->SetEmulationSpeed(currentSpeed);
        }
        i++;
    }

    currentSpeedIndex = _index;
}

bool GuiMgr::ActionAddGame(const string& _path_to_rom) {
    auto cartridge = BaseCartridge::new_game(_path_to_rom);
    if (cartridge == nullptr) {
        return false;
    }

    if (cartridge->console == CONSOLE_NONE) {
        LOG_ERROR("[emu] Console type not recognized");
        delete cartridge;
        showNewGameDialog = false;
        return false;
    }

    if (!cartridge->ReadRom()) {
        showNewGameDialog = false;
        LOG_ERROR("[emu] error while reading rom ", _path_to_rom);
        delete cartridge;
        return false;
    }

    cartridge->CopyToRomFolder();
    cartridge->ClearRom();

    if (!write_games_to_config({ cartridge }, false)) {
        showNewGameDialog = false;
        LOG_ERROR("[emu] couldn't write new game to config");
        delete cartridge;
        return false;
    }

    AddGameGuiCtx(cartridge);
    return true;
}

void GuiMgr::ActionDeleteGames() {
    auto games_to_delete = vector<BaseCartridge*>();
    for (int i = 0; const auto & n : gamesSelected) {
        if (n.value) games_to_delete.push_back(games[i]);
        i++;
    }

    if (games_to_delete.size() > 0) {
        delete_games_from_config(games_to_delete);
        ReloadGamesGuiCtx();
    }

    requestDeleteGames = false;
}

void GuiMgr::ActionGameSelectUp() {
    if (gameSelectedIndex > 0) { --gameSelectedIndex; }
    for (auto& n : gamesSelected) { n.value = false; }
    gamesSelected[gameSelectedIndex].value = true;
}

void GuiMgr::ActionGameSelectDown() {
    if (gameSelectedIndex < games.size() - 1) { gameSelectedIndex++; }
    for (auto& n : gamesSelected) { n.value = false; }
    gamesSelected[gameSelectedIndex].value = true;
}

void GuiMgr::ActionToggleMainMenuBar() {
    if (gameRunning) {
        showMainMenuBar = !showMainMenuBar;
    }
}

void GuiMgr::ActionSetBreakPoint(vector<bank_index>& _table_breakpoints, const bank_index& _current_index, vector<pair<int, int>>& _breakpoints, TableBase& _table_obj) {
    unique_lock<mutex> lock_debug_breakpoints(mutDebugBreakpoints);

    bool found = false;
    int i = 0;

    for (auto& it : _table_breakpoints) {
        if (it == _current_index) {
            found = true;
            break;
        }
        i++;
    }

    if (found) {
        _table_breakpoints.erase(_table_breakpoints.begin() + i);
        _breakpoints.erase(_breakpoints.begin() + i);
    } else {
        int bank = _current_index.bank;
        int addr = _table_obj.GetAddressByIndex(_current_index);

        _table_breakpoints.emplace_back(_current_index);
        _breakpoints.emplace_back(pair(bank, addr));
    }
}

void GuiMgr::ActionSetFramerateTarget() {
    HardwareMgr::SetFramerateTarget(framerateTarget, fpsUnlimited);
}

void GuiMgr::ActionSetSwapchainSettings() {
    HardwareMgr::SetSwapchainSettings(vsync, tripleBuffering);
}

void GuiMgr::ActionSetMasterVolume() {
    HardwareMgr::SetMasterVolume(volume);
}

void GuiMgr::ActionSetSamplingRate() {
    HardwareMgr::SetSamplingRate(samplingRate);
}



void GuiMgr::StartGame(const bool& _restart) {
    if (games.size() > 0) {
        // gather settings
        virtual_graphics_settings graphics_settings = {};
        graphics_settings.buffering = tripleBufferingEmu ? V_TRIPPLE_BUFFERING : V_DOUBLE_BUFFERING;

        emulation_settings emu_settings = {};
        emu_settings.debug_enabled = showInstrDebugger;
        emu_settings.emulation_speed = currentSpeed;

        if (vhwmgr->InitHardware(games[gameSelectedIndex], graphics_settings, emu_settings, _restart, this, &GuiMgr::DebugCallback) != 0x00) {
            gameRunning = false;
        } else {
            vhwmgr->GetInstrDebugTable(debugInstrTable);
            vhwmgr->GetMemoryDebugTables(debugMemoryTables);

            if (vhwmgr->StartHardware() == 0x00) {
                gameRunning = true;
            } else {
                gameRunning = false;
            }
        }
    }
}

void GuiMgr::GetBankAndAddressTable(TableBase& _table_obj, int& _bank, int& _address) {
    bank_index centre = _table_obj.GetCurrentIndexCentre();
    _bank = centre.bank;
    _address = _table_obj.GetAddressByIndex(centre);
}

void GuiMgr::CheckWindow(const windowID& _id) {
    windowsActive.at(_id) = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    windowsHovered.at(_id) = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
}

// needs revision, but works so far
template <class T>
void GuiMgr::CheckScroll(const windowID& _id, Table<T>& _table) {
    if (sdlScrollUp || sdlScrollDown) {
        bool scroll = false;

        if (windowHovered) {
            scroll = _id == hoveredId;
        } else if (windowActive) {
            scroll = _id == activeId;
        }

        if (scroll) {
            if (sdlScrollUp) {
                sdlScrollUp = false;
                if (sdlkShiftDown) { _table.ScrollUpPage(); } else { _table.ScrollUp(1); }
            } else if (sdlScrollDown) {
                sdlScrollDown = false;
                if (sdlkShiftDown) { _table.ScrollDownPage(); } else { _table.ScrollDown(1); }
            }
        }
    }
}

void GuiMgr::AddGameGuiCtx(BaseCartridge* _game_ctx) {
    games.emplace_back(_game_ctx);
    gamesSelected.clear();
    for (const auto& game : games) {
        gamesSelected.push_back({ false });
    }
    gamesSelected.back().value = true;
    gameSelectedIndex = (int)gamesSelected.size() - 1;
}

void GuiMgr::ReloadGamesGuiCtx() {
    for (int i = (int)games.size() - 1; i > -1; i--) {
        delete games[i];
        games.erase(games.begin() + i);
    }

    read_games_from_config(games);

    gamesSelected.clear();
    for (const auto& n : games) {
        gamesSelected.push_back({ false });
    }
    if (gamesSelected.size() > 0) {
        gamesSelected[0].value = true;
    }

    gameSelectedIndex = 0;
}

void GuiMgr::ResetGUI() {
    bankSelect = 0;
    addrSelect = 0;
    breakpointsTable = std::vector<bank_index>();
    breakpointsTableTmp = std::vector<bank_index>();
    breakpointsAddr = std::vector<std::pair<int, int>>();
    breakpointsAddrTmp = std::vector<std::pair<int, int>>();
    lastPc = -1;
    lastBank = -1;
    currentPc.store(0);
    currentBank.store(0);
    debugInstrCurrentInstrIndex = bank_index(0, 0);
    debugInstrTable = Table<instr_entry>(DEBUG_INSTR_LINES);
    debugInstrTableTmp = Table<instr_entry>(DEBUG_INSTR_LINES);
    regValues = std::vector<reg_entry>();
    flagValues = std::vector<reg_entry>();
    miscValues = std::vector<reg_entry>();
    debugMemoryTables = std::vector<Table<memory_entry>>();
    virtualFramerate = 0;
    hardwareInfo = std::vector<data_entry>();
}

/* ***********************************************************************************************************
    IMGUIGAMEBOYX SDL FUNCTIONS
*********************************************************************************************************** */
void GuiMgr::EventKeyDown(SDL_Keycode& _key) {
    if (gameRunning) {
        switch (_key) {
        case SDLK_F3:
            ActionContinueExecution();
            break;
        case SDLK_LSHIFT:
            sdlkShiftDown = true;
            break;
        default:
            vhwmgr->EventKeyDown(_key);
            break;
        }
    } else {
        switch (_key) {
        case SDLK_a:
            sdlkADown = true;
            if (sdlkCtrlDown) { ActionGamesSelectAll(); }
            break;
        case SDLK_LSHIFT:
            sdlkShiftDown = true;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            sdlkCtrlDown = true;
            break;
        case SDLK_UP:
            ActionGameSelectUp();
            break;
        case SDLK_DOWN:
            ActionGameSelectDown();
            break;
        default:
            break;
        }
    }
}

void GuiMgr::EventKeyUp(SDL_Keycode& _key) {
    if (gameRunning) {
        switch (_key) {
        case SDLK_F10:
            ActionToggleMainMenuBar();
            break;
        case SDLK_LSHIFT:
            sdlkShiftDown = false;
            break;
        case SDLK_F9:
            ActionAutoExecution();
            break;
        case SDLK_F1:
            ActionGameReset();
            break;
        case SDLK_F11:
            HardwareMgr::ToggleFullscreen();
            break;
        case SDLK_ESCAPE:
            ActionGameStop();
            break;
        default:
            vhwmgr->EventKeyUp(_key);
            break;
        }
    } else {
        switch (_key) {
        case SDLK_a:
            sdlkADown = false;
            break;
        case SDLK_F10:
            ActionToggleMainMenuBar();
            break;
        case SDLK_LSHIFT:
            sdlkShiftDown = false;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            sdlkCtrlDown = false;
            break;
        case SDLK_DELETE:
            ActionDeleteGames();
            break;
        case SDLK_RETURN:
            ActionGameStart();
            break;
        case SDLK_F11:
            HardwareMgr::ToggleFullscreen();
            break;
        default:
            break;
        }
    }
}

void GuiMgr::EventMouseWheel(const Sint32& _wheel_y) {
    if (_wheel_y > 0) {
        sdlScrollUp = true;
        sdlScrollDown = false;
    } else if (_wheel_y < 0) {
        sdlScrollDown = true;
        sdlScrollUp = false;
    }
}

/* ***********************************************************************************************************
    CALLBACKS
*********************************************************************************************************** */
void GuiMgr::DebugCallback(const int& _pc, const int& _bank) {
    unique_lock<mutex> lock_debug_instr(mutDebugInstr);

    int current_bank = currentBank.load();
    int current_pc = currentPc.load();

    if (_pc != current_pc || _bank != current_bank) {
        bool pc_set_to_ram = pcSetToRam.load();

        if (pc_set_to_ram != (_bank < 0)) {
            pc_set_to_ram = !pc_set_to_ram;

            if (pc_set_to_ram) {
                vhwmgr->GetInstrDebugTableTmp(debugInstrTableTmp);
            }
        }

        currentPc.store(_pc);
        currentBank.store(_bank);
        pcSetToRam.store(pc_set_to_ram);

        debugInstrAutoscroll.store(true);
    }
    
    if (autoRunInstructions.load()) {
        unique_lock<mutex> lock_debug_breakpoints(mutDebugBreakpoints);
        auto& breakpoint_list = (pcSetToRam.load() ? breakpointsAddrTmp : breakpointsAddr);
        if (!(find(breakpoint_list.begin(), breakpoint_list.end(), std::pair(_bank, _pc)) != breakpoint_list.end())) {
            vhwmgr->SetProceedExecution(true);
        }
    }
    if (nextInstruction.load()) {
        vhwmgr->SetProceedExecution(true);
        nextInstruction.store(false);
    }
}
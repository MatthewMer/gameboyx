#include "GuiMgr.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "GameboyCartridge.h"
#include "imgui.h"
#include "nfd.h"
#include "logger.h"
#include "helper_functions.h"
#include "general_config.h"
#include "HardwareMgr.h"
#include <format>
#include <cmath>
#include <queue>

using namespace std;

namespace GUI {
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
        auto graph_settings = Backend::HardwareMgr::GetGraphicsSettings();
        framerateTarget = graph_settings.framerateTarget;
        fpsUnlimited = graph_settings.fpsUnlimited;
        tripleBuffering = graph_settings.tripleBuffering;
        vsync = graph_settings.presentModeFifo;

        auto aud_settings = Backend::HardwareMgr::GetAudioSettings();
        samplingRate = aud_settings.sampling_rate;
        samplingRateMax = aud_settings.sampling_rate_max;
        masterVolume = aud_settings.master_volume;
        lfeVolume = aud_settings.lfe_volume;
        reverbDelay = aud_settings.delay;
        reverbDecay = aud_settings.decay;
        hfOutputEnable = aud_settings.hf_output_enable;
        lfeOutputEnable = aud_settings.lfe_output_enable;
        lfeLowPassEnable = aud_settings.lfe_low_pass_enable;
        distLowPassEnable = aud_settings.dist_low_pass_enable;
        baseVolume = aud_settings.base_volume;

        vhwmgr = Emulation::VHardwareMgr::getInstance();

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

        mainFont = Backend::HardwareMgr::GetFont(0);
        IM_ASSERT(mainFont != nullptr);

        // TODO: clean up, move to separate function with possibility to change mapping in GUI
        keyboardMapping = std::unordered_map<SDL_Keycode, SDL_GameControllerButton>();

        keyboardMapping[SDLK_g] = SDL_CONTROLLER_BUTTON_X;
        keyboardMapping[SDLK_r] = SDL_CONTROLLER_BUTTON_Y;
        keyboardMapping[SDLK_d] = SDL_CONTROLLER_BUTTON_B;
        keyboardMapping[SDLK_f] = SDL_CONTROLLER_BUTTON_A;
        keyboardMapping[SDLK_DOWN] = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        keyboardMapping[SDLK_UP] = SDL_CONTROLLER_BUTTON_DPAD_UP;
        keyboardMapping[SDLK_LEFT] = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        keyboardMapping[SDLK_RIGHT] = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    }

    GuiMgr::~GuiMgr() {
        if (gameRunning) {
            vhwmgr->ShutdownHardware();
        }
        Emulation::VHardwareMgr::resetInstance();
    }

    /* ***********************************************************************************************************
        GUI PROCESS ENTRY POINT
    *********************************************************************************************************** */
    void GuiMgr::ProcessInput() {
        // debug
        static bool debug_enabled = showInstrDebugger;
        if (debug_enabled != showInstrDebugger) {
            debug_enabled = showInstrDebugger;

            vhwmgr->SetDebugEnabled(debug_enabled);
        }

        // keys
        auto& inputs = Backend::HardwareMgr::GetKeyQueue();
        while (!inputs.empty()) {
            pair<SDL_Keycode, bool>& input = inputs.front();

            switch (input.second) {
            case true:
                EventKeyDown(0, input.first);
                break;
            case false:
                EventKeyUp(0, input.first);
                break;
            }

            inputs.pop();
        }

        // gamepads
        auto& buttons = Backend::HardwareMgr::GetButtonQueue();
        while (!buttons.empty()) {
            tuple<int, SDL_GameControllerButton, bool>& button = buttons.front();

            switch (get<2>(button)) {
            case true:
                EventButtonDown(get<0>(button), get<1>(button));
                break;
            case false:
                EventButtonUp(get<0>(button), get<1>(button));
                break;
            }

            buttons.pop();
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
        const auto& scroll = Backend::HardwareMgr::GetScroll();
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

        bool show_any = false;

        ImGui::PushFont(mainFont);
        if (showInstrDebugger)      { ShowDebugInstructions();                      show_any = true; }
        if (showWinAbout)           { ShowWindowAbout();                            show_any = true; }
        if (showHardwareInfo)       { ShowHardwareInfo();                           show_any = true; }
        if (showMemoryObserver)     { ShowMemoryObserver();                         show_any = true; }
        if (showImGuiDebug)         { ImGui::ShowDebugLogWindow(&showImGuiDebug);   show_any = true; }
        if (showGraphicsInfo)       { ShowGraphicsInfo();                           show_any = true; }
        if (showGraphicsOverlay)    { ShowGraphicsOverlay(); }
        if (!gameRunning)           { ShowGameSelect();                             show_any = true; }
        if (showNewGameDialog)      { ShowNewGameDialog(); }
        if (showGraphicsSettings)   { ShowGraphicsSettings(); }
        if (showAudioSettings)      { ShowAudioSettings();                          show_any = true; }
        if (showMainMenuBar)        { ShowMainMenuBar();                            show_any = true; }
        if (showGraphicsDebugger)   { ShowDebugGraphics();                          show_any = true; }
        if (showNetworkSettings)    { ShowNetworkSettings();                        show_any = true; }
        if (showCallstack)          { ShowCallstack();                              show_any = true; }
        if (showEmulationGeneral)   { ShowEmulationGeneralSettings();               show_any = true; }
        ImGui::PopFont();

        if (show_any != showAny) {
            showAny = show_any;
            Backend::HardwareMgr::SetMouseAlwaysVisible(showAny);
        }

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
                    ImGui::MenuItem("General", nullptr, &showEmulationGeneral);
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
                // network
                ImGui::MenuItem("Network", nullptr, &showNetworkSettings);
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
                ImGui::MenuItem("Graphics Debugger", nullptr, &showGraphicsDebugger);
                ImGui::MenuItem("Memory Observer", nullptr, &showMemoryObserver);
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
            for (int i = 0; const auto & [key, value] : Config::EMULATION_SPEEDS) {
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
        if (ImGui::Begin("About", &showWinAbout, Config::WIN_CHILD_FLAGS)) {
            CheckWindow(ABOUT);
            ImGui::Text("GameboyX Emulator");
            ImGui::Text("Version: %s %d.%d", Config::GBX_RELEASE, Config::GBX_VERSION_MAJOR, Config::GBX_VERSION_MINOR, Config::GBX_VERSION_PATCH);
            ImGui::Text("Author: %s", Config::GBX_AUTHOR);
            ImGui::Spacing();
            ImGui::End();
        }
    }

    void GuiMgr::ShowDebugInstructions() {
        ImGui::SetNextWindowSize(Config::debug_instr_win_size);

        if (ImGui::Begin("Instruction Debugger", &showInstrDebugger, Config::WIN_CHILD_FLAGS)) {
            CheckWindow(DEBUG_INSTR);
            ShowDebugInstrButtonsHead();
            ShowDebugInstrTable();
            ShowDebugInstrSelect();
            ShowDebugInstrMiscData("Registers", debugInstrRegColNum, Config::DEBUG_REGISTER_COLUMNS, regValues);
            ShowDebugInstrMiscData("Flags and ISR", debugInstrFlagColNum, Config::DEBUG_FLAG_COLUMNS, flagValues);
            ShowDebugInstrMiscData("Misc data", debugInstrFlagColNum, Config::DEBUG_FLAG_COLUMNS, miscValues);
            ImGui::End();
        }
    }

    void GuiMgr::ShowDebugInstrButtonsHead() {
        if (gameRunning) {
            if (ImGui::Button("Step")) { ActionStepThroughExecution(); }
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
        /*
        ImGui::SameLine();
        if (ImGui::Button("Callstack")) { showCallstack = !showCallstack; }
        */
        ImGui::SameLine();
        bool auto_run = autoRun;
        ImGui::Checkbox("Auto run", &autoRun);
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
            ImGui::TextColored(Config::IMGUI_RED_COL, "PC set to RAM");
        }
        if (ImGui::BeginTable("instruction_debug", debugInstrColNum, Config::TABLE_FLAGS)) {
            for (int i = 0; i < debugInstrColNum; i++) {
                ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER, Config::DEBUG_INSTR_COLUMNS[i]);
            }

            if (gameRunning) {
                Emulation::instr_entry cur_entry;
            
                unique_lock<mutex> lock_debug_instr(mutDebugInstr);
                bool pc_set_to_ram = pcSetToRam.load();
                auto& table = (pc_set_to_ram ? debugInstrTableTmp : debugInstrTable);
                if (debugInstrAutoscroll.load()) {
                    lastIndex = currentIndex;

                    indexCurInstruction = currentIndex;
                    table.SetToIndex(indexCurInstruction);
                    bankAddrSelect = table.GetIndexCentre();

                    debugInstrAutoscroll.store(false);
                } else if(CheckScroll(DEBUG_INSTR, table)) {
                    bankAddrSelect = table.GetIndexCentre();
                }
                lock_debug_instr.unlock();
            
                auto& breakpoints = (pc_set_to_ram ? breakpointsTableTmp : breakpointsTable);

                while (table.Next(cur_entry)) {
                    GuiTable::bank_index current_index = table.GetIndex();

                    ImGui::TableNextColumn();

                    if (std::find(breakpoints.begin(), breakpoints.end(), current_index) != breakpoints.end()) {
                        ImGui::TextColored(Config::IMGUI_RED_COL, ">>>");
                    }
                    ImGui::TableNextColumn();

                    ImGui::Selectable(cur_entry.first.c_str(), current_index == indexCurInstruction, Config::SEL_FLAGS);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                        ActionSetBreakPoint(breakpoints, current_index);
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

    void GuiMgr::ShowCallstack() {
        ImGui::SetNextWindowSize(Config::debug_instr_win_size);

        if (ImGui::Begin("Callstack", &showCallstack, Config::WIN_CHILD_FLAGS)) {

            ImGui::End();
        }
    }

    void GuiMgr::ShowDebugInstrSelect() {
        if (gameRunning) {
            auto& table = (pcSetToRam ? debugInstrTableTmp : debugInstrTable);

            if (ImGui::InputInt("Memory bank", &bankAddrSelect.bank, 1, 100, Config::INPUT_INT_FLAGS)) {
                bankAddrSelect.address = 0;
                ActionSetToIndex(table, bankAddrSelect);
            }
            if (ImGui::InputInt("Memory address", &bankAddrSelect.address, 0, 100, Config::INPUT_INT_HEX_FLAGS)) {
                ActionSetToIndex(table, bankAddrSelect);
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::InputInt("Memory bank", &bankAddrSelect.bank);
            ImGui::InputInt("Memory address", &bankAddrSelect.address, 0);
            ImGui::EndDisabled();
        }
    }

    void GuiMgr::ShowDebugInstrMiscData(const char* _title, const int& _col_num, const std::vector<float>& _columns, const std::vector<Emulation::reg_entry>& _data) {
        ImGui::Separator();
        ImGui::TextColored(HIGHLIGHT_COLOR, _title);
        if (!_data.empty()) {
            if (ImGui::BeginTable(_title, _col_num, Config::TABLE_FLAGS)) {
                for (int i = 0; i < _col_num; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER, _columns[i]);
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


    void GuiMgr::ShowMemoryObserver() {
        ImGui::SetNextWindowSize(Config::debug_mem_win_size);

        if (ImGui::Begin("Memory Observer", &showMemoryObserver, Config::WIN_CHILD_FLAGS)) {
            CheckWindow(DEBUG_MEM);
            if (ImGui::BeginTabBar("memory_inspector_tabs", 0)) {

                if (gameRunning) {
                    for (auto & table : debugMemoryTables) {
                        ShowDebugMemoryTab(table);
                    }
                } else {
                    if (ImGui::BeginTabItem("NONE")) {
                        if (ImGui::BeginTable("no_memory", dbgMemColNum, Config::TABLE_FLAGS)) {
                            ImGui::PushStyleColor(ImGuiCol_Text, HIGHLIGHT_COLOR);
                            for (int j = 0; j < dbgMemColNum; j++) {
                                ImGui::TableSetupColumn(Config::DEBUG_MEM_COLUMNS[j].first.c_str(), Config::TABLE_COLUMN_FLAGS, Config::DEBUG_MEM_COLUMNS[j].second);
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

    void GuiMgr::ShowDebugMemoryTab(GuiTable::Table<Emulation::memory_entry>& _table) {
        string table_name = _table.name;

        if (ImGui::BeginTabItem(table_name.c_str())) {
            CheckScroll(DEBUG_MEM, _table);

            auto index = _table.GetIndexCentre();
            
            if (ImGui::InputInt("Memory bank", &index.bank, 1, 100, Config::INPUT_INT_FLAGS)) {
                index.address = 0;
                ActionSetToIndex(_table, index);
            }
            if (ImGui::InputInt("Memory address", &index.address, 0, 100, Config::INPUT_INT_HEX_FLAGS)) {
                ActionSetToIndex(_table, index);
            }

            dbgMemCellAnyHovered = false;

            if (ImGui::BeginTable(table_name.c_str(), dbgMemColNum, Config::TABLE_FLAGS)) {
                ImGui::PushStyleColor(ImGuiCol_Text, HIGHLIGHT_COLOR);
                for (int j = 0; j < dbgMemColNum; j++) {
                    ImGui::TableSetupColumn(Config::DEBUG_MEM_COLUMNS[j].first.c_str(), Config::TABLE_COLUMN_FLAGS, Config::DEBUG_MEM_COLUMNS[j].second);
                }
                ImGui::TableHeadersRow();
                ImGui::PopStyleColor();

                int line = 0;

                Emulation::memory_entry current_entry;
                while (_table.Next(current_entry)) {
                    ImGui::TableNextColumn();

                    ImGui::TextColored(HIGHLIGHT_COLOR, get<Emulation::MEM_ENTRY_ADDR>(current_entry).c_str());

                    // pointer to 16 bytes of table entry
                    u8* data = get<Emulation::MEM_ENTRY_REF>(current_entry);
                    for (int i = 0; i < get<Emulation::MEM_ENTRY_LEN>(current_entry); i++) {
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
        ImGui::SetNextWindowSize(Config::hw_info_win_size);

        if (ImGui::Begin("Hardware Info", &showHardwareInfo, Config::WIN_CHILD_FLAGS)) {
            CheckWindow(HW_INFO);
            if (ImGui::BeginTable("hardware_info", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < hwInfoColNum; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER, Config::HW_INFO_COLUMNS[i]);
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
        IO::check_and_create_config_folders();

        string current_path = Backend::FileIO::get_current_path();
        string s_path_rom_folder = Config::ROM_FOLDER;

        auto filter_items = std::vector<nfdfilteritem_t>();
        for (const auto& [key, value] : Emulation::FILE_EXTS) {
            filter_items.emplace_back(value.first.c_str(), value.second.c_str());
        }

        nfdchar_t* out_path = nullptr;
        const auto result = NFD_OpenDialog(&out_path, filter_items.data(), (nfdfiltersize_t)Emulation::FILE_EXTS.size(), s_path_rom_folder.c_str());

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

        ImGui::PushStyleColor(ImGuiCol_WindowBg, Config::IMGUI_BG_COLOR);
        if (ImGui::Begin("games_select", nullptr, Config::MAIN_WIN_FLAGS)) {
            CheckWindow(GAME_SELECT);
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 10, 10 });

            if (ImGui::BeginTable("game_list", mainColNum, Config::MAIN_TABLE_FLAGS)) {

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GUI_STYLE.ItemSpacing.x, GUI_STYLE.CellPadding.y * 2));

                for (int i = 0; const auto & [label, fraction] : Config::GAMES_COLUMNS) {
                    ImGui::TableSetupColumn(label.c_str(), Config::TABLE_COLUMN_FLAGS, MAIN_VIEWPORT->Size.x * fraction);
                    ImGui::TableSetupScrollFreeze(i, 0);
                    i++;
                }
                ImGui::TableHeadersRow();

                for (int i = 0; const auto & game : games) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    // TODO: icon
                    ImGui::TableNextColumn();

                    ImGui::Selectable(Emulation::FILE_EXTS.at(game->console).first.c_str(), gamesSelected[i].value, Config::SEL_FLAGS);
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
        ImVec2 window_pos = ImVec2((graphicsOverlayCorner & 1) ? GUI_IO.DisplaySize.x - Config::OVERLAY_DISTANCE : Config::OVERLAY_DISTANCE, (graphicsOverlayCorner & 2) ? GUI_IO.DisplaySize.y - Config::OVERLAY_DISTANCE : Config::OVERLAY_DISTANCE);
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

        if (ImGui::Begin("graphics_overlay", &showGraphicsOverlay, Config::WIN_OVERLAY_FLAGS)) {
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

        ImGui::SetNextWindowSize(Config::graph_settings_win_size);

        if (ImGui::Begin("Graphics Settings", &showGraphicsSettings, Config::WIN_CHILD_FLAGS)) {
            ImGui::TextColored(HIGHLIGHT_COLOR, "Application");
            if (ImGui::BeginTable("graphics app", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < 2; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER);
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

                if (ImGui::SliderInt("##fps", &framerateTarget, Config::APP_MIN_FRAMERATE, Config::APP_MAX_FRAMERATE)) {
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

            /*
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
            
            }*/
            ImGui::End();
        }
    }

    /* *************************************************************************************************
        AUDIO SETTINGS
    ************************************************************************************************* */
    void GuiMgr::ShowAudioSettings() {
        ImGui::SetNextWindowSize(Config::graph_settings_win_size);

        if (ImGui::Begin("Audio Settings", &showAudioSettings, Config::WIN_CHILD_FLAGS)) {

            ImGui::TextColored(HIGHLIGHT_COLOR, "General");
            if (ImGui::BeginTable("audio general", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < 2; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

                /* *************************************************************************************************
                    MASTER VOLUME
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Master Volume");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("sets overall volume");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::SliderFloat("##master_volume", &masterVolume, Config::APP_MIN_VOLUME, Config::APP_MAX_VOLUME)) {
                    ActionSetVolume();
                }
                ImGui::TableNextRow();

                /* *************************************************************************************************
                    DEVICE SAMPLING RATE
                ************************************************************************************************* */
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
                    for (const auto& [key, val] : Backend::Audio::SAMPLING_RATES) {
                        if (samplingRate == val.first) {
                            current_item = key;
                        }
                    }
                }

                if (ImGui::BeginCombo("##samplingrate", current_item)) {
                    for (const auto& [key, val] : Backend::Audio::SAMPLING_RATES) {
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

            ImGui::TextColored(HIGHLIGHT_COLOR, "LFE Channel");
            if (ImGui::BeginTable("audio lfe", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < 2; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER);
                }
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

                /* *************************************************************************************************
                    LFE CHANNEL OUTPUT
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Enable LFE channel");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("enables channel for low frequency output");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::Checkbox("##lfe", &lfeOutputEnable)) {
                    ActionSetOutputChannels();
                }
                ImGui::TableNextRow();

                /* *************************************************************************************************
                    LFE VOLUME
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("LFE volume");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("sets volume for LFE output");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::SliderFloat("##lfe_volume", &lfeVolume, Config::APP_MIN_LFE, Config::APP_MAX_LFE)) {
                    ActionSetVolume();
                }
                ImGui::TableNextRow();

                /* *************************************************************************************************
                    LFE LOW-PASS
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("LFE low-pass");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("enables a low-pass filter for the LFE channel, can mess with sound driver EQs\n(use only if there is no subwoofer for the LFE channel)");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::Checkbox("##lfe_lowpass", &lfeLowPassEnable)) {
                    ActionSetAudioFilters();
                }
                ImGui::TableNextRow();

                ImGui::PopStyleVar();
                ImGui::EndTable();
            }

            ImGui::TextColored(HIGHLIGHT_COLOR, "HF Channels");
            if (ImGui::BeginTable("audio hf", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < 2; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER);
                }
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

                /* *************************************************************************************************
                   HF CHANNEL OUTPUT
               ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Enable HF channels");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("enables channels for high frequency output");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::Checkbox("##hf", &hfOutputEnable)) {
                    ActionSetOutputChannels();
                }
                ImGui::TableNextRow();

                /* *************************************************************************************************
                    BASE VOLUME
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Base volume");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("sets volume for direct output");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::SliderFloat("##base_volume", &baseVolume, Config::APP_MIN_VOLUME, Config::APP_MAX_VOLUME)) {
                    ActionSetVolume();
                }
                ImGui::PopStyleVar();
                ImGui::EndTable();
            }
               
            ImGui::TextColored(HIGHLIGHT_COLOR, "Reverb");
            if (ImGui::BeginTable("audio reverb", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < 2; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER);
                }
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

                /* *************************************************************************************************
                    DIST LOW-PASS
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Reverberation with low-pass");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("enables a low-pass filter for reverberation. Creates an echo effect");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::Checkbox("##dist_lowpass", &distLowPassEnable)) {
                    ActionSetAudioFilters();
                }
                ImGui::TableNextRow();

                /* *************************************************************************************************
                    DIST DELAY
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Delay");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("sets delay for reverberation (how long it takes until an echo occurs)");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::SliderFloat("##reverb_delay", &reverbDelay, .0f, .5f)) {
                    ActionSetReverb();
                }
                ImGui::TableNextRow();

                /* *************************************************************************************************
                    DIST DECAY
                ************************************************************************************************* */
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Decay");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("sets reverbDecay factor for reverb (how much audio echoes)");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                if (ImGui::SliderFloat("##reverb_decay", &reverbDecay, .0f, .5f)) {
                    ActionSetReverb();
                }
                ImGui::TableNextRow();

                ImGui::PopStyleVar();
                ImGui::EndTable();
            }
            ImGui::End();
        }
    }

    void GuiMgr::ShowDebugGraphics() {
        ImGui::SetNextWindowSize(Config::graph_debug_win_size);

        if (ImGui::Begin("Graphics Debugger", &showGraphicsDebugger, Config::WIN_CHILD_FLAGS)) {
            CheckWindow(DEBUG_GRAPH);

            ShowDebugGraphicsSelects();
            ImGui::End();
        }
    }

    void GuiMgr::ShowDebugGraphicsSelects() {
        if (gameRunning) {
            for (auto& n : debugGraphicsSettings) {
                bool& set = get<2>(n);
                bool was_set = set;
                ImGui::Checkbox(get<1>(n).c_str(), &set);
                if (was_set != set) {
                    vhwmgr->SetGraphicsDebugSetting(set, get<0>(n));
                }
            }
        }
    }

    void GuiMgr::ShowNetworkSettings() {
        ImGui::SetNextWindowSize(Config::network_settings_win_size);

        if (ImGui::Begin("Network Settings", &showNetworkSettings, Config::WIN_CHILD_FLAGS)) {
            if (ImGui::BeginTable("network settings", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < 2; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("IP address");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("sets remote IPv4 address (server)");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();

                ImGui::PushItemWidth(35);
                for (int i = 0; auto & n : ipv4Address) {
                    std::string id = "##ip" + std::to_string(i);
                    if (i != 0) { ImGui::SameLine(); }
                    if (ImGui::InputInt(id.c_str(), &n, 0, 0, Config::INPUT_IP_FLAGS)) {
                        if (n < IP_ADDR_MIN) { n = IP_ADDR_MIN; }
                        if (n > IP_ADDR_MAX) { n = IP_ADDR_MAX; }
                    }
                    i++;
                }
                ImGui::PopItemWidth();
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Port");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::BeginTooltip()) {
                        ImGui::Text("sets remote port (server)");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::TableNextColumn();
                if (ImGui::InputInt("##port", &port, 0, 0, Config::INPUT_IP_FLAGS)) {
                    if (port < PORT_MIN) { port = PORT_MIN; }
                    if (port > PORT_MAX) { port = PORT_MAX; }
                }

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                bool open = Backend::HardwareMgr::CheckNetwork();
                if (open) {
                    if (ImGui::Button("Close")) {
                        Backend::HardwareMgr::CloseNetwork();
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(Config::IMGUI_GREEN_COL, "socket open");
                    if (ImGui::IsItemHovered()) {
                        if (ImGui::BeginTooltip()) {
                            ImGui::Text("current socket state");
                            ImGui::EndTooltip();
                        }
                    }
                } else {
                    if (ImGui::Button("Open")) {
                        std::string ip = "";
                        for (auto it = ipv4Address.begin(); it < ipv4Address.end(); it++) {
                            if (it != ipv4Address.begin()) { ip += "."; }
                            ip += std::to_string(*it);
                        }

                        Backend::network_settings nw_settings = {};
                        nw_settings.ipv4_address = ip;
                        nw_settings.port = port;

                        Backend::HardwareMgr::OpenNetwork(nw_settings);
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(Config::IMGUI_RED_COL, "socket closed");
                    if (ImGui::IsItemHovered()) {
                        if (ImGui::BeginTooltip()) {
                            ImGui::Text("current socket state");
                            ImGui::EndTooltip();
                        }
                    }
                }

                ImGui::PopStyleVar();
                ImGui::EndTable();
            }
            ImGui::End();
        }
    }

    void GuiMgr::ShowEmulationGeneralSettings() {
        ImGui::SetNextWindowSize(Config::emulation_general_win_size);

        if (ImGui::Begin("General", &showEmulationGeneral, Config::WIN_CHILD_FLAGS)) {
            ImGui::TextColored(HIGHLIGHT_COLOR, "Use boot ROM");

            if (ImGui::BeginTable("emulation settings", 2, Config::TABLE_FLAGS_NO_BORDER_OUTER_H)) {

                for (int i = 0; i < 2; i++) {
                    ImGui::TableSetupColumn("", Config::TABLE_COLUMN_FLAGS_NO_HEADER);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 3, 3 });

                for (auto & n : useBootRom) {
                    ImGui::TableNextColumn();
                    const std::string& name = Emulation::FILE_EXTS.at(n.first).first;
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::TableNextColumn();

                    ImGui::Checkbox(("##boot_rom_" + name).c_str(), &n.second.first);
                    ImGui::TableNextRow();

                    if (n.second.first) {
                        ImGui::TableNextColumn();
                        ImGui::TableNextColumn();
                        if (ImGui::BeginCombo(("##boot_rom_select_" + name).c_str(), BOOT_TYPES.at(n.second.second).c_str())) {
                            for (const auto& m : bootRomList.at(n.first)) {
                                bool selected = m == n.second.second;

                                if (ImGui::Selectable(BOOT_TYPES.at(m).c_str(), selected)) {
                                    n.second.second = m;
                                }
                                if (selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::TableNextRow();
                    }
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

    void GuiMgr::ActionStepThroughExecution() {
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

            if (std::find(current_table_breakpoints.begin(), current_table_breakpoints.end(), currentIndex) != current_table_breakpoints.end()) {
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

    void GuiMgr::ActionSetToIndex(GuiTable::TableBase& _table_obj, GuiTable::bank_index& _index) {
        _table_obj.SetToIndex(_index);
        _index = _table_obj.GetIndexCentre();
    }

    void GuiMgr::ActionSetToCurrentPC() {
        if (gameRunning) {
            auto& table = (pcSetToRam ? debugInstrTableTmp : debugInstrTable);

            table.SetToIndex(indexCurInstruction);
        }
    }

    void GuiMgr::ActionSetEmulationSpeed(const int& _index) {
        emulationSpeedsEnabled[currentSpeedIndex].value = false;
        emulationSpeedsEnabled[_index].value = true;

        for (int i = 0; const auto & [key, value] : Config::EMULATION_SPEEDS) {
            if (_index == i) {
                currentSpeed = key;
                vhwmgr->SetEmulationSpeed(currentSpeed);
            }
            i++;
        }

        currentSpeedIndex = _index;
    }

    bool GuiMgr::ActionAddGame(const string& _path_to_rom) {
        auto cartridge = Emulation::BaseCartridge::new_game(_path_to_rom);
        if (cartridge == nullptr) {
            return false;
        }

        if (cartridge->console == Emulation::CONSOLE_NONE) {
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

        if (!IO::write_games_to_config({ cartridge }, false)) {
            showNewGameDialog = false;
            LOG_ERROR("[emu] couldn't write new game to config");
            delete cartridge;
            return false;
        }

        AddGameGuiCtx(cartridge);
        return true;
    }

    void GuiMgr::ActionDeleteGames() {
        auto games_to_delete = vector<Emulation::BaseCartridge*>();
        for (int i = 0; const auto & n : gamesSelected) {
            if (n.value) games_to_delete.push_back(games[i]);
            i++;
        }

        if (games_to_delete.size() > 0) {
            IO::delete_games_from_config(games_to_delete);
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

    void GuiMgr::ActionSetBreakPoint(vector<GuiTable::bank_index>& _table_breakpoints, const GuiTable::bank_index& _current_index) {
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
        } else {
            int bank = _current_index.bank;
            int addr = _current_index.address;
            
            _table_breakpoints.emplace_back(_current_index);
        }
    }

    void GuiMgr::ActionSetFramerateTarget() {
        Backend::HardwareMgr::SetFramerateTarget(framerateTarget, fpsUnlimited);
    }

    void GuiMgr::ActionSetSwapchainSettings() {
        Backend::HardwareMgr::SetSwapchainSettings(vsync, tripleBuffering);
    }

    void GuiMgr::ActionSetVolume() {
        Backend::HardwareMgr::SetVolume(masterVolume, lfeVolume, baseVolume);
    }

    void GuiMgr::ActionSetSamplingRate() {
        Backend::HardwareMgr::SetSamplingRate(samplingRate);
    }

    void GuiMgr::ActionSetReverb() {
        Backend::HardwareMgr::SetReverb(reverbDelay, reverbDecay);
    }

    void GuiMgr::ActionSetOutputChannels() {
        Backend::HardwareMgr::SetAudioOutputEnable(hfOutputEnable, lfeOutputEnable);
    }

    void GuiMgr::ActionSetAudioFilters() {
        Backend::HardwareMgr::SetFilterEnable(distLowPassEnable, lfeLowPassEnable);
    }

    void GuiMgr::StartGame(const bool& _restart) {
        if (games.size() > 0) {
            // gather settings
            //virtual_graphics_settings graphics_settings = {};

            Emulation::emulation_settings emu_settings = {};
            emu_settings.debug_enabled = showInstrDebugger;
            emu_settings.emulation_speed = currentSpeed;

            auto* game = games[gameSelectedIndex];
            game->SetBootRom(false, "", Emulation::console_ids::CONSOLE_NONE);

            for (const auto& [key, value] : useBootRom) {
                if (key == game->console && value.first) {
                    game->SetBootRom(true, Config::BOOT_FOLDER + BOOT_TYPES.at(value.second), value.second);
                }
            }

            if (vhwmgr->InitHardware(games[gameSelectedIndex], emu_settings, _restart, 
                [this](Emulation::debug_data& _data) { this->DebugCallback(_data); }) != 0x00)
            {
                gameRunning = false;
            } else {
                debugInstrTable = GuiTable::Table<Emulation::instr_entry>(DEBUG_INSTR_LINES);
                auto& asm_tables = vhwmgr->GetAssemblyTables();
                for (auto& n : asm_tables) {
                    debugInstrTable.AddTableSectionDisposable(n);
                }

                auto& memory_tables = vhwmgr->GetMemoryTables();
                debugMemoryTables = std::vector<GuiTable::Table<Emulation::memory_entry>>();
                
                for (auto& n : memory_tables) {
                    debugMemoryTables.emplace_back(GuiTable::Table<Emulation::memory_entry>(DEBUG_MEM_LINES));
                    auto& table = debugMemoryTables.back();
                    table.name = n.GetMemoryType();
                    for (auto& m : n) {
                        table.AddTableSectionDisposable(m);
                    }
                }

                vhwmgr->GetGraphicsDebugSettings(debugGraphicsSettings);

                //vhwmgr->GetMemoryTypes(memoryTypes);

                gameRunning = vhwmgr->StartHardware() == 0x00;
            }
        }
    }

    void GuiMgr::CheckWindow(const windowID& _id) {
        windowsActive.at(_id) = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        windowsHovered.at(_id) = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    }

    // needs revision, but works so far
    template <class T>
    bool GuiMgr::CheckScroll(const windowID& _id, GuiTable::Table<T>& _table) {
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

            return scroll;
        } else {
            return false;
        }
    }

    void GuiMgr::AddGameGuiCtx(Emulation::BaseCartridge* _game_ctx) {
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

        IO::read_games_from_config(games);

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
        bankAddrSelect = {};
        breakpointsTable = std::vector<GuiTable::bank_index>();
        breakpointsTableTmp = std::vector<GuiTable::bank_index>();
        debugInstrTable = GuiTable::Table<Emulation::instr_entry>(DEBUG_INSTR_LINES);
        debugInstrTableTmp = GuiTable::Table<Emulation::instr_entry>(DEBUG_INSTR_LINES);
        regValues = std::vector<Emulation::reg_entry>();
        flagValues = std::vector<Emulation::reg_entry>();
        miscValues = std::vector<Emulation::reg_entry>();
        debugMemoryTables = std::vector<GuiTable::Table<Emulation::memory_entry>>();
        virtualFramerate = 0;
        hardwareInfo = std::vector<Emulation::data_entry>();
    }

    /* ***********************************************************************************************************
        IMGUIGAMEBOYX SDL FUNCTIONS
    *********************************************************************************************************** */
    void GuiMgr::EventKeyDown(const int& _player, SDL_Keycode& _key) {
        if (gameRunning) {
            switch (_key) {
            case SDLK_F3:
                ActionStepThroughExecution();
                break;
            case SDLK_LSHIFT:
                sdlkShiftDown = true;
                break;
            default:
                if (keyboardMapping.find(_key) != keyboardMapping.end()) {
                    vhwmgr->EventButtonDown(_player, keyboardMapping[_key]);
                }
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

    void GuiMgr::EventKeyUp(const int& _player, SDL_Keycode& _key) {
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
                Backend::HardwareMgr::ToggleFullscreen();
                break;
            case SDLK_ESCAPE:
                ActionGameStop();
                break;
            default:
                if (keyboardMapping.find(_key) != keyboardMapping.end()) {
                    vhwmgr->EventButtonUp(_player, keyboardMapping[_key]);
                }
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
                Backend::HardwareMgr::ToggleFullscreen();
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

    void GuiMgr::EventButtonDown(const int& _player, const SDL_GameControllerButton& _button) {
        if (gameRunning) {
            switch (_button) {
            default:
                vhwmgr->EventButtonDown(_player, _button);
                break;
            }
        } else {
            switch (_button) {
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                ActionGameSelectDown();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                ActionGameSelectUp();
                break;
            }
        }
    }

    void GuiMgr::EventButtonUp(const int& _player, const SDL_GameControllerButton& _button) {
        if (gameRunning) {
            switch (_button) {
            case SDL_CONTROLLER_BUTTON_BACK:
                ActionGameStop();
                break;
            default:
                vhwmgr->EventButtonUp(_player, _button);
                break;
            }
        } else {
            switch (_button) {
            case SDL_CONTROLLER_BUTTON_A:
                ActionGameStart();
                break;
            }
        }
    }

    /* ***********************************************************************************************************
        CALLBACKS
    *********************************************************************************************************** */
    void GuiMgr::DebugCallback(Emulation::debug_data& _data) {
        GuiTable::bank_index tmp = GuiTable::bank_index(_data.bank, _data.pc);

        // instruction debugger 
        unique_lock<mutex> lock_debug_instr(mutDebugInstr);
        if (tmp != currentIndex) {
            bool pc_set_to_ram = pcSetToRam.load();

            if (pc_set_to_ram != (tmp.bank < 0)) {
                pc_set_to_ram = !pc_set_to_ram;

                if (pc_set_to_ram) {
                    debugInstrTableTmp = GuiTable::Table<Emulation::instr_entry>(DEBUG_INSTR_LINES);
                    Emulation::assembly_tables table_tmp;
                    vhwmgr->GenerateTemporaryAssemblyTable(table_tmp);
                    debugInstrTableTmp.AddTableSectionDisposable(table_tmp.front());
                }
            }

            if (tmp.bank < 0) { tmp.bank = 0; }
            currentIndex = tmp;
            pcSetToRam.store(pc_set_to_ram);

            debugInstrAutoscroll.store(true);
        }
        lock_debug_instr.unlock();
    
        if (autoRunInstructions.load()) {
            unique_lock<mutex> lock_debug_breakpoints(mutDebugBreakpoints);
            auto& breakpoint_list = (pcSetToRam.load() ? breakpointsTableTmp : breakpointsTable);
            if (find(breakpoint_list.begin(), breakpoint_list.end(), tmp) == breakpoint_list.end()) {
                vhwmgr->SetProceedExecution(true);
            }
        }
        if (nextInstruction.load()) {
            vhwmgr->SetProceedExecution(true);
            nextInstruction.store(false);
        }
    }

    namespace IO {
        void games_from_string(vector<Emulation::BaseCartridge*>& _games, const vector<string>& _config_games);
        void games_to_string(const vector<Emulation::BaseCartridge*>& _games, vector<string>& _config_games);

        void check_and_create_config_folders() {
            Backend::FileIO::check_and_create_path(Config::CONFIG_FOLDER);
        }

        void check_and_create_config_files() {
            Backend::FileIO::check_and_create_file(Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);
        }

        void check_and_create_log_folders() {
            Backend::FileIO::check_and_create_path(Config::LOG_FOLDER);
        }

        void check_and_create_shader_folders() {
            Backend::FileIO::check_and_create_path(Config::SHADER_FOLDER);
            Backend::FileIO::check_and_create_path(Config::SPIR_V_FOLDER);
        }

        void check_and_create_save_folders() {
            Backend::FileIO::check_and_create_path(Config::SAVE_FOLDER);
        }

        void check_and_create_rom_folder() {
            Backend::FileIO::check_and_create_path(Config::ROM_FOLDER);
        }

        const unordered_map<Emulation::info_types, std::string> INFO_TYPES_MAP = {
            { Emulation::TITLE, "title" },
            { Emulation::CONSOLE,"console" },
            { Emulation::FILE_NAME,"file_name" },
            { Emulation::FILE_PATH,"file_path" },
            { Emulation::GAME_VER,"game_ver" }
        };

        void games_from_string(vector<Emulation::BaseCartridge*>& _games, const vector<string>& _config_games) {
            string line;
            _games.clear();

            string title = "";
            string file_name = "";
            string file_path = "";
            string console = "";
            string version = "";
            Emulation::console_ids id = Emulation::CONSOLE_NONE;

            bool entries_found = false;

            for (int i = 0; i < (int)_config_games.size(); i++) {
                if (_config_games[i].compare("") == 0) { continue; }
                line = Helpers::trim(_config_games[i]);

                // find start of entry
                if (line.find("[") == 0 && line.find("]") == line.length() - 1) {
                    if (entries_found) {
                        _games.emplace_back(Emulation::BaseCartridge::existing_game(title, file_name, file_path, id, version));
                        title = file_name = file_path = version = "";
                        id = Emulation::CONSOLE_NONE;
                    }
                    entries_found = true;

                    title = Helpers::split_string(Helpers::split_string(line, "[").back(), "]").front();
                }
                // filter entry parameters
                else if (_config_games[i].compare("") != 0 && entries_found) {
                    auto parameter = Helpers::split_string(line, "=");

                    if ((int)parameter.size() != 2) {
                        LOG_WARN("[emu] Error in line ", (i + 1));
                        continue;
                    }

                    parameter[0] = Helpers::trim(parameter[0]);
                    parameter[1] = Helpers::trim(parameter[1]);

                    if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::CONSOLE)) == 0) {
                        for (const auto& [key, value] : Emulation::FILE_EXTS) {
                            if (value.second.compare(parameter.back()) == 0) {
                                id = key;
                            }
                        }
                    } else if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::FILE_NAME)) == 0) {
                        file_name = parameter.back();
                    } else if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::FILE_PATH)) == 0) {
                        file_path = parameter.back();
                    } else if (parameter.front().compare(INFO_TYPES_MAP.at(Emulation::GAME_VER)) == 0) {
                        version = parameter.back();
                    }
                }
            }

            for (const auto& [key, value] : Emulation::FILE_EXTS) {
                if (value.second.compare(console) == 0) {
                    id = key;
                }
            }

            auto cartridge = Emulation::BaseCartridge::existing_game(title, file_name, file_path, id, version);
            if (cartridge != nullptr) {
                _games.emplace_back(cartridge);
            }

            return;
        }

        void games_to_string(const vector<Emulation::BaseCartridge*>& _games, vector<string>& _config_games) {
            _config_games.clear();
            for (const auto& n : _games) {
                _config_games.emplace_back("");
                _config_games.emplace_back("[" + n->title + "]");
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::FILE_NAME) + "=" + n->fileName);
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::FILE_PATH) + "=" + n->filePath);
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::CONSOLE) + "=" + Emulation::FILE_EXTS.at(n->console).second);
                _config_games.emplace_back(INFO_TYPES_MAP.at(Emulation::GAME_VER) + "=" + n->version);
            }
        }

        bool read_games_from_config(vector<Emulation::BaseCartridge*>& _games) {
            if (auto config_games = vector<string>(); Backend::FileIO::read_data(config_games, Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE)) {
                games_from_string(_games, config_games);
                return true;
            }

            return false;
        }


        bool write_games_to_config(const vector<Emulation::BaseCartridge*>& _games, const bool& _rewrite) {
            auto config_games = vector<string>();
            games_to_string(_games, config_games);

            if (Backend::FileIO::write_data(config_games, Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE, _rewrite)) {
                if (!_rewrite) LOG_INFO("[emu] ", (int)_games.size(), " game(s) added to " + Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);
                return true;
            }

            return false;
        }

        bool delete_games_from_config(vector<Emulation::BaseCartridge*>& _games) {
            auto games = vector<Emulation::BaseCartridge*>();
            if (!read_games_from_config(games)) {
                LOG_ERROR("[emu] reading games from ", Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);
                return false;
            }

            for (int i = (int)games.size() - 1; i >= 0; i--) {
                for (const auto& m : _games) {
                    if ((games[i]->filePath + games[i]->fileName).compare(m->filePath + m->fileName) == 0) {
                        delete games[i];
                        games.erase(games.begin() + i);
                    }
                }
            }

            if (!write_games_to_config(games, true)) {
                return false;
            }

            LOG_INFO("[emu] ", (int)_games.size(), " game(s) removed from ", Config::CONFIG_FOLDER + Config::GAMES_CONFIG_FILE);

            return true;
        }
    }
}
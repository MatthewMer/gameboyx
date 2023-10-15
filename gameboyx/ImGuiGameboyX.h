#pragma once

#include <SDL.h>
#include <imgui.h>
#include <vector>
#include "game_info.h"
#include "information_structs.h"
#include "helper_functions.h"
#include "ScrollableTable.h"

class ImGuiGameboyX {
public:
	// singleton instance access
	static ImGuiGameboyX* getInstance(machine_information& _machine_info, game_status& _game_status);
	static void resetInstance();

	// clone/assign protection
	ImGuiGameboyX(ImGuiGameboyX const&) = delete;
	ImGuiGameboyX(ImGuiGameboyX&&) = delete;
	ImGuiGameboyX& operator=(ImGuiGameboyX const&) = delete;
	ImGuiGameboyX& operator=(ImGuiGameboyX&&) = delete;

	// functions
	void ProcessGUI();
	// sdl functions
	void EventKeyDown(const SDL_Keycode& _key);
	void EventKeyUp(const SDL_Keycode& _key);
	void EventMouseWheel(const Sint32& _wheel_y);

	// main gets game context
	game_info& GetGameStartContext();
	// main reenables gui
	void GameStopped();
	void GameStartCallback();

private:
	// constructor
	ImGuiGameboyX(machine_information& _machine_info, game_status& _game_status);
	static ImGuiGameboyX* instance;
	~ImGuiGameboyX() = default;

	// special keys
	bool sdlkCtrlDown = false;
	bool sdlkCtrlDownFirst = false;
	bool sdlkShiftDown = false;
	bool sdlkDelDown = false;
	bool sdlkADown = false;
	bool sdlScrollDown = false;
	bool sdlScrollUp = false;

	// MAIN WINDOW GAME SELECT
	std::vector<game_info> games = std::vector<game_info>();
	std::vector<bool> gamesSelected = std::vector<bool>();
	int gameSelectedIndex = 0;
	bool deleteGames = false;
	int mainColNum = GAMES_COLUMNS.size();

	// debug instructions
	bank_index dbgInstrInstructionIndex = bank_index(0, 0);					// bank, index
	int dbgInstrBank = 0;
	int dbgInstrAddress = 0;
	bank_index dbgInstrCurrentBreakpoint = bank_index(0, 0);
	bool dbgInstrBreakpointSet = false;
	bool dbgInstrAutorun = false;
	int dbgInstrLastPC = -1;
	debug_instr_data dbgInstrCurrentEntry;
	int dbgInstrColNum = DEBUG_INSTR_COLUMNS.size();
	int dbgInstrColNumRegs = DEBUG_REGISTER_COLUMNS.size();

	// vector per memory type <start index, end index>
	std::vector<int> dbgMemBankIndex = std::vector<int>();
	memory_data dbgMemCurrentEntry;
	int dbgMemColNum = DEBUG_MEM_COLUMNS.size();

	bool showMainMenuBar = true;
	bool showWinAbout = false;
	bool showNewGameDialog = false;
	bool showGameSelect = true;
	bool showMemoryInspector = false;

	// gui functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowNewGameDialog();
	void ShowGameSelect();
	void ShowDebugInstructions();
	void ShowDebugMemoryInspector();
	void ShowHardwareInfo();

	// actions
	void ActionDeleteGames();
	bool ActionAddGame(const std::string& _path_to_rom);
	void ActionScrollToCurrentPC();
	void ActionStartGame(int _index);
	void ActionEndGame();
	void ActionRequestReset();
	void ActionBankSwitch(ScrollableTableBase& _table_obj, int& _bank);
	void ActionSearchAddress(ScrollableTableBase& _table_obj, int& _address);

	// helpers
	void AddGameGuiCtx(const game_info& _game_ctx);
	std::vector<game_info> DeleteGamesGuiCtx(const std::vector<int>& _index);
	void InitGamesGuiCtx();
	void ResetDebugInstr(); 
	void ResetMemInspector();
	bool CheckCurrentPCAutoScroll();
	bool CheckBreakPoint();
	void SetBreakPoint(const bank_index& _current_index);
	void WriteInstructionLog();
	void ResetEventMouseWheel();
	void SetBankAndAddress(ScrollableTableBase& _tyble_obj, int& _bank, int& _address);
	void SetupMemInspectorIndex();

	bool CheckScroll(ScrollableTableBase& _table_obj);

	void ProcessMainMenuKeys();

	// virtual hardware messages for debug
	machine_information& machineInfo;
	bool firstInstruction = true;

	// game status variables
	game_status& gameState;

	const ImGuiViewport* MAIN_VIEWPORT = ImGui::GetMainViewport();
	const ImGuiStyle& GUI_STYLE = ImGui::GetStyle();
};
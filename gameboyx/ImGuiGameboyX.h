#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The main GUI object which passes the draw data to the imgui backend (draws the screen) and manages it's contents
*	as well as processes parts of the user input (keyboard/mouse)
*/

#include <SDL.h>
#include <imgui.h>
#include <vector>
#include <array>
#include <queue>
#include "game_info.h"
#include "information_structs.h"
#include "helper_functions.h"
#include "ScrollableTable.h"

// workaround for vector<bool> being a special type of stl container, which uses single bits
// for beeleans and therefore no direct access by reference/pointer (google proxy reference object)
struct Bool{
	bool value = false;
};

class ImGuiGameboyX {
public:
	// singleton instance access
	static ImGuiGameboyX* getInstance(machine_information& _machine_info, game_status& _game_status, graphics_information& _graphics_info);
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
	void GameStarted();

private:
	// constructor
	ImGuiGameboyX(machine_information& _machine_info, game_status& _game_status, graphics_information& _graphics_info);
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
	const int mainColNum = GAMES_COLUMNS.size();

	// debug instructions
	bank_index dbgInstrInstructionIndex = bank_index(0, 0);					// bank, index
	int dbgInstrBank = 0;
	int dbgInstrAddress = 0;
	std::list<bank_index> dbgInstrBreakpoints = std::list<bank_index>();
	std::list<bank_index> dbgInstrBreakpointsTmp = std::list<bank_index>();
	bool dbgInstrPCoutOfRange = false;
	bool dbgInstrAutorun = false;
	int dbgInstrLastPC = -1;
	debug_instr_data dbgInstrCurrentEntry;
	const int dbgInstrColNum = DEBUG_INSTR_COLUMNS.size();
	const int dbgInstrColNumRegs = DEBUG_REGISTER_COLUMNS.size();
	const int dbgInstrColNumFlags = DEBUG_FLAG_COLUMNS.size();
	bool dbgInstrWasEnabled = false;

	// memory inspector
	std::vector<int> dbgMemBankIndex = std::vector<int>();
	memory_data dbgMemCurrentEntry;
	int dbgMemColNum = DEBUG_MEM_COLUMNS.size();
	Vec2 dbgMemCursorPos = Vec2(-1, -1);
	bool dbgMemCellHovered = false;
	bool dbgMemCellAnyHovered = false;

	bool graphicsShowOverlay = false;
	int graphicsOverlayCorner = 1;
	float graphicsFPSsamples[FPS_SAMPLES_NUM];
	std::queue<float> graphicsFPSfifo = std::queue<float>();
	std::queue<float> graphicsFPSfifoCopy = std::queue<float>();
	float graphicsFPSavg = .0f;
	int graphicsFPScount = 0;
	float graphicsFPScur = .0f;

	const int hwInfoColNum = HW_INFO_COLUMNS.size();

	int currentSpeedIndex = 0;
	std::vector<Bool> emulationSpeedsEnabled = std::vector<Bool>(EMULATION_SPEEDS.size(), { false });

	bool showMainMenuBar = true;
	bool showWinAbout = false;
	bool showNewGameDialog = false;
	bool showGameSelect = true;
	bool showMemoryInspector = false;
	bool showImGuiDebug = false;
	bool showGraphicsInfo = false;
	bool showEmulationSpeedSelect = false;

	// gui functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowNewGameDialog();
	void ShowGameSelect();
	void ShowDebugInstructions();
	void ShowDebugMemoryInspector();
	void ShowHardwareInfo();
	void ShowGraphicsInfo();
	void ShowGraphicsOverlay();

	// actions
	void ActionDeleteGames();
	bool ActionAddGame(const std::string& _path_to_rom);
	void ActionScrollToCurrentPC(ScrollableTableBase& _table_obj);
	void ActionStartGame(int _index);
	void ActionEndGame();
	void ActionRequestReset();
	void ActionBankSwitch(ScrollableTableBase& _table_obj, int& _bank);
	void ActionSearchAddress(ScrollableTableBase& _table_obj, int& _address);
	void ActionSetEmulationSpeed(const int& _index);

	// helpers
	void AddGameGuiCtx(const game_info& _game_ctx);
	std::vector<game_info> DeleteGamesGuiCtx(const std::vector<int>& _index);
	void InitGamesGuiCtx();
	void ResetDebugInstr(); 
	void ResetMemInspector();
	bool CheckCurrentPCAutoScroll();
	void SetBreakPoint(const bank_index& _current_index);
	void SetBreakPointTmp(const bank_index& _current_index);
	void ResetEventMouseWheel();
	void SetBankAndAddressScrollableTable(ScrollableTableBase& _tyble_obj, int& _bank, int& _address);
	void SetupMemInspectorIndex();

	bool CheckScroll(ScrollableTableBase& _table_obj);

	void ProcessMainMenuKeys();

	// virtual hardware messages for debug
	machine_information& machineInfo;
	bool firstInstruction = true;

	// game status variables
	game_status& gameState;

	// communication with graphics backend
	graphics_information& graphicsInfo;

	const ImGuiViewport* MAIN_VIEWPORT = ImGui::GetMainViewport();
	const ImGuiStyle& GUI_STYLE = ImGui::GetStyle();
	const ImGuiIO& GUI_IO = ImGui::GetIO();
	const ImVec4& HIGHLIGHT_COLOR = GUI_STYLE.Colors[ImGuiCol_TabActive];
};
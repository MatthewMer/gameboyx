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

#include "BaseCartridge.h"
#include "helper_functions.h"
#include "GuiTable.h"
#include "VHardwareMgr.h"

// workaround for vector<bool> being a special type of stl container, which uses single bits
// for beeleans and therefore no direct access by reference/pointer (google proxy reference object)
struct Bool{
	bool value = false;
};

enum windowID {
	GAME_SELECT,
	DEBUG_INSTR,
	DEBUG_MEM,
	HW_INFO,
	ABOUT
};

class GuiMgr {
public:
	// singleton instance access
	static GuiMgr* getInstance();
	static void resetInstance();

	// clone/assign protection
	GuiMgr(GuiMgr const&) = delete;
	GuiMgr(GuiMgr&&) = delete;
	GuiMgr& operator=(GuiMgr const&) = delete;
	GuiMgr& operator=(GuiMgr&&) = delete;

	// functions
	void ProcessGUI();

	// sdl functions
	void EventKeyDown(SDL_Keycode& _key);
	void EventKeyUp(SDL_Keycode& _key);
	void EventMouseWheel(const Sint32& _wheel_y);

private:
	// constructor
	GuiMgr();
	~GuiMgr();
	static GuiMgr* instance;

	VHardwareMgr* vhwmgr;

	// special keys
	bool sdlkCtrlDown = false;
	bool sdlkCtrlBlocked = false;
	bool sdlkShiftDown = false;
	bool sdlkDelDown = false;
	bool sdlkADown = false;
	bool sdlScrollDown = false;
	bool sdlScrollUp = false;

	// GUI elements to show -> ProcessGUI()
	bool showMainMenuBar = true;
	bool showGameSelect = true;
	bool showInstrDebugger = false;
	bool showHardwareInfo = false;
	bool showMemoryInspector = false;
	bool showGraphicsOverlay = false;
	bool showEmulationMenu = false;
	bool showEmulationSpeed = false;
	bool showWinAbout = false;
	bool showNewGameDialog = false;
	bool showImGuiDebug = false;
	bool showGraphicsInfo = false;

	// flow control values
	bool gameRunning = false;
	bool requestGameStart = false;
	bool requestGameStop = false;
	bool requestGameReset = false;
	bool requestDeleteGames = false;
	bool autoRun = false;
	bool pcSetToRam = false;

	void ProcessInput();
	bool windowActive;
	bool windowHovered;
	windowID activeId;
	windowID hoveredId;
	std::unordered_map<windowID, bool> windowsActive = {
		{GAME_SELECT, false},
		{DEBUG_INSTR, false},
		{DEBUG_MEM, false},
		{HW_INFO, false},
		{ABOUT, false}
	};
	std::unordered_map<windowID, bool> windowsHovered = {
		{GAME_SELECT, false},
		{DEBUG_INSTR, false},
		{DEBUG_MEM, false},
		{HW_INFO, false},
		{ABOUT, false}
	};
	void CheckWindow(const windowID& _id);
	template <class T>
	void CheckScroll(const windowID& _id, Table<T>& _table);

	// game select
	int gameSelectedIndex = 0;
	std::vector<Bool> gamesSelected = std::vector<Bool>();
	std::vector<BaseCartridge*> games = std::vector<BaseCartridge*>();
	const int mainColNum = (int)GAMES_COLUMNS.size();

	// debug instructions
	bool instrDebugWasEnabled = false;
	int bankSelect = 0;
	int addrSelect = 0;
	std::list<bank_index> breakpoints = std::list<bank_index>();
	std::list<bank_index> breakpointsTmp = std::list<bank_index>();
	int lastPc = -1;
	int currentPc = -1;
	int lastBank = -1;
	int currentBank = -1;
	bank_index debugInstrCurrentInstrIndex = bank_index(0, 0);
	const int debugInstrColNum = (int)DEBUG_INSTR_COLUMNS.size();
	const int debugInstrRegColNum = (int)DEBUG_REGISTER_COLUMNS.size();
	const int debugInstrFlagColNum = (int)DEBUG_FLAG_COLUMNS.size();
	Table<instr_entry> debugInstrTable = Table<instr_entry>(DEBUG_INSTR_LINES);
	Table<instr_entry> debugInstrTableTmp = Table<instr_entry>(DEBUG_INSTR_LINES);
	std::vector<reg_entry> regValues = std::vector<reg_entry>();
	std::vector<reg_entry> flagValues = std::vector<reg_entry>();
	std::vector<reg_entry> miscValues = std::vector<reg_entry>();

	// hardware info
	const int hwInfoColNum = (int)HW_INFO_COLUMNS.size();
	std::vector<data_entry> hardwareInfo = std::vector<data_entry>();
	float virtualFrequency = .0f;

	// memory inspector
	std::vector<Table<memory_entry>> debugMemoryTables = std::vector<Table<memory_entry>>();
	int dbgMemColNum = (int)DEBUG_MEM_COLUMNS.size();
	Vec2 dbgMemCursorPos = Vec2(-1, -1);
	bool dbgMemCellHovered = false;
	bool dbgMemCellAnyHovered = false;

	// graphics overlay (FPS)
	bool showGraphicsMenu = false;
	int virtualFramerate = 0;
	int graphicsOverlayCorner = 1;
	float graphicsFPSsamples[FPS_SAMPLES_NUM];
	std::queue<float> graphicsFPSfifo = std::queue<float>();
	std::queue<float> graphicsFPSfifoCopy = std::queue<float>();
	float graphicsFPSavg = .0f;
	int graphicsFPScount = 0;
	float graphicsFPScur = .0f;

	// emulation speed multiplier
	int currentSpeedIndex = 0;
	int currentSpeed = 1;
	std::vector<Bool> emulationSpeedsEnabled = std::vector<Bool>(EMULATION_SPEEDS.size(), { false });

	void ResetGUI();

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

	// main menur bar elements
	void ShowEmulationSpeeds();

	// instruction debugger components
	void ShowDebugInstrButtonsHead();
	void ShowDebugInstrTable();
	void ShowDebugInstrSelect();
	void ShowDebugInstrMiscData(const char* _title, const int& _col_num, const std::vector<float>& _columns, const std::vector<reg_entry>& _data);
	
	void ShowDebugMemoryTab(Table<memory_entry>& _table);

	// actions
	void ActionGameStart();
	void ActionGameStop();
	void ActionGameReset();
	void ActionDeleteGames();
	bool ActionAddGame(const std::string& _path_to_rom);
	void ActionGameSelectUp();
	void ActionGameSelectDown();
	void ActionContinueExecution();
	void ActionContinueExecutionBreakpoint();
	void ActionToggleMainMenuBar();
	void ActionSetToCurrentPC();
	void ActionSetToBank(TableBase& _table_obj, int& _bank);
	void ActionSetToAddress(TableBase& _table_obj, int& _address);
	void ActionSetEmulationSpeed(const int& _index);
	void ActionGamesSelect(const int& _index);
	void ActionGamesSelectAll();

	// helpers
	void AddGameGuiCtx(BaseCartridge* _game_ctx);
	void ReloadGamesGuiCtx();
	void CheckPCandBank();
	void ActionSetBreakPoint(std::list<bank_index>& _breakpoints, const bank_index& _current_index);
	void GetBankAndAddressTable(TableBase& _tyble_obj, int& _bank, int& _address);

	const ImGuiViewport* MAIN_VIEWPORT = ImGui::GetMainViewport();
	const ImGuiStyle& GUI_STYLE = ImGui::GetStyle();
	const ImGuiIO& GUI_IO = ImGui::GetIO();
	const ImVec4& HIGHLIGHT_COLOR = GUI_STYLE.Colors[ImGuiCol_TabActive];
};
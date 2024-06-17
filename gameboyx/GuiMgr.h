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
#include <mutex>

#include "BaseCartridge.h"
#include "helper_functions.h"
#include "GuiTable.h"
#include "VHardwareMgr.h"
#include "VHardwareStructs.h"

namespace GUI {
	// TODO: needs to be redone
	enum windowID {
		GAME_SELECT,
		DEBUG_INSTR,
		DEBUG_MEM,
		HW_INFO,
		ABOUT,
		DEBUG_GRAPH,
		GRAPGHICS_SETTINGS,
		NETWORK_SETTINGS,
		AUDIO_SETTINGS
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
		void ProcessInput();
		void ProcessGUI();

		// sdl functions
		void EventKeyDown(const int& _player, SDL_Keycode& _key);
		void EventKeyUp(const int& _player, SDL_Keycode& _key);
		void EventMouseWheel(const Sint32& _wheel_y);
		void EventButtonDown(const int& _player, const SDL_GameControllerButton& _button);
		void EventButtonUp(const int& _player, const SDL_GameControllerButton& _button);

	private:
		// constructor
		GuiMgr();
		~GuiMgr();
		static GuiMgr* instance;

		Emulation::VHardwareMgr* vhwmgr;

		// special keys
		bool sdlkCtrlDown = false;
		bool sdlkCtrlBlocked = false;
		bool sdlkShiftDown = false;
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
		bool showGraphicsSettings = false;
		bool showAudioSettings = false;
		bool showGraphicsDebugger = false;
		bool showNetworkSettings = false;
		bool showCallstack = false;

		bool showAny = false;

		// flow control values
		bool gameRunning = false;
		bool requestGameStart = false;
		bool requestGameStop = false;
		bool requestGameReset = false;
		bool requestDeleteGames = false;
		bool autoRun = false;

		bool windowActive;
		bool windowHovered;
		windowID activeId;
		windowID hoveredId;
		std::unordered_map<windowID, bool> windowsActive = {
			{GAME_SELECT, false},
			{DEBUG_INSTR, false},
			{DEBUG_MEM, false},
			{HW_INFO, false},
			{ABOUT, false},
			{DEBUG_GRAPH, false}
		};
		std::unordered_map<windowID, bool> windowsHovered = {
			{GAME_SELECT, false},
			{DEBUG_INSTR, false},
			{DEBUG_MEM, false},
			{HW_INFO, false},
			{ABOUT, false},
			{DEBUG_GRAPH, false}
		};
		void CheckWindow(const windowID& _id);
		template <class T>
		void CheckScroll(const windowID& _id, GuiTable::Table<T>& _table);

		// game select
		int gameSelectedIndex = 0;
		std::vector<Bool> gamesSelected = std::vector<Bool>();
		std::vector<Emulation::BaseCartridge*> games = std::vector<Emulation::BaseCartridge*>();
		const int mainColNum = (int)Config::GAMES_COLUMNS.size();

		// debug instructions
		int bankSelect = 0;
		int addrSelect = 0;
		std::vector<GuiTable::bank_index> breakpointsTable = std::vector<GuiTable::bank_index>();
		std::vector<GuiTable::bank_index> breakpointsTableTmp = std::vector<GuiTable::bank_index>();
		std::vector<std::pair<int, int>> breakpointsAddr = std::vector<std::pair<int, int>>();
		std::vector<std::pair<int, int>> breakpointsAddrTmp = std::vector<std::pair<int, int>>();
		int lastPc = -1;
		int lastBank = -1;
		alignas(64) std::atomic<int> currentPc = -1;
		alignas(64) std::atomic<int> currentBank = -1;
		alignas(64) std::atomic<bool> pcSetToRam = false;
		alignas(64) std::atomic<bool> debugInstrAutoscroll = false;
		GuiTable::bank_index debugInstrCurrentInstrIndex = GuiTable::bank_index(0, 0);
		const int debugInstrColNum = (int)Config::DEBUG_INSTR_COLUMNS.size();
		const int debugInstrRegColNum = (int)Config::DEBUG_REGISTER_COLUMNS.size();
		const int debugInstrFlagColNum = (int)Config::DEBUG_FLAG_COLUMNS.size();
		GuiTable::Table<Emulation::instr_entry> debugInstrTable = GuiTable::Table<Emulation::instr_entry>(DEBUG_INSTR_LINES);
		GuiTable::Table<Emulation::instr_entry> debugInstrTableTmp = GuiTable::Table<Emulation::instr_entry>(DEBUG_INSTR_LINES);
		std::vector<Emulation::reg_entry> regValues = std::vector<Emulation::reg_entry>();
		std::vector<Emulation::reg_entry> flagValues = std::vector<Emulation::reg_entry>();
		std::vector<Emulation::reg_entry> miscValues = std::vector<Emulation::reg_entry>();

		// hardware info
		const int hwInfoColNum = (int)Config::HW_INFO_COLUMNS.size();
		std::vector<Emulation::data_entry> hardwareInfo = std::vector<Emulation::data_entry>();
		float virtualFrequency = .0f;

		// memory inspector
		std::vector<GuiTable::Table<Emulation::memory_entry>> debugMemoryTables = std::vector<GuiTable::Table<Emulation::memory_entry>>();
		int dbgMemColNum = (int)Config::DEBUG_MEM_COLUMNS.size();
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
		std::vector<Bool> emulationSpeedsEnabled = std::vector<Bool>(Config::EMULATION_SPEEDS.size(), { false });

		// graphics settings
		int framerateTarget = 0;
		bool fpsUnlimited = false;
		bool tripleBuffering = false;
		bool vsync = false;

		bool showAudioMenu = false;
		float volume = .5f;
		float lfe = .5f;
		int samplingRateMax = 0;
		int samplingRate = 0;
		float delay = 0;
		float decay = 0;

		std::array<int, 4> ipv4Address = { 127, 0, 0, 1 };
		int port = 9800;

		std::vector<std::tuple<int, std::string, bool>> debugGraphicsSettings;

		std::unordered_map<SDL_Keycode, SDL_GameControllerButton> keyboardMapping = std::unordered_map<SDL_Keycode, SDL_GameControllerButton>();

		std::map<int, std::string> memoryTypes;

		std::vector<Emulation::callstack_data> callstack;

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
		void ShowGraphicsSettings();
		void ShowAudioSettings();
		void ShowDebugGraphics();
		void ShowNetworkSettings();
		void ShowCallstack();

		// main menur bar elements
		void ShowEmulationSpeeds();

		// instruction debugger components
		void ShowDebugInstrButtonsHead();
		void ShowDebugInstrTable();
		void ShowDebugInstrSelect();
		void ShowDebugInstrMiscData(const char* _title, const int& _col_num, const std::vector<float>& _columns, const std::vector<Emulation::reg_entry>& _data);

		void ShowDebugMemoryTab(GuiTable::Table<Emulation::memory_entry>& _table);

		void ShowDebugGraphicsSelects();

		// actions
		void ActionGameStart();
		void ActionGameStop();
		void ActionGameReset();
		void ActionDeleteGames();
		bool ActionAddGame(const std::string& _path_to_rom);
		void ActionGameSelectUp();
		void ActionGameSelectDown();
		void ActionContinueExecution();
		void ActionToggleMainMenuBar();
		void ActionSetToCurrentPC();
		void ActionSetToBank(GuiTable::TableBase& _table_obj, int& _bank);
		void ActionSetToAddress(GuiTable::TableBase& _table_obj, int& _address);
		void ActionSetEmulationSpeed(const int& _index);
		void ActionGamesSelect(const int& _index);
		void ActionGamesSelectAll();
		void ActionSetFramerateTarget();
		void ActionSetSwapchainSettings();
		void ActionSetVolume();
		void ActionSetSamplingRate();
		void ActionSetReverb();
		void ActionAutoExecution();

		// helpers
		void AddGameGuiCtx(Emulation::BaseCartridge* _game_ctx);
		void ReloadGamesGuiCtx();
		void ActionSetBreakPoint(std::vector<GuiTable::bank_index>& _table_breakpoints, const GuiTable::bank_index& _current_index, std::vector<std::pair<int, int>>& _breakpoints, GuiTable::TableBase& _table_obj);
		void GetBankAndAddressTable(GuiTable::TableBase& _tyble_obj, int& _bank, int& _address);
		void StartGame(const bool& _restart);

		const ImGuiViewport* MAIN_VIEWPORT = ImGui::GetMainViewport();
		const ImGuiStyle& GUI_STYLE = ImGui::GetStyle();
		const ImGuiIO& GUI_IO = ImGui::GetIO();
		const ImVec4& HIGHLIGHT_COLOR = GUI_STYLE.Colors[ImGuiCol_TabActive];
		ImFont* mainFont;


		void DebugCallback(Emulation::debug_data& _data);
		alignas(64) std::atomic<bool> nextInstruction = false;
		alignas(64) std::atomic<bool> autoRunInstructions = false;
		std::mutex mutDebugInstr;
		std::mutex mutDebugBreakpoints;
	};
}
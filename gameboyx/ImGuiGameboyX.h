#pragma once

#include <SDL.h>
#include <imgui.h>
#include <vector>
#include "game_info.h"
#include "information_structs.h"
#include "helper_functions.h"

class ImGuiGameboyX {
public:
	// singleton instance access
	static ImGuiGameboyX* getInstance(machine_information& _machine_info, game_status& _game_status);
	static void resetInstance();

	// clone/assign protection
	ImGuiGameboyX(ImGuiGameboyX const&) = delete;
	ImGuiGameboyX(ImGuiGameboyX &&) = delete;
	ImGuiGameboyX& operator=(ImGuiGameboyX const&) = delete;
	ImGuiGameboyX& operator=(ImGuiGameboyX &&) = delete;

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

	// variables
	std::vector<game_info> games = std::vector<game_info>();
	std::vector<bool> gamesSelected = std::vector<bool>();
	int gameSelectedIndex = 0;
	bool deleteGames = false;

	// debug instructions
	Vec2 debugInstrIndex = Vec2(0, 0);				// bank, index
	Vec2 debugScrollStartIndex = Vec2(0, 0);			// bank. index
	Vec2 debugScrollEndIndex = Vec2(0, 0);			// bank, index
	int debugAddrToSearch = 0;						// pc
	bool debugScrollDown = false;
	bool debugScrollUp = false;
	Vec2 debugCurrentBreakpoint = Vec2(0, 0);
	bool debugCurrentBreakpointSet = false;
	bool debugAutoRun = false;

	bool showMainMenuBar = true;
	bool showWinAbout = false;
	bool showNewGameDialog = false;
	bool showGameSelect = true;

	// gui functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowNewGameDialog();
	void ShowGameSelect();
	void ShowDebugInstructions();
	void ShowHardwareInfo();

	// actions
	void ActionDeleteGames();
	bool ActionAddGame(const std::string& _path_to_rom);

	void ActionProcessSpecialKeys();

	void ActionDebugScrollUp(const int& _num);
	void ActionDebugScrollDown(const int& _num);
	void ActionScrollToCurrentPC();

	// game run state
	void ActionStartGame(int _index);
	void ActionEndGame();
	void ActionRequestReset();
	
	void ActionBankSwitch();
	void ActionBankJumpToAddr();

	// helpers
	void AddGameGuiCtx(const game_info& _game_ctx);
	std::vector<game_info> DeleteGamesGuiCtx(const std::vector<int>& _index);
	void InitGamesGuiCtx();
	void ResetDebugInstr();
	void DebugSearchAddrSet();
	void BankScrollAddrSet(const int& _bank, const int& _index);
	void CurrentPCAutoScroll();
	bool CheckBreakPoint();
	void SetBreakPoint(const Vec2& _current_index);

	// virtual hardware messages for debug
	machine_information& machineInfo;
	bool firstInstruction = true;

	// game status variables
	game_status& gameState;
};
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
	static ImGuiGameboyX* getInstance(message_buffer& _msg_buffer, game_status& _game_status);
	static void resetInstance();

	// clone/assign protection
	ImGuiGameboyX(ImGuiGameboyX const&) = delete;
	ImGuiGameboyX(ImGuiGameboyX &&) = delete;
	ImGuiGameboyX& operator=(ImGuiGameboyX const&) = delete;
	ImGuiGameboyX& operator=(ImGuiGameboyX &&) = delete;

	// functions
	void ProcessGUI();
	// sdl functions
	void KeyDown(const SDL_Keycode& _key);
	void KeyUp(const SDL_Keycode& _key);
	void MouseWheelEvent(const Sint32& _wheel_y);

	// main gets game context
	game_info& GetGameStartContext();
	// main reenables gui
	void GameStopped();

private:
	// constructor
	ImGuiGameboyX(message_buffer& _msg_buffer, game_status& _game_status);
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
	int gamesPrevIndex = 0;
	bool deleteGames = false;

	// debug instructions
	Vec2 debug_instr_index = Vec2(0, 0);				// bank, index
	Vec2 debug_scroll_start_index = Vec2(0, 0);			// bank. index
	Vec2 debug_scroll_end_index = Vec2(0, 0);			// bank, index
	int debug_current_pc_top = 0;						// pc
	bool debug_scroll_down = false;
	bool debug_scroll_up = false;
	Vec2 break_point = Vec2(0, 0);
	bool break_point_set = false;
	bool auto_run = false;

	// game run state
	void ActionStartGame(int _index);
	void ActionEndGame();
	

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
	
	void ActionBankSwitch();
	void ActionBankJumpToAddr();

	// helpers
	void AddGameGuiCtx(const game_info& _game_ctx);
	std::vector<game_info> DeleteGamesGuiCtx(const std::vector<int>& _index);
	void InitGamesGuiCtx();
	void ResetDebugInstr();
	void BankPCAddrSet();
	void BankScrollAddrSet(const int& _bank, const int& _index);
	void ActionScrollToCurrentPC();
	void CurrentPCAutoScroll();
	bool CheckBreakPoint();
	void SetBreakPoint(const Vec2& _current_index);

	// virtual hardware messages for debug
	message_buffer& msgBuffer;
	bool firstInstruction = true;

	// game status variables
	game_status& gameStatus;
};
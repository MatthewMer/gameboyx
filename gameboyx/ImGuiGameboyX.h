#pragma once

#include <SDL.h>
#include <imgui.h>
#include <vector>
#include "game_info.h"
#include "information_structs.h"

class ImGuiGameboyX {
public:
	// singleton instance access
	static ImGuiGameboyX* getInstance(message_buffer& _msg_fifo, game_status& _game_status);
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

	// main gets game context
	game_info& GetGameStartContext();
	// main reenables gui
	void GameStopped();

private:
	// constructor
	ImGuiGameboyX(message_buffer& _msg_fifo, game_status& _game_status);
	static ImGuiGameboyX* instance;
	~ImGuiGameboyX() = default;

	// special keys
	bool sdlkCtrlDown = false;
	bool sdlkCtrlDownFirst = false;
	bool sdlkShiftDown = false;
	bool sdlkDelDown = false;
	bool sdlkADown = false;

	// variables
	std::vector<game_info> games = std::vector<game_info>();
	std::vector<bool> gamesSelected = std::vector<bool>();
	int gamesPrevIndex = 0;
	bool deleteGames = false;

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

	// actions
	void ActionDeleteGames();
	bool ActionAddGame(const std::string& _path_to_rom);
	void ActionProcessSpecialKeys();

	// helpers
	void AddGameGuiCtx(const game_info& _game_ctx);
	std::vector<game_info> DeleteGamesGuiCtx(const std::vector<int>& _index);
	void InitGamesGuiCtx();
	void CopyDebugInstructionOutput();
	void ClearOutput();
	void InitDebugOutputVectors();

	// virtual hardware messages for debug
	message_buffer& msgBuffer;
	std::vector<std::string> debugInstructionOutput = std::vector<std::string>();
	const int allowedOutputSize = 20;
	bool firstInstruction = true;

	// game status variables
	game_status& gameStatus;
};
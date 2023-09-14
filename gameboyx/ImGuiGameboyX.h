#pragma once

#include <SDL.h>
#include <imgui.h>
#include <vector>
#include "game_info.h"
#include "message_fifo.h"

class ImGuiGameboyX {
public:
	// singleton instance access
	static ImGuiGameboyX* getInstance(const message_fifo& _msg_fifo);
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

	// main checks game start
	bool CheckPendingGameStart() const;
	// main gets game context
	game_info& SetGameStartAndGetContext();
	// main reenables gui
	void GameStopped();

private:
	// constructor
	explicit ImGuiGameboyX(const message_fifo& _msg_fifo);
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
	bool gameRunning = false;
	bool pendingGameStart = false;
	int gameToStart = 0;

	bool showMainMenuBar = true;
	bool showWinAbout = false;
	bool showNewGameDialog = false;
	bool showGameSelect = true;

	// gui functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowNewGameDialog();
	void ShowGameSelect();

	// actions
	void ActionDeleteGames();
	bool ActionAddGame(const std::string& _path_to_rom);
	void ActionProcessSpecialKeys();

	// helpers
	void AddGameGuiCtx(const game_info& _game_ctx);
	std::vector<game_info> DeleteGamesGuiCtx(const std::vector<int>& _index);
	void InitGamesGuiCtx();

	// virtual hardware messages for debug
	const message_fifo& msgFifo;
};
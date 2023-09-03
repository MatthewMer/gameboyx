#pragma once

#include <SDL.h>
#include <imgui.h>
#include <vector>
#include "game_info.h"

class ImGuiGameboyX {
public:
	// singleton instance access
	static ImGuiGameboyX* getInstance();
	// clone/assign protection
	ImGuiGameboyX(ImGuiGameboyX& _instance) = delete;
	void operator=(const ImGuiGameboyX&) = delete;

	// functions
	void ShowGUI();
	// sdl functions
	void KeyDown(SDL_Keycode key);
	void KeyUp(SDL_Keycode key);

protected:
	// constructor
	ImGuiGameboyX();
	static ImGuiGameboyX* instance;

private:
	// variables
	std::vector<game_info> games = std::vector<game_info>();

	bool showGui = true;

	bool showMainMenuBar = true;
	bool showWinAbout = false;
	bool showNewGameDialog = false;
	bool showGameSelect = true;

	// gui functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowNewGameDialog();
	void ShowGameSelect();
};
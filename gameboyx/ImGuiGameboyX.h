#pragma once

#include "Cartridge.h"
#include <SDL.h>

class ImGuiGameboyX {
public:
	// singleton instance access
	static ImGuiGameboyX* getInstance();
	// clone/assign protection
	ImGuiGameboyX(ImGuiGameboyX& instance) = delete;
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

	bool show_gui = true;
	bool show_main_menu_bar = true;
	bool show_win_about = false;
	bool show_new_game_dialog = false;

	// gui functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowNewGameDialog();
	void ShowGameSelect();
};
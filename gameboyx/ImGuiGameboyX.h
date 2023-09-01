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
	ImGuiGameboyX() = default;
	static ImGuiGameboyX* instance;

private:
	// variables
	vector<game_info> games = vector<game_info>();

	bool show_gui = true;
	bool show_main_menu_bar = true;
	bool show_win_about = false;
	bool show_new_game_dialogue = false;

	// gui functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowNewGameDialogue();
	void ShowGameSelect();

	// misc functions
	bool ReadGamesFromConfig();
};
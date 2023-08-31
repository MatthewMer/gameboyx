#pragma once

class ImGuiGameboyX {
public:
	// constructor
	ImGuiGameboyX();

	// functions
	void ShowGUI();

private:
	// variables
	bool show_gui;
	bool show_main_menu_bar;
	bool show_win_about;
	bool show_game_select;

	// functions
	void ShowMainMenuBar();
	void ShowWindowAbout();
	void ShowGameSelect();
};
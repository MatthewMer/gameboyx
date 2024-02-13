#include "GameboyCTRL.h"
#include "gameboy_defines.h"

enum key_codes {
	START,
	SELECT,
	B,
	A,
	DOWN,
	UP,
	LEFT,
	RIGHT
};

void GameboyCTRL::InitKeyMap() {
	keyMap = std::unordered_map<SDL_Keycode, int>();

	keyMap[SDLK_e] = START;
	keyMap[SDLK_q] = SELECT;
	keyMap[SDLK_m] = B;
	keyMap[SDLK_n] = A;
	keyMap[SDLK_s] = DOWN;
	keyMap[SDLK_w] = UP;
	keyMap[SDLK_a] = LEFT;
	keyMap[SDLK_d] = RIGHT;
}

bool GameboyCTRL::SetKey(const SDL_Keycode& _key) {
	
	if (keyMap.find(_key) != keyMap.end()) {

		// set bool in case cpu writes to joyp register and requires current states to set the right bits
		// and directly set the corresponding bit and request interrupt in case of a high to low transition
		switch (keyMap[_key]) {
		case START:
			controlCtx->start_pressed = true;
			memInstance->SetButton(JOYP_START_DOWN, true);
			break;
		case SELECT:
			controlCtx->select_pressed = true;
			memInstance->SetButton(JOYP_SELECT_UP, true);
			break;
		case B:
			controlCtx->b_pressed = true;
			memInstance->SetButton(JOYP_B_LEFT, true);
			break;
		case A:
			controlCtx->a_pressed = true;
			memInstance->SetButton(JOYP_A_RIGHT, true);
			break;
		case DOWN:
			controlCtx->down_pressed = true;
			memInstance->SetButton(JOYP_START_DOWN, false);
			break;
		case UP:
			controlCtx->up_pressed = true;
			memInstance->SetButton(JOYP_SELECT_UP, false);
			break;
		case LEFT:
			controlCtx->left_pressed = true;
			memInstance->SetButton(JOYP_B_LEFT, false);
			break;
		case RIGHT:
			controlCtx->right_pressed = true;
			memInstance->SetButton(JOYP_A_RIGHT, false);
			break;
		}
		return true;
	} else {
		return false;
	}
}

bool GameboyCTRL::ResetKey(const SDL_Keycode& _key) {
	if (keyMap.find(_key) != keyMap.end()) {

		switch (keyMap[_key]) {
		case START:
			controlCtx->start_pressed = false;
			memInstance->UnsetButton(JOYP_START_DOWN, true);
			break;
		case SELECT:
			controlCtx->select_pressed = false;
			memInstance->UnsetButton(JOYP_SELECT_UP, true);
			break;
		case B:
			controlCtx->b_pressed = false;
			memInstance->UnsetButton(JOYP_B_LEFT, true);
			break;
		case A:
			controlCtx->a_pressed = false;
			memInstance->UnsetButton(JOYP_A_RIGHT, true);
			break;
		case DOWN:
			controlCtx->down_pressed = false;
			memInstance->UnsetButton(JOYP_START_DOWN, false);
			break;
		case UP:
			controlCtx->up_pressed = false;
			memInstance->UnsetButton(JOYP_SELECT_UP, false);
			break;
		case LEFT:
			controlCtx->left_pressed = false;
			memInstance->UnsetButton(JOYP_B_LEFT, false);
			break;
		case RIGHT:
			controlCtx->right_pressed = false;
			memInstance->UnsetButton(JOYP_A_RIGHT, false);
			break;
		}
		return true;
	} else {
		return false;
	}
}
#include "ControllerSM83.h"
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

void ControllerSM83::InitKeyMap() {
	machineInfo.key_map = std::unordered_map<SDL_Keycode, int>();

	machineInfo.key_map[SDLK_e] = START;
	machineInfo.key_map[SDLK_q] = SELECT;
	machineInfo.key_map[SDLK_m] = B;
	machineInfo.key_map[SDLK_n] = A;
	machineInfo.key_map[SDLK_s] = DOWN;
	machineInfo.key_map[SDLK_w] = UP;
	machineInfo.key_map[SDLK_a] = LEFT;
	machineInfo.key_map[SDLK_d] = RIGHT;
}

bool ControllerSM83::SetKey(const SDL_Keycode& _key) {
	
	if (machineInfo.key_map.find(_key) != machineInfo.key_map.end()) {

		// set bool in case cpu writes to joyp register and requires current states to set the right bits
		// and directly set the corresponding bit and request interrupt in case of a high to low transition
		switch (machineInfo.key_map[_key]) {
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

bool ControllerSM83::ResetKey(const SDL_Keycode& _key) {
	if (machineInfo.key_map.find(_key) != machineInfo.key_map.end()) {

		switch (machineInfo.key_map[_key]) {
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
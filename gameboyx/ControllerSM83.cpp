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

		switch (machineInfo.key_map[_key]) {
		case START:
			controlCtx->start_pressed = true;
			break;
		case SELECT:
			controlCtx->select_pressed = true;
			break;
		case B:
			controlCtx->b_pressed = true;
			break;
		case A:
			controlCtx->a_pressed = true;
			break;
		case DOWN:
			controlCtx->down_pressed = true;
			break;
		case UP:
			controlCtx->up_pressed = true;
			break;
		case LEFT:
			controlCtx->left_pressed = true;
			break;
		case RIGHT:
			controlCtx->right_pressed = true;
			break;
		}
		return true;
	} else {
		return false;
	}
}

bool ControllerSM83::ResetKey(const SDL_Keycode& _key) {
	switch (machineInfo.key_map[_key]) {
	case START:
		controlCtx->start_pressed = false;
		break;
	case SELECT:
		controlCtx->select_pressed = false;
		break;
	case B:
		controlCtx->b_pressed = false;
		break;
	case A:
		controlCtx->a_pressed = false;
		break;
	case DOWN:
		controlCtx->down_pressed = false;
		break;
	case UP:
		controlCtx->up_pressed = false;
		break;
	case LEFT:
		controlCtx->left_pressed = false;
		break;
	case RIGHT:
		controlCtx->right_pressed = false;
		break;
	default:
		return false;
		break;
	}

	return true;
}
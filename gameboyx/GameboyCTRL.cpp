#include "GameboyCTRL.h"
#include "gameboy_defines.h"

bool GameboyCTRL::SetKey(const int& _player, const SDL_GameControllerButton& _key) {
		// set bool in case cpu writes to joyp register and requires current states to set the right bits
		// and directly set the corresponding bit and request interrupt in case of a high to low transition
		switch (_key) {
		case SDL_CONTROLLER_BUTTON_START:
			controlCtx->start_pressed = true;
			memInstance->SetButton(JOYP_START_DOWN, true);
			break;
		case SDL_CONTROLLER_BUTTON_BACK:
			controlCtx->select_pressed = true;
			memInstance->SetButton(JOYP_SELECT_UP, true);
			break;
		case SDL_CONTROLLER_BUTTON_B:
			controlCtx->b_pressed = true;
			memInstance->SetButton(JOYP_B_LEFT, true);
			break;
		case SDL_CONTROLLER_BUTTON_A:
			controlCtx->a_pressed = true;
			memInstance->SetButton(JOYP_A_RIGHT, true);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			controlCtx->down_pressed = true;
			memInstance->SetButton(JOYP_START_DOWN, false);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			controlCtx->up_pressed = true;
			memInstance->SetButton(JOYP_SELECT_UP, false);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			controlCtx->left_pressed = true;
			memInstance->SetButton(JOYP_B_LEFT, false);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			controlCtx->right_pressed = true;
			memInstance->SetButton(JOYP_A_RIGHT, false);
			break;
		default:
			return false;
			break;
		}
		return true;
}

bool GameboyCTRL::ResetKey(const int& _player, const SDL_GameControllerButton& _key) {
	switch (_key) {
	case SDL_CONTROLLER_BUTTON_START:
		if (controlCtx->start_pressed) {
			controlCtx->start_pressed = false;
			memInstance->UnsetButton(JOYP_START_DOWN, true);
		}
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		if (controlCtx->select_pressed) {
			controlCtx->select_pressed = false;
			memInstance->UnsetButton(JOYP_SELECT_UP, true);
		}
		break;
	case SDL_CONTROLLER_BUTTON_B:
		if (controlCtx->b_pressed) {
			controlCtx->b_pressed = false;
			memInstance->UnsetButton(JOYP_B_LEFT, true);
		}
		break;
	case SDL_CONTROLLER_BUTTON_A:
		if (controlCtx->a_pressed) {
			controlCtx->a_pressed = false;
			memInstance->UnsetButton(JOYP_A_RIGHT, true);
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		if (controlCtx->down_pressed) {
			controlCtx->down_pressed = false;
			memInstance->UnsetButton(JOYP_START_DOWN, false);
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		if (controlCtx->up_pressed) {
			controlCtx->up_pressed = false;
			memInstance->UnsetButton(JOYP_SELECT_UP, false);
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		if (controlCtx->left_pressed) {
			controlCtx->left_pressed = false;
			memInstance->UnsetButton(JOYP_B_LEFT, false);
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		if (controlCtx->right_pressed) {
			controlCtx->right_pressed = false;
			memInstance->UnsetButton(JOYP_A_RIGHT, false);
		}
		break;
	default:
		return false;
		break;
	}
	return true;
}
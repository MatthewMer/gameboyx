#include "GameboyCTRL.h"
#include "gameboy_defines.h"

namespace Emulation {
	namespace Gameboy {
		GameboyCTRL::GameboyCTRL(std::shared_ptr<BaseCartridge> _cartridge) : BaseCTRL() {}

		void GameboyCTRL::Init() {
			m_MemInstance = std::dynamic_pointer_cast<GameboyMEM>(BaseMEM::s_GetInstance());
			controlCtx = m_MemInstance.lock()->GetControlContext();
		}

		bool GameboyCTRL::SetKey(const int& _player, const SDL_GameControllerButton& _key) {
			// set bool in case cpu writes to joyp register and requires current states to set the right bits
			// and directly set the corresponding bit and request interrupt in case of a high to low transition
			switch (_key) {
			case SDL_CONTROLLER_BUTTON_X:
				controlCtx->start_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_START_DOWN, true);
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				controlCtx->select_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_SELECT_UP, true);
				break;
			case SDL_CONTROLLER_BUTTON_B:
				controlCtx->b_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_B_LEFT, true);
				break;
			case SDL_CONTROLLER_BUTTON_A:
				controlCtx->a_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_A_RIGHT, true);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				controlCtx->down_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_START_DOWN, false);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				controlCtx->up_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_SELECT_UP, false);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				controlCtx->left_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_B_LEFT, false);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				controlCtx->right_pressed = true;
				m_MemInstance.lock()->SetButton(JOYP_A_RIGHT, false);
				break;
			default:
				return false;
				break;
			}
			return true;
		}

		bool GameboyCTRL::ResetKey(const int& _player, const SDL_GameControllerButton& _key) {
			switch (_key) {
			case SDL_CONTROLLER_BUTTON_X:
				if (controlCtx->start_pressed) {
					controlCtx->start_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_START_DOWN, true);
				}
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				if (controlCtx->select_pressed) {
					controlCtx->select_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_SELECT_UP, true);
				}
				break;
			case SDL_CONTROLLER_BUTTON_B:
				if (controlCtx->b_pressed) {
					controlCtx->b_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_B_LEFT, true);
				}
				break;
			case SDL_CONTROLLER_BUTTON_A:
				if (controlCtx->a_pressed) {
					controlCtx->a_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_A_RIGHT, true);
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				if (controlCtx->down_pressed) {
					controlCtx->down_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_START_DOWN, false);
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				if (controlCtx->up_pressed) {
					controlCtx->up_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_SELECT_UP, false);
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				if (controlCtx->left_pressed) {
					controlCtx->left_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_B_LEFT, false);
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				if (controlCtx->right_pressed) {
					controlCtx->right_pressed = false;
					m_MemInstance.lock()->UnsetButton(JOYP_A_RIGHT, false);
				}
				break;
			default:
				return false;
				break;
			}
			return true;
		}
	}
}
#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The GameboyCartridge source contains the class related to everythig with the actual ROM data of the dumped cartridge.
*	The VHardwareMgr creates at first an Instance of this class and reads the ROM dump to a vector based on the
*	information stored in the game_info Struct in game_info.h and reads basic information from it.
*	That's literally the only purpose of this class.
*/

/* ***********************************************************************************************************
	INCLUDES
*********************************************************************************************************** */
#include <vector>

#include "BaseCartridge.h"
#include "defs.h"

/* ***********************************************************************************************************
	CLASSES
*********************************************************************************************************** */
namespace Emulation {
	namespace Gameboy {
		class GameboyCartridge : protected BaseCartridge {
		public:
			friend class BaseCartridge;
			// constructor
			explicit GameboyCartridge(const Config::console_ids& _id, const std::string& _file) : BaseCartridge(_id, _file) {};
			// destructor
			~GameboyCartridge() = default;

			bool ReadRom() override;

		private:

		};
	}
}
#include "BaseMMU.h"

#include "GameboyMMU.h"
#include "logger.h"

#include <vector>

using namespace std;

void save_thread(BaseMMU* _obj, std::string _file) {
	bool saving = true;
	u32 time_delta = 0;

	while (saving) {
		time_delta = _obj->GetSaveTimeDiff();

		if (time_delta > 500000) {
			saving = false;
		}
	}

	auto data = _obj->GetSaveData();
	write_data(data, _file, true);
}

/* ***********************************************************************************************************
	MMU BASE CLASS
*********************************************************************************************************** */
BaseMMU* BaseMMU::instance = nullptr;

BaseMMU* BaseMMU::getInstance(BaseCartridge* _cartridge) {
	if (instance == nullptr) {
		switch (_cartridge->console) {
		case GB:
		case GBC:
			instance = GameboyMMU::getInstance(_cartridge);
			break;
		}
	}

	return instance;
}

BaseMMU* BaseMMU::getInstance() {
	if (instance == nullptr) {
		LOG_ERROR("[emu] MMU instance is nullptr");
	}
	return instance;
}

void BaseMMU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

u32 BaseMMU::GetSaveTimeDiff() {
	unique_lock<mutex> lock_save_time(mutSave);
	steady_clock::time_point save_time_cur = high_resolution_clock::now();
	return (u32)duration_cast<microseconds>(save_time_cur - saveTimePrev).count();
}

vector<char> BaseMMU::GetSaveData() {
	saveFinished.store(true);
	return saveData;
}
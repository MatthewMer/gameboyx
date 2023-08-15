#include "Cartridge.h"

#include <fstream>
#include "config.h"

Cartridge::Cartridge(char* file_path){
	this->file_path = file_path;
	read_buffer = std::vector<char>();
}

bool Cartridge::read_data() {
	// read cartridge data to buffer
	printf("\nFile to load: %s\n", file_path);

	std::ifstream file(file_path, std::ios::binary | std::ios::ate);
	if (!file) {
		printf("\nFailed to open file\n");
		return false;
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	read_buffer = std::vector<char>(size);

	if (!file.read(read_buffer.data(), size)) {
		printf("\nFailed to read file\n");
		return false;
	}

	file.close();

#ifdef DEBUG_READ
	printf("\nSize:\t\t\t%i Bytes / %.2f kB\n", (int)read_buffer.size(), read_buffer.size() / 1000.0);

	printf("\n\n ----- ROM DATA -----\n\nAddr.\t  ");
	for (int i = 0; i < 0x10; i++) {
		printf("0x%.2x ", i);
	}
	printf("\n\n");
	for (int y = 0; y < read_buffer.size() / 0x10; y++) {
		printf("0x%.6x\t", y * 0x10);
		for (int x = 0; x < 0x10; x++) {
			printf("  %.2x ", read_buffer[y * 0x10 + x] & 0xff);
		}
		printf("\n");
	}
#endif

	// interprete rom header







	return true;
}
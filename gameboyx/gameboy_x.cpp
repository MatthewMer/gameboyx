// gemuboy_color.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "SDL.h"

#include "Cartridge.h"

int main(int argc, char* argv[])
{




    Cartridge cart = Cartridge(argv[argc - 1]);
    cart.read_data();
}
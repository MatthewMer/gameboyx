#pragma once

#include "Cartridge.h"

class GraphicsUnitBase
{
public:
	// get/reset instance
	static GraphicsUnitBase* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	GraphicsUnitBase(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase(GraphicsUnitBase&&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase&&) = delete;

protected:
	// constructor
	GraphicsUnitBase() = default;

private:
	static GraphicsUnitBase* instance;
	~GraphicsUnitBase() = default;
};
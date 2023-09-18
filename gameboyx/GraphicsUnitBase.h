#pragma once

#include "Cartridge.h"

class GraphicsUnitBase
{
public:
	// get/reset instance
	static GraphicsUnitBase* getInstance();
	static void resetInstance();

	// clone/assign protection
	GraphicsUnitBase(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase(GraphicsUnitBase&&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase&&) = delete;

	// members
	virtual void NextFrame() = 0;

protected:
	// constructor
	GraphicsUnitBase() = default;
	~GraphicsUnitBase() = default;

	u8 isrFlags = 0;

private:
	static GraphicsUnitBase* instance;
};
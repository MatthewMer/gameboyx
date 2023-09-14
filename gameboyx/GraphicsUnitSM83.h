#pragma once

#include "GraphicsUnitBase.h"
#include "MemorySM83.h"

class GraphicsUnitSM83 : protected GraphicsUnitBase
{
public:
	friend class GraphicsUnitBase;

private:
	// constructor
	explicit GraphicsUnitSM83(const Cartridge& _cart_obj);
	// destructor
	~GraphicsUnitSM83() = default;

	MemorySM83* mem_instance = nullptr;
};
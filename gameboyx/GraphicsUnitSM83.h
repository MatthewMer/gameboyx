#pragma once

#include "GraphicsUnitBase.h"

class GraphicsUnitSM83 : protected GraphicsUnitBase
{
public:
	friend class GraphicsUnitBase;

private:
	// constructor
	explicit GraphicsUnitSM83(const Cartridge& _cart_obj);
	// destructor
	~GraphicsUnitSM83() = default;
};
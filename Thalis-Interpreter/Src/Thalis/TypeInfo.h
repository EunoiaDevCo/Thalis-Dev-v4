#pragma once

#include "Common.h"

struct TypeInfo
{
	TypeInfo(uint16 type, uint8 pointerLevel) :
		type(type), pointerLevel(pointerLevel) { }

	TypeInfo() :
		type(INVALID_ID), pointerLevel(0) {
	}

	uint16 type;
	uint8 pointerLevel;
};
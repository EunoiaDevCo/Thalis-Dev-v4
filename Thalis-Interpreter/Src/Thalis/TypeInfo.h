#pragma once

#include "Common.h"

struct TypeInfo
{
	TypeInfo(uint16 type, uint8 pointerLevel, uint16 derivedType = INVALID_ID) :
		type(type), pointerLevel(pointerLevel), derivedType(derivedType == INVALID_ID ? type : derivedType) { }

	TypeInfo() :
		type(INVALID_ID), pointerLevel(0), derivedType(INVALID_ID) {
	}

	uint16 type;
	uint8 pointerLevel;
	uint16 derivedType;
};
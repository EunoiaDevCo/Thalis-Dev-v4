#pragma once

#include "../TypeInfo.h"

#define IO_MODULE_ID 0

class Module
{
public:
	static TypeInfo GetFunctionReturnInfo(uint16 moduleID, uint16 function);
	static TypeInfo GetConstantTypeInfo(uint16 moduleID, uint16 constant);
};
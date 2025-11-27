#pragma once

#include "../TypeInfo.h"

#define IO_MODULE_ID		0
#define MATH_MODULE_ID		1
#define WINDOW_MODULE_ID	2
#define GL_MODULE_ID		3
#define FS_MODULE_ID		4
#define MEM_MODULE_ID		5

class Module
{
public:
	static TypeInfo GetFunctionReturnInfo(uint16 moduleID, uint16 function);
	static TypeInfo GetConstantTypeInfo(uint16 moduleID, uint16 constant);
};
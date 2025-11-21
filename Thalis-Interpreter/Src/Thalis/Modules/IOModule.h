#pragma once

#include "../Value.h"
#include "../TypeInfo.h"
#include <vector>

enum class IOModuleConstant : uint16
{

};

enum class IOModuleFunction : uint16
{
	PRINT,
	PRINTLN,
};

class Program;
class IOModule
{
public:
	static bool Init();
	static Value CallFunction(Program* program, uint16 function, const std::vector<Value>& args);
	static Value Constant(Program* program, uint16 constant);

	static TypeInfo GetFunctionReturnInfo(uint16 function);
	static TypeInfo GetConstantTypeInfo(uint16 constant);
};
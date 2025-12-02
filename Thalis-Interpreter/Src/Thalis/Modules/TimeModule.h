#pragma once

#include "../Value.h"
#include "../TypeInfo.h"
#include <vector>

enum class TimeModuleConstant : uint16
{

};

enum class TimeModuleFunction : uint16
{
	GET_MILLI, GET_MICRO, GET_NANO,
};

class Program;
class TimeModule
{
public:
	static bool Init();
	static Value CallFunction(Program* program, uint16 function, const std::vector<Value>& args);
	static Value Constant(Program* program, uint16 constant);

	static TypeInfo GetFunctionReturnInfo(uint16 function);
	static TypeInfo GetConstantTypeInfo(uint16 constant);

	static void SetBeginTime();
};
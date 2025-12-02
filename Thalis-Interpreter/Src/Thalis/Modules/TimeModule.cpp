#include "TimeModule.h"
#include "../Program.h"

#include <chrono>

static std::chrono::high_resolution_clock::time_point g_BeginTime;

bool TimeModule::Init()
{
	return true;
}

Value TimeModule::CallFunction(Program* program, uint16 function, const std::vector<Value>& args)
{
	switch ((TimeModuleFunction)function)
	{
	case TimeModuleFunction::GET_MILLI: {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		uint64 milli = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_BeginTime).count();
		return Value::MakeUInt64(milli, program->GetStackAllocator());
	} break;
	case TimeModuleFunction::GET_MICRO: {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		uint64 micro = std::chrono::duration_cast<std::chrono::microseconds>(now - g_BeginTime).count();
		return Value::MakeUInt64(micro, program->GetStackAllocator());
	} break;
	case TimeModuleFunction::GET_NANO: {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		uint64 nano = std::chrono::duration_cast<std::chrono::nanoseconds>(now - g_BeginTime).count();
		return Value::MakeUInt64(nano, program->GetStackAllocator());
	} break;
	}
}

void TimeModule::SetBeginTime()
{
	g_BeginTime = std::chrono::high_resolution_clock::now();
}

Value TimeModule::Constant(Program* program, uint16 constant)
{
	return Value::MakeNULL();
}

TypeInfo TimeModule::GetFunctionReturnInfo(uint16 function)
{
	return TypeInfo((uint16)ValueType::UINT64, 0);
}

TypeInfo TimeModule::GetConstantTypeInfo(uint16 constant)
{
	return TypeInfo(INVALID_ID, 0);
}
#include "IOModule.h"
#include <iostream>

bool IOModule::Init()
{
	return true;
}

Value IOModule::CallFunction(Program* program, uint16 function, const std::vector<Value>& args)
{
	switch ((IOModuleFunction)function)
	{
	case IOModuleFunction::PRINT: {
		std::cout << args[0];
	} break;
	case IOModuleFunction::PRINTLN: {
		if (args.empty())
			std::cout << std::endl;
		else
			std::cout << args[0] << std::endl;
	} break;
	}

	return Value::MakeNULL();
}

Value IOModule::Constant(Program* program, uint16 constant)
{
	return Value::MakeNULL();
}

TypeInfo IOModule::GetFunctionReturnInfo(uint16 function)
{
	return TypeInfo((uint16)ValueType::VOID_T, 0);
}

TypeInfo IOModule::GetConstantTypeInfo(uint16 constant)
{
	return TypeInfo(INVALID_ID, 0);
}

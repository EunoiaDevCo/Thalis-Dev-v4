#include "MemModule.h"
#include "../Program.h"

bool MemModule::Init()
{
	return true;
}

Value MemModule::CallFunction(Program* program, uint16 function, const std::vector<Value>& args)
{
	switch ((MemModuleFunction)function)
	{
	case MemModuleFunction::COPY: {
		Value dst = args[0];
		Value src = args[1];
		uint64 size = args[2].GetUInt64();

		memcpy(*(void**)dst.data, *(void**)src.data, size);
		return Value::MakeNULL();
	} break;
	case MemModuleFunction::ALLOC: {
		uint64 size = args[0].GetUInt64();
		void* data = program->GetHeapAllocator()->Alloc(size);
		return Value::MakePointer((uint16)ValueType::VOID_T, 1, data, program->GetStackAllocator());
	} break;
	case MemModuleFunction::FREE: {
		void* data = *(void**)args[0].data;
		program->GetHeapAllocator()->Free(data);
		return Value::MakeNULL();
	} break;
	case MemModuleFunction::SET: {
		void* data = *(void**)args[0].data;
		int32 value = args[1].GetInt32();
		uint64 size = args[2].GetUInt64();
		memset(data, value, size);
		return Value::MakeNULL();
	} break;
	}

	return Value::MakeNULL();
}

Value MemModule::Constant(Program* program, uint16 constant)
{
	return Value::MakeNULL();
}

TypeInfo MemModule::GetFunctionReturnInfo(uint16 function)
{
	return TypeInfo((uint16)ValueType::VOID_T, 0);
}

TypeInfo MemModule::GetConstantTypeInfo(uint16 constant)
{
	return TypeInfo(INVALID_ID, 0);
}
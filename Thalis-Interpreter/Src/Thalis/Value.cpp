#include "Value.h"
#include "Program.h"
#include "Class.h"

Value Value::Clone(Program* program, Allocator* allocator) const
{
	Value value;
	value.type = type;
	value.pointerLevel = pointerLevel;
	value.isArray = isArray;
	value.isReference = isReference;

	if (isReference)
	{
		value.data = allocator->Alloc(sizeof(void*));
		*(void**)value.data = *(void**)data;
	}
	else if (IsPointer())
	{
		value.data = allocator->Alloc(sizeof(void*));
		*(void**)value.data = data;
	}
	else
	{
		uint64 typeSize = program->GetTypeSize(type);
		value.data = allocator->Alloc(program->GetTypeSize(type));
		memcpy(value.data, data, typeSize);
	}

	return value;
}

Value Value::CastTo(Program* program, uint16 newType, uint8 pointerLevel, Allocator* allocator) const
{
	Value value;
	value.type = newType;
	value.pointerLevel = pointerLevel;
	value.isReference = false;
	if (IsPointer())
	{
		value.data = allocator->Alloc(sizeof(void*));
		memcpy(value.data, data, sizeof(void*));
	}
	else
	{
		uint64 typeSize = program->GetTypeSize(newType);
		value.data = allocator->Alloc(typeSize);
		switch ((ValueType)newType)
		{
		case ValueType::UINT8:   *(uint8*)value.data = GetUInt8(); break;
		case ValueType::UINT16:  *(uint16*)value.data = GetUInt16(); break;
		case ValueType::UINT32:  *(uint32*)value.data = GetUInt32(); break;
		case ValueType::UINT64:  *(uint64*)value.data = GetUInt64(); break;
		case ValueType::INT8:    *(int8*)value.data = GetInt8(); break;
		case ValueType::INT16:   *(int16*)value.data = GetInt16(); break;
		case ValueType::INT32:   *(int32*)value.data = GetInt32(); break;
		case ValueType::INT64:   *(int64*)value.data = GetInt64(); break;
		case ValueType::REAL32:  *(real32*)value.data = GetReal32(); break;
		case ValueType::REAL64:  *(real64*)value.data = GetReal64(); break;
		case ValueType::BOOL:    *(bool*)value.data = GetBool(); break;
		case ValueType::CHAR:    *(char*)value.data = GetChar(); break;
		default: {
			memcpy(value.data, data, typeSize);
		} break;
		}
	}

	return value;
}

Value Value::MakeArray(Program* program, uint16 type, uint8 elementPointerLevel, uint32* dimension, uint32 numDimensions, Allocator* allocator)
{
	uint64 typeSize = elementPointerLevel == 0 ? program->GetTypeSize(type) : sizeof(void*);

	ArrayHeader header {};
	header.elementPointerLevel = elementPointerLevel;
	header.numDimensions = numDimensions;
	uint32 numElements = 1;
	for (uint32 i = 0; i < numDimensions; i++)
	{
		header.dimensions[i] = dimension[i];
		numElements *= dimension[i];
	}

	uint64 arrayDataSize = typeSize * numElements + sizeof(ArrayHeader);
	uint8* arrayData = (uint8*)allocator->Alloc(arrayDataSize);
	memset(arrayData, 0, arrayDataSize);
	memcpy(arrayData, &header, sizeof(ArrayHeader));

	Value array;
	array.type = type;
	array.pointerLevel = 1 + elementPointerLevel;
	array.isArray = true;
	array.data = arrayData + sizeof(ArrayHeader);

	return array;
}

Value Value::MakeObject(Program* program, uint16 type, Allocator* allocator)
{
	Class* cls = program->GetClass(type);
	uint64 typeSize = cls->GetSize();

	void* data = allocator->Alloc(typeSize);
	memset(data, 0, typeSize);

	Value object;
	object.type = type;
	object.pointerLevel = 0;
	object.isArray = false;
	object.data = data;
	object.isReference = false;

	return object;
}

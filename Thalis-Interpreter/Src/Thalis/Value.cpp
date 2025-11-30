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

		*(void**)value.data = *(void**)data;
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

static void InitializeArrayHeaders(Program* program, uint8* data, Class* cls)
{
	const std::vector<ClassField>& members = cls->GetMemberFields();
	for (uint32 i = 0; i < members.size(); i++)
	{
		const ClassField& member = members[i];

		if (member.numDimensions > 0)
		{
			ArrayHeader* header = (ArrayHeader*)(data + member.offset - sizeof(ArrayHeader));
			header->numDimensions = member.numDimensions;
			header->elementPointerLevel = member.type.pointerLevel - 1;
			uint32 numElements = 1;
			for (uint32 j = 0; j < member.numDimensions; j++)
			{
				header->dimensions[j] = member.dimensions[j].first;
				numElements *= header->dimensions[j];
			}

			if (!Value::IsPrimitiveType(member.type.type) && member.type.pointerLevel == 0)
			{
				Class* elementClass = program->GetClass(member.type.type);
				for (uint32 j = 0; j < numElements; j++)
				{
					InitializeArrayHeaders(program, data + member.offset + elementClass->GetSize() * j, elementClass);
				}
			}
		}
		else if (!Value::IsPrimitiveType(member.type.type) && member.type.pointerLevel == 0)
		{
			InitializeArrayHeaders(program, data + member.offset, program->GetClass(member.type.type));
		}
	}
}

Value Value::MakeArray(Program* program, uint16 type, uint8 elementPointerLevel, uint32* dimension, uint32 numDimensions, Allocator* allocator)
{
	uint64 typeSize = elementPointerLevel == 0 ? program->GetTypeSize(type) : sizeof(void*);
	if (!Value::IsPrimitiveType(type) && elementPointerLevel == 0)
		typeSize += sizeof(VTable*);

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

	uint8* elements = arrayData + sizeof(ArrayHeader);
	if (elementPointerLevel == 0 && !IsPrimitiveType(type))
	{
		Class* cls = program->GetClass(type);
		VTable* vtable = cls->GetVTable();

		for (uint32 i = 0; i < numElements; i++)
		{
			uint8* elementBase = elements + i * typeSize;
			*(VTable**)elementBase = vtable;

			InitializeArrayHeaders(program, elementBase, cls);
		}
	}

	Value array;
	array.type = type;
	array.pointerLevel = 1 + elementPointerLevel;
	array.isArray = true;
	array.data = elements;
	array.isReference = false;

	return array;
}

Value Value::MakeObject(Program* program, uint16 type, Allocator* allocator)
{
	Class* cls = program->GetClass(type);
	uint64 typeSize = cls->GetSize();

	uint64 totalSize = sizeof(VTable*) + typeSize;
	uint8* memory = (uint8*)allocator->Alloc(totalSize);

	*(VTable**)memory = cls->GetVTable();

	Value value;
	value.type = type;
	value.pointerLevel = 0;
	value.data = memory + sizeof(VTable*);
	value.isArray = false;
	value.isReference = false;

	memset(value.data, 0, typeSize);
	InitializeArrayHeaders(program, (uint8*)value.data, cls);

	return value;
}

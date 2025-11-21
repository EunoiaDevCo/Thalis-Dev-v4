#include "Program.h"
#include "Class.h"
#include "ASTExpression.h"
#include "Modules/ModuleID.h"
#include "Modules/IOModule.h"
#include "Memory/Memory.h"

static Program* g_CompiledProgram;

Program::Program()
{
	m_ProgramCounter = 0;
	m_CurrentScope = -1;
	g_CompiledProgram = this;
	m_StackAllocator = new BumpAllocator(Memory::KBToBytes(128));
	m_HeapAllocator = new HeapAllocator();
	m_InitializationAllocator = new BumpAllocator(Memory::KBToBytes(32));
	m_ReturnAllocator = new BumpAllocator(Memory::KBToBytes(16));
	m_ScopeStack.reserve(64);
	m_ScopeStack.resize(64);
}

void Program::ExecuteProgram(uint32 pc)
{
	uint32 initStaticsPC = GetCodeSize();
	InitStatics();
	CleanUpForExecution();
	m_StackAllocator->Free();
	AddJumpCommand(pc);

	m_ProgramCounter = initStaticsPC;
	OpCode opcode = ReadOPCode();
	while (opcode != OpCode::END)
	{
		ExecuteOpCode(opcode);
		opcode = ReadOPCode();
	}

	for (uint32 i = 0; i < m_StringPool.size(); i++)
	{
		m_HeapAllocator->Free(m_StringPool[i]);
	}
}

void Program::AddJumpCommand(uint32 pc)
{
	WriteOPCode(OpCode::JUMP);
	WriteUInt32(pc);
}

void Program::AddPushConstantUInt8Command(uint8 value)
{
	WriteOPCode(OpCode::PUSH_UINT8);
	WriteUInt8(value);
}

void Program::AddPushConstantUInt16Command(uint16 value)
{
	WriteOPCode(OpCode::PUSH_UINT16);
	WriteUInt16(value);
}

void Program::AddPushConstantUInt32Command(uint32 value)
{
	WriteOPCode(OpCode::PUSH_UINT32);
	WriteUInt32(value);
}

void Program::AddPushConstantUInt64Command(uint64 value)
{
	WriteOPCode(OpCode::PUSH_UINT64);
	WriteUInt64(value);
}

void Program::AddPushConstantInt8Command(int8 value)
{
	WriteOPCode(OpCode::PUSH_INT8);
	WriteInt8(value);
}

void Program::AddPushConstantInt16Command(int16 value)
{
	WriteOPCode(OpCode::PUSH_INT16);
	WriteInt16(value);
}

void Program::AddPushConstantInt32Command(int32 value)
{
	WriteOPCode(OpCode::PUSH_INT32);
	WriteInt32(value);
}

void Program::AddPushConstantInt64Command(int64 value)
{
	WriteOPCode(OpCode::PUSH_INT64);
	WriteInt64(value);
}

void Program::AddPushConstantReal32Command(real32 value)
{
	WriteOPCode(OpCode::PUSH_REAL32);
	WriteReal32(value);
}

void Program::AddPushConstantReal64Command(real64 value)
{
	WriteOPCode(OpCode::PUSH_REAL64);
	WriteReal64(value);
}

void Program::AddPushConstantCharCommand(char value)
{
	WriteOPCode(OpCode::PUSH_CHAR);
	WriteInt8(value);
}

void Program::AddPushConstantBoolCommand(bool value)
{
	WriteOPCode(OpCode::PUSH_BOOL);
	WriteUInt8(value);
}

void Program::AddPushCStrCommand(char* value)
{
	WriteOPCode(OpCode::PUSH_CSTR);
	WriteCStr(value);
}

void Program::AddPushLocalCommand(uint16 slot)
{
	WriteOPCode(OpCode::PUSH_LOCAL);
	WriteUInt16(slot);
}

void Program::AddPushTypedNullCommand(uint16 type, uint8 pointerLevel)
{
	WriteOPCode(OpCode::PUSH_TYPED_NULL);
	WriteUInt16(type);
	WriteUInt8(pointerLevel);
}

void Program::AddPushIndexedCommand(uint64 typeSize, uint8 numIndices)
{
	WriteOPCode(OpCode::PUSH_INDEXED);
	WriteUInt64(typeSize);
	WriteUInt8(numIndices);
}

void Program::AddPushStaticVariableCommand(uint16 classID, uint64 offset, uint16 type, uint8 pointerLevel, bool isReference)
{
	WriteOPCode(OpCode::PUSH_STATIC_VARIABLE);
	WriteUInt16(classID);
	WriteUInt64(offset);
	WriteUInt16(type);
	WriteUInt8(pointerLevel);
	WriteUInt8(isReference);
}

void Program::AddPushMemberCommand(uint16 type, uint8 pointerLevel, uint64 offset, bool isReference)
{
	WriteOPCode(OpCode::PUSH_MEMBER);
	WriteUInt16(type);
	WriteUInt8(pointerLevel);
	WriteUInt64(offset);
	WriteUInt8(isReference);
}

uint32 Program::AddPushLoopCommand()
{
	WriteOPCode(OpCode::PUSH_LOOP);
	uint32 pos = GetCodeSize();
	WriteUInt32(0);
	WriteUInt32(0);
	return pos;
}

void Program::AddPopLoopCommand()
{
	WriteOPCode(OpCode::POP_LOOP);
}

void Program::AddDeclarePrimitiveCommand(ValueType type, uint16 slot)
{
	switch (type)
	{
	case ValueType::UINT8:	WriteOPCode(OpCode::DECLARE_UINT8); break;
	case ValueType::UINT16: WriteOPCode(OpCode::DECLARE_UINT16); break;
	case ValueType::UINT32: WriteOPCode(OpCode::DECLARE_UINT32); break;
	case ValueType::UINT64: WriteOPCode(OpCode::DECLARE_UINT64); break;
	case ValueType::INT8:	WriteOPCode(OpCode::DECLARE_INT8); break;
	case ValueType::INT16:	WriteOPCode(OpCode::DECLARE_INT16); break;
	case ValueType::INT32:	WriteOPCode(OpCode::DECLARE_INT32); break;
	case ValueType::INT64:	WriteOPCode(OpCode::DECLARE_INT64); break;
	case ValueType::REAL32: WriteOPCode(OpCode::DECLARE_REAL32); break;
	case ValueType::REAL64: WriteOPCode(OpCode::DECLARE_REAL64); break;
	case ValueType::BOOL:	WriteOPCode(OpCode::DECLARE_CHAR); break;
	case ValueType::CHAR:	WriteOPCode(OpCode::DECLARE_BOOL); break;
	}

	WriteUInt16(slot);
}

void Program::AddDeclarePointerCommand(uint16 type, uint8 pointerLevel, uint16 slot)
{
	WriteOPCode(OpCode::DECLARE_POINTER);
	WriteUInt16(type);
	WriteUInt8(pointerLevel);
	WriteUInt16(slot);
}

void Program::AddDeclareStackArrayCommand(uint16 type, uint8 elementPointerLevel, uint32* dimensions, uint8 numDimensions, uint32 initializerCount, uint16 slot)
{
	WriteOPCode(OpCode::DECLARE_STACK_ARRAY);
	WriteUInt16(type);
	WriteUInt8(elementPointerLevel);
	WriteUInt8(numDimensions);
	WriteUInt32(initializerCount);
	WriteUInt16(slot);
	for (uint32 i = 0; i < numDimensions; i++)
		WriteUInt32(dimensions[i]);
}

void Program::AddDeclareObjectWithConstructorCommand(uint16 type, uint16 functionID, uint16 slot)
{
	WriteOPCode(OpCode::DECLARE_OBJECT_WITH_CONSTRUCTOR);
	WriteUInt16(type);
	WriteUInt16(functionID);
	WriteUInt16(slot);
}

void Program::AddDeclareObjectWithAssignCommand(uint16 type, uint16 slot)
{
	WriteOPCode(OpCode::DECLARE_OBJECT_WITH_ASSIGN);
	WriteUInt16(type);
	WriteUInt16(slot);
}

void Program::AddDeclareReferenceCommand(uint16 slot)
{
	WriteOPCode(OpCode::DECLARE_REFERENCE);
	WriteUInt16(slot);
}

void Program::AddModuleConstantCommand(uint16 moduleID, uint16 constant)
{
	WriteOPCode(OpCode::MODULE_CONSTANT);
	WriteUInt16(moduleID);
	WriteUInt16(constant);
}

void Program::AddModuleFunctionCallCommand(uint16 moduleID, uint16 functionID, uint8 argCount, bool usesReturnValue)
{
	WriteOPCode(OpCode::MODULE_FUNCTION_CALL);
	WriteUInt16(moduleID);
	WriteUInt16(functionID);
	WriteUInt8(argCount);
	WriteUInt8(usesReturnValue);
}

void Program::AddStaticFunctionCallCommand(uint16 classID, uint16 functionID, bool usesReturnValue)
{
	WriteOPCode(OpCode::STATIC_FUNCTION_CALL);
	WriteUInt16(classID);
	WriteUInt16(functionID);
	WriteUInt8(usesReturnValue);
}

void Program::AddReturnCommand(uint8 returnInfo)
{
	WriteOPCode(OpCode::RETURN);
	WriteUInt8(returnInfo);
}

void Program::AddMemberFunctionCallCommand(uint16 classID, uint16 functionID, bool usesReturnValue)
{
	WriteOPCode(OpCode::MEMBER_FUNCTION_CALL);
	WriteUInt16(classID);
	WriteUInt16(functionID);
	WriteUInt8(usesReturnValue);
}

void Program::AddUnaryUpdateCommand(uint8 op, bool pushToStack)
{
	WriteOPCode(OpCode::UNARY_UPDATE);
	WriteUInt8(op);
	WriteUInt8(pushToStack);
}

void Program::WriteUInt64(uint64 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(uint64));
}

void Program::WriteUInt32(uint32 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(uint32));
}

void Program::WriteUInt16(uint16 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(uint16));
}

void Program::WriteUInt8(uint8 value)
{
	m_Code.push_back(value);
}

void Program::WriteInt8(int8 value)
{
	m_Code.push_back(static_cast<uint8>(value));
}

void Program::WriteInt16(int16 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(int16));
}

void Program::WriteInt32(int32 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(int32));
}

void Program::WriteInt64(int64 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(int64));
}

void Program::WriteReal32(real32 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(real32));
}

void Program::WriteReal64(real64 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(real64));
}

void Program::WriteOPCode(OpCode code)
{
	WriteUInt16((uint16)code);
}

void Program::WriteCStr(char* cstr)
{
	uint8* bytes = reinterpret_cast<uint8*>(&cstr);
	m_Code.insert(m_Code.end(), bytes, bytes + sizeof(char*));
}

void Program::PatchUInt32(uint32 pos, uint32 value)
{
	uint8* bytes = reinterpret_cast<uint8*>(&value);
	memcpy(&m_Code[pos], bytes, sizeof(uint32));
}

void Program::PatchPushLoopCommand(uint32 pos, uint32 start, uint32 end)
{
	uint8* startBytes = reinterpret_cast<uint8*>(&start);
	uint8* endBytes = reinterpret_cast<uint8*>(&end);
	std::memcpy(&m_Code[pos], startBytes, sizeof(uint32));
	std::memcpy(&m_Code[pos + sizeof(uint32)], endBytes, sizeof(uint32));
}

uint16 Program::AddClass(Class* cls)
{
	m_Classes.push_back(cls);
	uint16 id = (m_Classes.size() - 1) + 128;
	m_ClassNameMap[cls->GetName()] = id;
	cls->SetID(id);
	return id;
}

uint16 Program::GetClassID(const std::string& name)
{
	const auto&& it = m_ClassNameMap.find(name);
	if (it == m_ClassNameMap.end())
		return INVALID_ID;

	return it->second;
}

void Program::SetClassWithMainFunction(uint16 id)
{
	m_ClassWithMainFunction = id;
}

std::string Program::GetTypeName(uint16 type)
{
	switch ((ValueType)type)
	{
	case ValueType::BOOL:    return "bool";
	case ValueType::CHAR:    return "char";
	case ValueType::UINT8:   return "uint8";
	case ValueType::UINT16:  return "uint16";
	case ValueType::UINT32:  return "uint32";
	case ValueType::UINT64:  return "uint64";
	case ValueType::INT8:    return "int8";
	case ValueType::INT16:   return "int16";
	case ValueType::INT32:   return "int32";
	case ValueType::INT64:   return "int64";
	case ValueType::REAL32:  return "real32";
	case ValueType::REAL64:  return "real64";
	case ValueType::VOID_T:	 return "void";
	case ValueType::TEMPLATE_TYPE: return "template_type";
	default: return GetClass(type)->GetName();
	}
}

Class* Program::GetClass(uint16 id)
{
	return m_Classes[id - 128];
}

Class* Program::GetClassByName(const std::string& name)
{
	for (uint32 i = 0; i < m_Classes.size(); i++)
		if (m_Classes[i]->GetName() == name)
			return m_Classes[i];

	return nullptr;
}

uint16 Program::GetModuleID(const std::string& name)
{
	const auto&& it = m_ModuleNameMap.find(name);
	if (it == m_ModuleNameMap.end())
		return INVALID_ID;
	return it->second;
}

void Program::AddModule(const std::string& name, uint16 id)
{
	m_ModuleNameMap[name] = id;
}

uint64 Program::GetTypeSize(uint16 type)
{
	switch ((ValueType)type)
	{
	case ValueType::UINT8:	return sizeof(uint8);
	case ValueType::UINT16: return sizeof(uint16);
	case ValueType::UINT32: return sizeof(uint32);
	case ValueType::UINT64: return sizeof(uint64);
	case ValueType::INT8:	return sizeof(int8);
	case ValueType::INT16:	return sizeof(int16);
	case ValueType::INT32:	return sizeof(int32);
	case ValueType::INT64:	return sizeof(int64);
	case ValueType::REAL32: return sizeof(real32);
	case ValueType::REAL64: return sizeof(real64);
	case ValueType::CHAR:	return sizeof(char);
	case ValueType::BOOL:	return sizeof(bool);
	case ValueType::VOID_T:	return 0;
	case ValueType::TEMPLATE_TYPE: return 0;
	}

	return GetClass(type)->GetSize();
}

uint32 Program::GetCodeSize() const
{
	return m_Code.size();
}

bool Program::Resolve()
{
	for (uint32 i = 0; i < m_CreatedExpressions.size(); i++)
		m_CreatedExpressions[i]->Resolve(this);

	return true;
}

void Program::EmitCode()
{
	for (uint32 i = 0; i < m_Classes.size(); i++)
		m_Classes[i]->EmitCode(this);
}

Program* Program::GetCompiledProgram()
{
	return g_CompiledProgram;
}

void Program::ExecuteOpCode(OpCode opcode)
{
	switch (opcode)
	{
	case OpCode::JUMP: {
		m_ProgramCounter = ReadUInt32();
	} break;
	case OpCode::JUMP_IF_FALSE: {
		uint32 target = ReadUInt32();
		Value condition = m_Stack.back();
		m_Stack.pop_back();
		if (!condition.GetBool())
			m_ProgramCounter = target;
	} break;
	case OpCode::PUSH_UINT8: {
		m_Stack.push_back(Value::MakeUInt8(ReadUInt8(), m_StackAllocator));
	} break;
	case OpCode::PUSH_UINT16: {
		m_Stack.push_back(Value::MakeUInt16(ReadUInt16(), m_StackAllocator));
	} break;
	case OpCode::PUSH_UINT32: {
		m_Stack.push_back(Value::MakeUInt32(ReadUInt32(), m_StackAllocator));
	} break;
	case OpCode::PUSH_UINT64: {
		m_Stack.push_back(Value::MakeUInt64(ReadUInt64(), m_StackAllocator));
	} break;
	case OpCode::PUSH_INT8: {
		m_Stack.push_back(Value::MakeInt8(ReadInt8(), m_StackAllocator));
	} break;
	case OpCode::PUSH_INT16: {
		m_Stack.push_back(Value::MakeInt16(ReadInt16(), m_StackAllocator));
	} break;
	case OpCode::PUSH_INT32: {
		m_Stack.push_back(Value::MakeInt32(ReadInt32(), m_StackAllocator));
	} break;
	case OpCode::PUSH_INT64: {
		m_Stack.push_back(Value::MakeInt64(ReadInt64(), m_StackAllocator));
	} break;
	case OpCode::PUSH_REAL32: {
		m_Stack.push_back(Value::MakeReal32(ReadReal32(), m_StackAllocator));
	} break;
	case OpCode::PUSH_REAL64: {
		m_Stack.push_back(Value::MakeReal64(ReadReal64(), m_StackAllocator));
	} break;
	case OpCode::PUSH_CHAR: {
		m_Stack.push_back(Value::MakeChar(ReadInt8(), m_StackAllocator));
	} break;
	case OpCode::PUSH_BOOL: {
		m_Stack.push_back(Value::MakeBool(ReadUInt8(), m_StackAllocator));
	} break;
	case OpCode::PUSH_CSTR: {
		m_Stack.push_back(Value::MakePointer((uint16)ValueType::CHAR, 1, ReadCStr(), m_StackAllocator));
	} break;
	case OpCode::PUSH_LOCAL: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		m_Stack.push_back(frame->GetLocal(slot).Actual());
	} break;
	case OpCode::PUSH_TYPED_NULL: {
		uint16 type = ReadUInt16();
		uint8 pointerLevel = ReadUInt8();
		m_Stack.push_back(Value::MakeNULL(type, pointerLevel));
	} break;
	case OpCode::PUSH_INDEXED: {
		uint64 typeSize = ReadUInt64();
		uint8 numIndices = ReadUInt8();
		for (uint8 i = 0; i < numIndices; i++)
		{
			m_Dimensions[i] = m_Stack.back().GetUInt32();
			m_Stack.pop_back();
		}

		Value base = m_Stack.back();
		m_Stack.pop_back();
		
		Value element;
		element.type = base.type;
		element.pointerLevel = base.pointerLevel - 1;
		element.isArray = false;
		element.isReference = false;
		
		if (base.isArray) //stack array
		{
			uint32 index = base.Calculate1DArrayIndex(m_Dimensions);

			if (element.pointerLevel > 0)
			{
				element.data = (void**)((uint8*)base.data + index * sizeof(void*));
			}
			else
			{
				element.data = (uint8*)base.data + index * typeSize;
			}
		}
		else if (base.IsPointer())
		{
			uint8* ptr = (uint8*)base.data;

			uint8 pointerLevel = element.pointerLevel;
			for (uint32 i = 0; i < numIndices; i++)
			{
				if (pointerLevel > 0)
				{
					ptr = *(uint8**)((uint8*)ptr + m_Dimensions[i] * sizeof(void*));
				}
				else
				{
					ptr += m_Dimensions[i] * typeSize;
				}

				pointerLevel--;
			}

			element.data = ptr;
		}

		m_Stack.push_back(element);
	} break;
	case OpCode::PUSH_STATIC_VARIABLE: {
		uint16 classID = ReadUInt16();
		uint64 offset = ReadUInt64();
		uint16 type = ReadUInt16();
		uint8 pointerLevel = ReadUInt8();
		bool isReference = ReadUInt8();

		Value value;
		value.type = type;
		value.pointerLevel = pointerLevel;
		value.isArray = false;
		value.data = GetClass(classID)->GetStaticData(offset);
		value.isReference = isReference;

		m_Stack.push_back(value);
	} break;
	case OpCode::PUSH_MEMBER: {
		Value base = m_Stack.back();
		m_Stack.pop_back();

		Value member;
		member.type = ReadUInt16();
		member.pointerLevel = ReadUInt8();
		member.isArray = false;
		uint64 offset = ReadUInt64();
		member.isReference = ReadUInt8();
		member.data = (uint8*)base.data + offset;

		m_Stack.push_back(member);
	} break;
	case OpCode::PUSH_THIS: {
		m_Stack.push_back(m_ThisStack.back());
	} break;
	case OpCode::DECLARE_UINT8: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeUInt8(assignValue.GetUInt8(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_UINT16: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeUInt16(assignValue.GetUInt16(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_UINT32: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeUInt32(assignValue.GetUInt32(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_UINT64: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeUInt64(assignValue.GetUInt64(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_INT8: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeInt8(assignValue.GetInt8(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_INT16: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeInt16(assignValue.GetInt16(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_INT32: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeInt32(assignValue.GetInt32(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_INT64: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeInt64(assignValue.GetInt64(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_REAL32: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeReal32(assignValue.GetReal32(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_REAL64: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeReal64(assignValue.GetReal64(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_CHAR: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeChar(assignValue.GetChar(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_BOOL: {
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back();
		m_Stack.pop_back();
		frame->DeclareLocal(slot, Value::MakeBool(assignValue.GetBool(), m_StackAllocator));
	} break;
	case OpCode::DECLARE_POINTER: {
		uint16 type = ReadUInt16();
		uint8 pointerLevel = ReadUInt8();
		uint16 slot = ReadUInt16();
		Frame* frame = m_FrameStack.back();
		Value assignValue = m_Stack.back().Clone(this, m_StackAllocator);

		m_Stack.pop_back();
		frame->DeclareLocal(slot, assignValue);
	} break;
	case OpCode::DECLARE_STACK_ARRAY: {
		uint16 type = ReadUInt16();
		uint8 elementPointerLevel = ReadUInt8();
		uint8 numDimensions = ReadUInt8();
		uint32 initializerCount = ReadUInt32();
		uint16 slot = ReadUInt16();
		for (uint32 i = 0; i < numDimensions; i++)
			m_Dimensions[i] = ReadUInt32();

		uint64 typeSize = GetTypeSize(type);
		Value array = Value::MakeArray(this, type, elementPointerLevel, m_Dimensions, numDimensions, m_StackAllocator);
		for (uint32 i = 0; i < initializerCount; i++)
		{
			Value assignValue = m_Stack.back();
			m_Stack.pop_back();
			array.AssignOffset(assignValue, type, elementPointerLevel, typeSize, i * typeSize);
		}

		m_FrameStack.back()->DeclareLocal(slot, array);
	} break;
	case OpCode::DECLARE_OBJECT_WITH_CONSTRUCTOR: {
		uint16 type = ReadUInt16();
		uint16 functionID = ReadUInt16();
		uint16 slot = ReadUInt16();

		Value object = Value::MakeObject(this, type, m_StackAllocator);
		m_FrameStack.back()->DeclareLocal(slot, object);
	} break;
	case OpCode::DECLARE_OBJECT_WITH_ASSIGN: {
		uint16 type = ReadUInt16();
		uint16 slot = ReadUInt16();

		Value assignValue = m_Stack.back();
		m_Stack.pop_back();

		Value object = Value::MakeObject(this, type, m_StackAllocator);
		uint64 size = GetClass(type)->GetSize();
		object.Assign(assignValue, size);
		m_FrameStack.back()->DeclareLocal(slot, object);
	} break;
	case OpCode::DECLARE_REFERENCE: {
		uint16 slot = ReadUInt16();

		Value assignValue = m_Stack.back();
		m_Stack.pop_back();

		Value reference = Value::MakeReference(assignValue, m_StackAllocator);
		m_FrameStack.back()->DeclareLocal(slot, reference);
	} break;
	case OpCode::SET: {
		Value variable = m_Stack.back(); m_Stack.pop_back();
		Value assignValue = m_Stack.back(); m_Stack.pop_back();
		variable.Assign(assignValue, GetTypeSize(variable.type));
	} break;
	case OpCode::MODULE_CONSTANT: {
		uint16 moduleID = ReadUInt16();
		uint16 constantID = ReadUInt16();
		ExecuteModuleConstant(moduleID, constantID);
	} break;
	case OpCode::MODULE_FUNCTION_CALL: {
		uint16 moduleID = ReadUInt16();
		uint16 functionID = ReadUInt16();
		uint8 argCount = ReadUInt8();
		bool usesReturnValue = ReadUInt8();

		m_ArgStorage.clear();
		for (uint32 i = 0; i < argCount; i++)
		{
			m_ArgStorage.push_back(m_Stack.back());
			m_Stack.pop_back();
		}

		ExecuteModuleFunctionCall(moduleID, functionID, usesReturnValue);
	} break;
	case OpCode::STATIC_FUNCTION_CALL: {
		uint16 classID = ReadUInt16();
		uint16 functionID = ReadUInt16();
		bool usesReturnValue = ReadUInt8();

		Function* function = GetClass(classID)->GetFunction(functionID);

		CallFrame callFrame;
		callFrame.basePointer = m_Stack.size();
		callFrame.popThisStack = false;
		callFrame.returnPC = m_ProgramCounter;
		callFrame.usesReturnValue = usesReturnValue;
		callFrame.loopCount = m_LoopStack.size();

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();

		Frame* frame = m_FramePool.Acquire(function->numLocals);
		AddFunctionArgsToFrame(frame, function);

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = function->pc;
	} break;
	case OpCode::RETURN: {
		uint8 returnInfo = ReadUInt8();
		Frame* frame = m_FrameStack.back(); m_FrameStack.pop_back();
		CallFrame& callFrame = m_CallStack.back(); m_CallStack.pop_back();
		
		for (int32 i = m_LoopStack.size() - 1; i >= (int32)callFrame.loopCount; i--)
		{
			const LoopFrame& loop = m_LoopStack[i];
			ScopeInfo& scope = m_ScopeStack[m_CurrentScope--];
			for (uint32 j = 0; j < scope.objects.size(); j++) //TODO: Call destructors
			{

			}
		}

		Value returnValue = Value::MakeNULL();
		uint64 returnMarker = m_ReturnAllocator->GetMarker();
		if (returnInfo == 1) //Returns value
		{
			if (callFrame.usesReturnValue)
			{
				returnValue = m_Stack.back().Actual().Clone(this, m_ReturnAllocator);
			}

			m_Stack.pop_back();
		}
		else if (returnInfo == 2)//Returns reference
		{
			returnValue = m_Stack.back();
			m_Stack.pop_back();
		}

		m_LoopStack.resize(callFrame.loopCount);

		ScopeInfo& scope = m_ScopeStack[m_CurrentScope--];

		//TODO: Call destructors
		m_StackAllocator->FreeToMarker(scope.marker);

		if (returnValue.type != INVALID_ID)
		{
			if(returnInfo == 2)
			{
				m_Stack.push_back(returnValue);
			}
			else
			{
				Value value = returnValue.Clone(this, m_StackAllocator);
				m_ReturnAllocator->FreeToMarker(returnMarker);
				m_Stack.push_back(value);
			}
		}

		m_ProgramCounter = callFrame.returnPC;
	} break;
	case OpCode::MEMBER_FUNCTION_CALL: {
		uint16 classID = ReadUInt16();
		uint16 functionID = ReadUInt16();
		bool usesReturnValue = ReadUInt8();

		Class* cls = GetClass(classID);
		Function* function = cls->GetFunction(functionID);

		Value objToCallFunctionOn = m_Stack.back(); m_Stack.pop_back();

		CallFrame callFrame;
		callFrame.basePointer = m_Stack.size();
		callFrame.popThisStack = true;
		callFrame.returnPC = m_ProgramCounter;
		callFrame.usesReturnValue = usesReturnValue;
		callFrame.loopCount = m_LoopStack.size();

		Frame* frame = m_FramePool.Acquire(function->numLocals);
		AddFunctionArgsToFrame(frame, function);

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();

		m_ThisStack.push_back(Value::MakePointer(classID, 1, objToCallFunctionOn.data, m_StackAllocator));

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = function->pc;
	} break;
	case OpCode::ADDRESS_OF: {
		Value value = m_Stack.back();
		m_Stack.pop_back();
		Value pointer = Value::MakePointer(value.type, value.pointerLevel + 1, value.data, m_StackAllocator);
		m_Stack.push_back(pointer);
	} break;
	case OpCode::DEREFERENCE: {
		Value pointer = m_Stack.back();
		m_Stack.pop_back();
		Value value = pointer.Dereference();
		m_Stack.push_back(value);
	} break;
	case OpCode::ADD: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.Add(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::SUBTRACT: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.Sub(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::MULTIPLY: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.Mul(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::DIVIDE: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.Div(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::MOD: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.Mod(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::LESS: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.LessThan(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::GREATER: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.GreaterThan(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::LESS_EQUAL: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.LessThanOrEqual(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::GREATER_EQUAL: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.GreaterThanOrEqual(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::EQUALS: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.Equals(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::NOT_EQUALS: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.NotEquals(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::LOGICAL_AND: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.LogicalAnd(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::LOGICAL_OR: {
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.LogicalOr(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::PUSH_SCOPE: {
		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
	} break;
	case OpCode::POP_SCOPE: {
		ScopeInfo* scope = &m_ScopeStack[m_CurrentScope];
		//TOOD: Call destructors
		m_StackAllocator->FreeToMarker(scope->marker);
		m_CurrentScope--;
	} break;
	case OpCode::PUSH_LOOP: {
		LoopFrame loop;
		loop.startPC = ReadUInt32();
		loop.endPC = ReadUInt32();
		loop.scopeCount = m_CurrentScope;
		m_LoopStack.push_back(loop);
	} break;
	case OpCode::POP_LOOP: {
		m_LoopStack.pop_back();
	} break;
	case OpCode::UNARY_UPDATE: {
		uint8 type = ReadUInt8();
		bool pushToStack = ReadUInt8();
		switch (type)
		{
		case 0: { //Pre-inc
			Value value = m_Stack.back();
			value.Increment();
			if (!pushToStack)
				m_Stack.pop_back();
		} break;
		case 1: { //Pre-dec
			Value value = m_Stack.back();
			value.Decrement();
			if (!pushToStack)
				m_Stack.pop_back();
		} break;
		case 2: { //Post-inc
			Value value = m_Stack.back(); m_Stack.pop_back();
			Value clone = pushToStack ? value.Clone(this, m_StackAllocator) : Value::MakeNULL();
			value.Increment();
			if (pushToStack)
				m_Stack.push_back(clone);
		} break;
		case 3: { //Post-dec
			Value value = m_Stack.back();
			Value clone = pushToStack ? value.Clone(this, m_StackAllocator) : Value::MakeNULL();
			value.Decrement();
			if (pushToStack)
				m_Stack.push_back(clone);
		} break;
		}
	} break;
	case OpCode::BREAK: {
		LoopFrame loop = m_LoopStack.back();

		for (uint32 i = loop.scopeCount + 1; i < m_CurrentScope; i++)
		{
			for (uint32 j = 0; j < m_ScopeStack[i].objects.size(); j++) //TODO: Call destructors
			{

			}

			m_StackAllocator->FreeToMarker(m_ScopeStack[i].marker);
		}

		m_CurrentScope = loop.scopeCount + 1;
		m_ProgramCounter = loop.endPC;
	} break;
	case OpCode::CONTINUE: {
		LoopFrame loop = m_LoopStack.back();
		
		for (uint32 i = loop.scopeCount; i < m_ScopeStack.size(); i++)
		{
			for (uint32 j = 0; j < m_ScopeStack[i].objects.size(); j++) //TODO: Call destructors
			{

			}

			m_StackAllocator->FreeToMarker(m_ScopeStack[i].marker);
		}

		m_ScopeStack.resize(loop.scopeCount + 1);
		m_ProgramCounter = loop.startPC;
	} break;
	}
}

void Program::ExecuteModuleFunctionCall(uint16 moduleID, uint16 function, bool usesReturnValue)
{
	Value value = Value::MakeNULL();
	switch (moduleID)
	{
	case IO_MODULE_ID: value = IOModule::CallFunction(this, function, m_ArgStorage); break;
	}

	if (value.type != INVALID_ID && usesReturnValue)
		m_Stack.push_back(value);
}

void Program::ExecuteModuleConstant(uint16 moduleID, uint16 constant)
{
	switch (moduleID)
	{
	case IO_MODULE_ID: m_Stack.push_back(IOModule::Constant(this, constant)); break;
	}
}

void Program::AddFunctionArgsToFrame(Frame* frame, Function* function)
{
	for (int32 i = function->parameters.size() - 1; i >= 0; i--)
	{
		const FunctionParameter& param = function->parameters[i];
		Value arg = m_Stack.back(); m_Stack.pop_back();
		if (!param.isReference)
		{
			arg = arg.Clone(this, m_StackAllocator);
		}
		
		if (arg.type != param.type.type)
		{
			if (arg.isReference)
			{
				throw std::runtime_error("Reference type mismatch");
			}

			arg = arg.CastTo(this, param.type.type, param.type.pointerLevel, m_StackAllocator);
		}

		frame->DeclareLocal(param.variableID, arg);
	}
}

void Program::CleanUpForExecution()
{
	for (uint32 i = 0; i < m_CreatedExpressions.size(); i++)
	{
		delete m_CreatedExpressions[i];
	}

	m_CreatedExpressions.reserve(0);
	m_InitializationAllocator->Destroy();
}

void Program::InitStatics()
{
	for (uint32 i = 0; i < m_Classes.size(); i++)
	{
		m_Classes[i]->InitStaticData(this);
	}
}

uint64 Program::ReadUInt64()
{
	uint64* value = (uint64*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(uint64);
	return *value;
}

uint32 Program::ReadUInt32()
{
	uint32* value = (uint32*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(uint32);
	return *value;
}

uint16 Program::ReadUInt16()
{
	uint16* value = (uint16*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(uint16);
	return *value;
}

uint8 Program::ReadUInt8()
{
	return m_Code[m_ProgramCounter++];
}

int8 Program::ReadInt8()
{
	return static_cast<int8>(m_Code[m_ProgramCounter++]);
}

int16 Program::ReadInt16()
{
	int16* value = (int16*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(int16);
	return *value;
}

int32 Program::ReadInt32()
{
	int32* value = (int32*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(int32);
	return *value;
}

int64 Program::ReadInt64()
{
	int64* value = (int64*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(int64);
	return *value;
}

real32 Program::ReadReal32()
{
	real32* value = (real32*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(real32);
	return *value;
}

real64 Program::ReadReal64()
{
	real64* value = (real64*)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(real64);
	return *value;
}

OpCode Program::ReadOPCode()
{
	return (OpCode)ReadUInt16();
}

char* Program::ReadCStr()
{
	char* value = *(char**)&m_Code[m_ProgramCounter];
	m_ProgramCounter += sizeof(char*);
	return value;
}



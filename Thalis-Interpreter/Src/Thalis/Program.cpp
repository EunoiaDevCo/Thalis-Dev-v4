#include "Program.h"
#include "Class.h"
#include "ASTExpression.h"
#include "Modules/ModuleID.h"
#include "Modules/IOModule.h"
#include "Modules/MathModule.h"
#include "Modules/WindowModule.h"
#include "Modules/GLModule.h"
#include "Modules/FSModule.h"
#include "Modules/MemModule.h"
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

void Program::AddPushIndexedCommand(uint64 typeSize, uint8 numIndices, uint16 indexFunctionID, uint16 classID)
{
	WriteOPCode(OpCode::PUSH_INDEXED);
	WriteUInt64(typeSize);
	WriteUInt8(numIndices);
	WriteUInt16(indexFunctionID);
	if (indexFunctionID != INVALID_ID)
		WriteUInt16(classID);
}

void Program::AddPushStaticVariableCommand(uint16 classID, uint64 offset, uint16 type, uint8 pointerLevel, bool isReference, bool isArray)
{
	WriteOPCode(OpCode::PUSH_STATIC_VARIABLE);
	WriteUInt16(classID);
	WriteUInt64(offset);
	WriteUInt16(type);
	WriteUInt8(pointerLevel);
	WriteUInt8(isReference);
	WriteUInt8(isArray);
}

void Program::AddPushMemberCommand(uint16 type, uint8 pointerLevel, uint64 offset, bool isReference, bool isArray)
{
	WriteOPCode(OpCode::PUSH_MEMBER);
	WriteUInt16(type);
	WriteUInt8(pointerLevel);
	WriteUInt64(offset);
	WriteUInt8(isReference);
	WriteUInt8(isArray);
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

void Program::AddSetCommand(uint16 assignFunctionID)
{
	WriteOPCode(OpCode::SET);
	WriteUInt16(assignFunctionID);
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

void Program::AddDeclareObjectWithAssignCommand(uint16 type, uint16 slot, uint16 copyConstructorID)
{
	WriteOPCode(OpCode::DECLARE_OBJECT_WITH_ASSIGN);
	WriteUInt16(type);
	WriteUInt16(slot);
	WriteUInt16(copyConstructorID);
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

void Program::AddConstructorCallCommand(uint16 type, uint16 functionID)
{
	WriteOPCode(OpCode::CONSTRUCTOR_CALL);
	WriteUInt16(type);
	WriteUInt16(functionID);
}

void Program::AddVirtualFunctionCallCommand(uint16 functionID, bool usesReturnValue)
{
	WriteOPCode(OpCode::VIRTUAL_FUNCTION_CALL);
	WriteUInt16(functionID);
	WriteUInt8(usesReturnValue);
}

void Program::AddUnaryUpdateCommand(uint8 op, bool pushToStack)
{
	WriteOPCode(OpCode::UNARY_UPDATE);
	WriteUInt8(op);
	WriteUInt8(pushToStack);
}

void Program::AddAritmaticCommand(Operator op, uint16 functionID)
{
	switch (op)
	{
	case Operator::ADD: WriteOPCode(OpCode::ADD); break;
	case Operator::MINUS: WriteOPCode(OpCode::SUBTRACT); break;
	case Operator::MULTIPLY: WriteOPCode(OpCode::MULTIPLY); break;
	case Operator::DIVIDE: WriteOPCode(OpCode::DIVIDE); break;
	case Operator::MOD: WriteOPCode(OpCode::MOD); break;
	case Operator::EQUALS: WriteOPCode(OpCode::EQUALS); break;
	case Operator::NOT_EQUALS: WriteOPCode(OpCode::NOT_EQUALS); break;
	case Operator::LESS: WriteOPCode(OpCode::LESS); break;
	case Operator::GREATER: WriteOPCode(OpCode::GREATER); break;
	case Operator::LESS_EQUALS: WriteOPCode(OpCode::LESS_EQUAL); break;
	case Operator::GREATER_EQUALS: WriteOPCode(OpCode::GREATER_EQUAL); break;
	}

	WriteUInt16(functionID);
}

void Program::AddNewCommand(uint16 type, uint16 functionID)
{
	WriteOPCode(OpCode::NEW);
	WriteUInt16(type);
	WriteUInt16(functionID);
}

void Program::AddNewArrayCommand(uint16 type, uint8 pointerLevel)
{
	WriteOPCode(OpCode::NEW_ARRAY);
	WriteUInt16(type);
	WriteUInt8(pointerLevel);
}

void Program::AddCastCommand(uint16 targetType, uint8 targetPointerLevel)
{
	WriteOPCode(OpCode::CAST);
	WriteUInt16(targetType);
	WriteUInt8(targetPointerLevel);
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

static ValueType PrimitiveTypeFromName(const std::string& name)
{
	if (name == "uint8")   return ValueType::UINT8;
	if (name == "uint16")  return ValueType::UINT16;
	if (name == "uint32")  return ValueType::UINT32;
	if (name == "uint64")  return ValueType::UINT64;
	if (name == "int8")    return ValueType::INT8;
	if (name == "int16")   return ValueType::INT16;
	if (name == "int32")   return ValueType::INT32;
	if (name == "int64")   return ValueType::INT64;
	if (name == "real32")  return ValueType::REAL32;
	if (name == "real64")  return ValueType::REAL64;
	if (name == "bool")    return ValueType::BOOL;
	if (name == "char")    return ValueType::CHAR;
	if (name == "void")    return ValueType::VOID_T;
	return ValueType::LAST_TYPE;
}

uint16 Program::GetTypeID(const std::string& name)
{
	ValueType primitiveType = PrimitiveTypeFromName(name);
	if (primitiveType != ValueType::LAST_TYPE) return (uint16)primitiveType;

	return GetClassID(name);
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

void Program::BuildVTables()
{
	for (uint32 i = 0; i < m_Classes.size(); i++)
		m_Classes[i]->BuildVTable();
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
	case OpCode::PUSH_UNTYPED_NULL: {
		m_Stack.push_back(Value::MakeNULL());
	} break;
	case OpCode::PUSH_INDEXED: {
		uint64 typeSize = ReadUInt64();
		uint8 numIndices = ReadUInt8();
		uint16 indexFunctionID = ReadUInt16();

		if (indexFunctionID != INVALID_ID)
		{
			uint16 classID = ReadUInt16();
			Class* cls = GetClass(classID);
			Function* function = cls->GetFunction(indexFunctionID);

			CallFrame callFrame;
			callFrame.basePointer = m_Stack.size();
			callFrame.popThisStack = true;
			callFrame.usesReturnValue = true;
			callFrame.loopCount = m_LoopStack.size();

			m_CurrentScope++;
			m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
			callFrame.scopeCount = m_CurrentScope;

			Frame* frame = m_FramePool.Acquire(function->numLocals);
			AddFunctionArgsToFrame(frame, function);

			callFrame.returnPC = m_ProgramCounter;

			Value objToCallFunctionOn = m_Stack.back(); m_Stack.pop_back();
			m_ThisStack.push_back(Value::MakePointer(classID, 1, objToCallFunctionOn.data, m_StackAllocator));

			m_CallStack.push_back(callFrame);
			m_FrameStack.push_back(frame);

			m_ProgramCounter = function->pc;

			break;
		}

		for (uint8 i = 0; i < numIndices; i++)
		{
			m_Dimensions[i] = m_Stack.back().Actual().GetUInt32();
			m_Stack.pop_back();
		}

		Value base = m_Stack.back();
		m_Stack.pop_back();
		
		Value element;
		element.type = base.type;
		element.pointerLevel = base.pointerLevel - 1;
		element.isArray = false;
		element.isReference = false;

		if (base.isArray)
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
			void* ptr = *(void**)base.data;

			uint8 pointerLevel = element.pointerLevel;
			for (uint32 i = 0; i < numIndices; i++)
			{
				if (pointerLevel > 0)
				{
					ptr = (void**)((uint8*)ptr + m_Dimensions[i] * sizeof(void*));
				}
				else
				{
					ptr = (uint8*)ptr + m_Dimensions[i] * typeSize;
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
		bool isArray = ReadUInt8();

		Value value;
		value.type = type;
		value.pointerLevel = pointerLevel;
		value.isArray = isArray;
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
		uint64 offset = ReadUInt64();
		member.isReference = ReadUInt8();
		member.isArray = ReadUInt8();
		member.data = (uint8*)base.data + offset;

		m_Stack.push_back(member);
	} break;
	case OpCode::PUSH_THIS: {
		m_Stack.push_back(m_ThisStack.back());
		uint32 bp = 0;
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

		uint32 elementCount = 1;
		for (uint32 i = 0; i < numDimensions; i++)
		{
			m_Dimensions[i] = ReadUInt32();
			elementCount *= m_Dimensions[i];
		}

		uint64 typeSize = GetTypeSize(type);
		Value array = Value::MakeArray(this, type, elementPointerLevel, m_Dimensions, numDimensions, m_StackAllocator);

		if (!Value::IsPrimitiveType(type))
		{
			uint32 ccount = m_PendingConstructors.size();
			for (uint32 i = 0; i < elementCount; i++)
			{
				Value element;
				element.type = type;
				element.pointerLevel = elementPointerLevel;
				element.isReference = false;
				element.isArray = false;
				element.data = (uint8*)array.data + i * typeSize;

				AddConstructorRecursive(element, true);
			}

			ExecutePendingConstructors(ccount);
		}

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
		m_ScopeStack[m_CurrentScope].objects.push_back(object);

		uint32 ccount = m_PendingConstructors.size();
		AddConstructorRecursive(object);
		ExecutePendingConstructors(ccount);

		if (functionID != INVALID_ID)
		{
			Function* function = GetClass(type)->GetFunction(functionID);

			CallFrame callFrame;
			callFrame.basePointer = m_Stack.size();
			callFrame.popThisStack = true;
			callFrame.usesReturnValue = false;
			callFrame.loopCount = m_LoopStack.size();

			m_CurrentScope++;
			m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
			callFrame.scopeCount = m_CurrentScope;

			Frame* frame = m_FramePool.Acquire(function->numLocals);
			AddFunctionArgsToFrame(frame, function);

			callFrame.returnPC = m_ProgramCounter;

			m_CallStack.push_back(callFrame);
			m_FrameStack.push_back(frame);
			m_ThisStack.push_back(Value::MakePointer(type, 1, object.data, m_StackAllocator));

			m_ProgramCounter = function->pc;
		}
	} break;
	case OpCode::DECLARE_OBJECT_WITH_ASSIGN: {
		uint16 type = ReadUInt16();
		uint16 slot = ReadUInt16();
		uint16 copyConstructorID = ReadUInt16();

		Value assignValue = m_Stack.back();
		m_Stack.pop_back();

		Value object = Value::MakeObject(this, type, m_StackAllocator);
		m_FrameStack.back()->DeclareLocal(slot, object);
		m_ScopeStack[m_CurrentScope].objects.push_back(object);

		uint32 ccount = m_PendingConstructors.size();
		AddConstructorRecursive(object);
		ExecutePendingConstructors(ccount);

		if (copyConstructorID != INVALID_ID)
		{
			Class* cls = GetClass(type);
			Function* copyConstructorFunction = cls->GetFunction(copyConstructorID);

			ExecuteAssignFunction(object, assignValue, copyConstructorFunction);
		}
		else
		{
			uint64 size = GetClass(type)->GetSize();
			object.Assign(assignValue, size);
		}
	} break;
	case OpCode::DECLARE_REFERENCE: {
		uint16 slot = ReadUInt16();

		Value assignValue = m_Stack.back();
		m_Stack.pop_back();

		Value reference = Value::MakeReference(assignValue, m_StackAllocator);
		m_FrameStack.back()->DeclareLocal(slot, reference);
	} break;
	case OpCode::SET: {
		uint16 assignFunctionID = ReadUInt16();
		Value variable = m_Stack.back(); m_Stack.pop_back();
		Value assignValue = m_Stack.back(); m_Stack.pop_back();

		if (assignFunctionID == INVALID_ID)
		{
			if (!variable.IsPrimitive())
			{
				uint32 bp = 0;
			}

			variable.Assign(assignValue, GetTypeSize(variable.type));
		}
		else
		{
			Class* cls = GetClass(variable.type);
			Function* assignFunction = cls->GetFunction(assignFunctionID);

			ExecuteAssignFunction(variable, assignValue, assignFunction);
		}
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
		callFrame.usesReturnValue = usesReturnValue;
		callFrame.loopCount = m_LoopStack.size();

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
		callFrame.scopeCount = m_CurrentScope;

		Frame* frame = m_FramePool.Acquire(function->numLocals);
		AddFunctionArgsToFrame(frame, function);

		callFrame.returnPC = m_ProgramCounter;

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = function->pc;
	} break;
	case OpCode::RETURN: {
		uint8 returnInfo = ReadUInt8();
		Frame* frame = m_FrameStack.back(); m_FrameStack.pop_back();
		CallFrame callFrame = m_CallStack.back(); m_CallStack.pop_back();
		
		if (callFrame.popThisStack)
			m_ThisStack.pop_back();

		uint32 dcount = m_PendingDestructors.size();

		uint64 freeMarker = m_ScopeStack[callFrame.scopeCount].marker;
		for (int32 i = m_CurrentScope; i >= (int32)callFrame.scopeCount; i--)
		{
			ScopeInfo& scope = m_ScopeStack[i];
			for (uint32 j = 0; j < scope.objects.size(); j++)
			{
				AddDestructorRecursive(scope.objects[j]);
			}
			scope.objects.clear();
		}
		m_CurrentScope = callFrame.scopeCount - 1;
		ExecutePendingDestructors(dcount);

		m_LoopStack.resize(callFrame.loopCount);

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

		m_StackAllocator->FreeToMarker(freeMarker);

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
		m_FramePool.Release(frame);
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
		callFrame.usesReturnValue = usesReturnValue;
		callFrame.loopCount = m_LoopStack.size();

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
		callFrame.scopeCount = m_CurrentScope;

		Frame* frame = m_FramePool.Acquire(function->numLocals);
		AddFunctionArgsToFrame(frame, function);

		callFrame.returnPC = m_ProgramCounter;

		m_ThisStack.push_back(Value::MakePointer(classID, 1, objToCallFunctionOn.data, m_StackAllocator));

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = function->pc;
	} break;
	case OpCode::VIRTUAL_FUNCTION_CALL: {
		uint16 functionID = ReadUInt16();
		bool usesReturnValue = ReadUInt8();

		Value objToCallFunctionOn = m_Stack.back(); m_Stack.pop_back();
		m_ThisStack.push_back(objToCallFunctionOn);

		VTable* vtable = *(VTable**)((uint8*)objToCallFunctionOn.data - sizeof(VTable*));
		Function* function = vtable->GetFunction(functionID);

		CallFrame callFrame;
		callFrame.basePointer = m_Stack.size();
		callFrame.popThisStack = true;
		callFrame.usesReturnValue = usesReturnValue;
		callFrame.loopCount = m_LoopStack.size();

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
		callFrame.scopeCount = m_CurrentScope;

		Frame* frame = m_FramePool.Acquire(function->numLocals);
		AddFunctionArgsToFrame(frame, function);

		callFrame.returnPC = m_ProgramCounter;

		m_ThisStack.push_back(Value::MakePointer(objToCallFunctionOn.type, 1, objToCallFunctionOn.data, m_StackAllocator));

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = function->pc;
	} break;
	case OpCode::CONSTRUCTOR_CALL: {
		uint16 type = ReadUInt16();
		uint16 functionID = ReadUInt16();

		Value object = Value::MakeObject(this, type, m_StackAllocator);
		m_ScopeStack[m_CurrentScope].objects.push_back(object);

		uint32 ccount = m_PendingConstructors.size();
		AddConstructorRecursive(object);
		ExecutePendingConstructors(ccount);

		Class* cls = GetClass(type);
		Function* function = cls->GetFunction(functionID);

		CallFrame callFrame;
		callFrame.basePointer = m_Stack.size();
		callFrame.popThisStack = true;
		callFrame.usesReturnValue = false;
		callFrame.loopCount = m_LoopStack.size();

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
		callFrame.scopeCount = m_CurrentScope;

		Frame* frame = m_FramePool.Acquire(function->numLocals);
		AddFunctionArgsToFrame(frame, function);

		callFrame.returnPC = m_ProgramCounter;

		m_ThisStack.push_back(Value::MakePointer(type, 1, object.data, m_StackAllocator));

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = function->pc;

		m_Stack.push_back(object);
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
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (lhs.IsPointer())
		{
			Value val = lhs;
			val.data = (uint8*)lhs.data + rhs.GetUInt64() * GetTypeSize(lhs.type);
			m_Stack.push_back(val);
		}
		else if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.Add(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::SUBTRACT: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (lhs.IsPointer())
		{
			Value val = lhs;
			val.data = (uint8*)lhs.data - rhs.GetUInt64() * GetTypeSize(lhs.type);
			m_Stack.push_back(val);
		}
		else if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.Sub(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::MULTIPLY: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.Mul(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::DIVIDE: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.Div(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::MOD: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.Mod(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::LESS: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.LessThan(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::GREATER: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.GreaterThan(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::LESS_EQUAL: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.LessThanOrEqual(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::GREATER_EQUAL: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.GreaterThanOrEqual(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::EQUALS: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.Equals(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::NOT_EQUALS: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		if (functionID != INVALID_ID)
		{
			Class* cls = GetClass(lhs.type);
			Function* function = cls->GetFunction(functionID);
			ExecuteArithmaticFunction(lhs, rhs, function);
		}
		else
		{
			Value value = lhs.NotEquals(rhs, m_StackAllocator);
			m_Stack.push_back(value);
		}
	} break;
	case OpCode::LOGICAL_AND: {
		uint16 functionID = ReadUInt16();
		Value rhs = m_Stack.back(); m_Stack.pop_back();
		Value lhs = m_Stack.back(); m_Stack.pop_back();
		Value value = lhs.LogicalAnd(rhs, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::LOGICAL_OR: {
		uint16 functionID = ReadUInt16();
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
		uint32 dcount = m_PendingDestructors.size();
		for (uint32 i = 0; i < scope->objects.size(); i++)
		{
			AddDestructorRecursive(scope->objects[i]);
		}
		ExecutePendingDestructors(dcount);
		scope->objects.clear();

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
	case OpCode::NEW: {
		uint16 type = ReadUInt16();
		uint16 functionID = ReadUInt16();
		Value object = Value::MakeObject(this, type, m_HeapAllocator);
		Value pointer = Value::MakePointer(type, 1, object.data, m_StackAllocator);

		uint32 ccount = m_PendingConstructors.size();
		AddConstructorRecursive(object);
		ExecutePendingConstructors(ccount);

		if (functionID != INVALID_ID)
		{
			Function* function = GetClass(type)->GetFunction(functionID);

			CallFrame callFrame;
			callFrame.basePointer = m_Stack.size();
			callFrame.popThisStack = true;
			callFrame.usesReturnValue = false;
			callFrame.loopCount = m_LoopStack.size();

			m_CurrentScope++;
			m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
			callFrame.scopeCount = m_CurrentScope;

			Frame* frame = m_FramePool.Acquire(function->numLocals);
			AddFunctionArgsToFrame(frame, function);

			callFrame.returnPC = m_ProgramCounter;

			m_ThisStack.push_back(pointer);

			m_CallStack.push_back(callFrame);
			m_FrameStack.push_back(frame);

			m_ProgramCounter = function->pc;
		}

		m_Stack.push_back(pointer);
	} break;
	case OpCode::NEW_ARRAY: {
		uint16 type = ReadUInt16();
		uint8 pointerLevel = ReadUInt8();
		uint32 size = m_Stack.back().Actual().GetUInt32();
		m_Stack.pop_back();

		Value array = Value::MakeArray(this, type, pointerLevel, &size, 1, m_HeapAllocator);

		if (!Value::IsPrimitiveType(type))
		{
			uint32 ccount = m_PendingConstructors.size();
			uint64 typeSize = GetTypeSize(type);
			for (uint32 i = 0; i < size; i++)
			{
				Value element;
				element.type = type;
				element.pointerLevel = pointerLevel;
				element.isReference = false;
				element.isArray = false;
				element.data = (uint8*)array.data + i * typeSize;

				AddConstructorRecursive(element, true);
			}

			ExecutePendingConstructors(ccount);
		}

		Value pointer = Value::MakePointer(type, pointerLevel + 1, array.data, m_StackAllocator);
		//pointer.isArray = true;
		m_Stack.push_back(pointer);
	} break;
	case OpCode::DELETE: {
		Value object = m_Stack.back();
		m_Stack.pop_back();

		object = object.Dereference();

		uint32 dcount = m_PendingDestructors.size();
		AddDestructorRecursive(object);
		ExecutePendingDestructors(dcount);

		m_HeapAllocator->Free((uint8*)object.data - sizeof(VTable*));
	} break;
	case OpCode::DELETE_ARRAY: {
		Value heapArray = m_Stack.back().Dereference();
		m_Stack.pop_back();

		ArrayHeader* arrayHeader = (ArrayHeader*)((uint8*)heapArray.data - sizeof(ArrayHeader));
		if (arrayHeader->elementPointerLevel == 0)
		{
			uint32 dcount = m_PendingDestructors.size();
			uint64 typeSize = GetTypeSize(heapArray.type);
			uint32 numElements = 1;
			for (uint32 i = 0; i < arrayHeader->numDimensions; i++)
				numElements *= arrayHeader->dimensions[i];

			for (uint32 i = 0; i < numElements; i++)
			{
				Value element;
				element.type = heapArray.type;
				element.isArray = false;
				element.pointerLevel = 0;
				element.data = (uint8*)heapArray.data + (typeSize * i);
				AddDestructorRecursive(element);
			}

			ExecutePendingDestructors(dcount);
		}

		m_HeapAllocator->Free(arrayHeader);
	} break;
	case OpCode::CAST: {
		uint16 targetType = ReadUInt16();
		uint8 targetPointerLevel = ReadUInt8();
		Value value = m_Stack.back();
		m_Stack.pop_back();

		value = value.CastTo(this, targetType, targetPointerLevel, m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::NEGATE: {
		Value value = m_Stack.back();
		m_Stack.pop_back();
		value = value.Negate(m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::INVERT: {
		Value value = m_Stack.back();
		m_Stack.pop_back();
		value = value.Invert(m_StackAllocator);
		m_Stack.push_back(value);
	} break;
	case OpCode::STRLEN: {
		Value value = m_Stack.back();
		m_Stack.pop_back();
		uint32 length = strlen(value.GetCString());
		m_Stack.push_back(Value::MakeUInt32(length, m_StackAllocator));
	} break;
	case OpCode::PLUS_EQUALS: {
		Value increment = m_Stack.back(); m_Stack.pop_back();
		Value value = m_Stack.back(); m_Stack.pop_back();
		value.PlusEquals(increment);
	} break;
	case OpCode::MINUS_EQUALS: {
		Value increment = m_Stack.back(); m_Stack.pop_back();
		Value value = m_Stack.back(); m_Stack.pop_back();
		value.MinusEquals(increment);
	} break;
	case OpCode::TIMES_EQUALS: {
		Value increment = m_Stack.back(); m_Stack.pop_back();
		Value value = m_Stack.back(); m_Stack.pop_back();
		value.TimesEquals(increment);
	} break;
	case OpCode::DIVIDE_EQUALS: {
		Value increment = m_Stack.back(); m_Stack.pop_back();
		Value value = m_Stack.back(); m_Stack.pop_back();
		value.DivideEquals(increment);
	} break;
	}
}

void Program::ExecuteModuleFunctionCall(uint16 moduleID, uint16 function, bool usesReturnValue)
{
	Value value = Value::MakeNULL();
	switch (moduleID)
	{
	case IO_MODULE_ID: value = IOModule::CallFunction(this, function, m_ArgStorage); break;
	case MATH_MODULE_ID: value = MathModule::CallFunction(this, function, m_ArgStorage); break;
	case WINDOW_MODULE_ID: value = WindowModule::CallFunction(this, function, m_ArgStorage); break;
	case GL_MODULE_ID: value = GLModule::CallFunction(this, function, m_ArgStorage); break;
	case FS_MODULE_ID: value = FSModule::CallFunction(this, function, m_ArgStorage); break;
	case MEM_MODULE_ID: value = MemModule::CallFunction(this, function, m_ArgStorage); break;
	}

	if (value.type != INVALID_ID && usesReturnValue)
		m_Stack.push_back(value);
}

void Program::ExecuteModuleConstant(uint16 moduleID, uint16 constant)
{
	switch (moduleID)
	{
	case IO_MODULE_ID: m_Stack.push_back(IOModule::Constant(this, constant)); break;
	case MATH_MODULE_ID: m_Stack.push_back(MathModule::Constant(this, constant)); break;
	case WINDOW_MODULE_ID: m_Stack.push_back(WindowModule::Constant(this, constant)); break;
	case GL_MODULE_ID: m_Stack.push_back(GLModule::Constant(this, constant)); break;
	case FS_MODULE_ID: m_Stack.push_back(FSModule::Constant(this, constant)); break;
	case MEM_MODULE_ID: m_Stack.push_back(MemModule::Constant(this, constant)); break;
	}
}

void Program::ExecuteAssignFunction(const Value& dstValue, const Value& assignValue, Function* function)
{
	CallFrame callFrame;
	callFrame.basePointer = m_Stack.size();
	callFrame.popThisStack = true;
	callFrame.usesReturnValue = false;
	callFrame.loopCount = m_LoopStack.size();

	m_CurrentScope++;
	m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
	callFrame.scopeCount = m_CurrentScope;

	m_Stack.push_back(assignValue);
	Frame* frame = m_FramePool.Acquire(function->numLocals);
	AddFunctionArgsToFrame(frame, function);

	callFrame.returnPC = m_ProgramCounter;

	m_ThisStack.push_back(Value::MakePointer(dstValue.type, 1, dstValue.data, m_StackAllocator));

	m_CallStack.push_back(callFrame);
	m_FrameStack.push_back(frame);

	m_ProgramCounter = function->pc;

	while (m_ProgramCounter != callFrame.returnPC)
	{
		OpCode innerOpCode = ReadOPCode();
		if (innerOpCode == OpCode::END) break;
		ExecuteOpCode(innerOpCode);
	}
}

void Program::ExecuteArithmaticFunction(const Value& lhs, const Value& rhs, Function* function)
{
	CallFrame callFrame;
	callFrame.basePointer = m_Stack.size();
	callFrame.popThisStack = true;
	callFrame.usesReturnValue = true;
	callFrame.loopCount = m_LoopStack.size();

	m_CurrentScope++;
	m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
	callFrame.scopeCount = m_CurrentScope;

	m_Stack.push_back(rhs);
	Frame* frame = m_FramePool.Acquire(function->numLocals);
	AddFunctionArgsToFrame(frame, function);
	callFrame.returnPC = m_ProgramCounter;

	m_ThisStack.push_back(Value::MakePointer(lhs.type, 1, lhs.data, m_StackAllocator));

	m_CallStack.push_back(callFrame);
	m_FrameStack.push_back(frame);

	m_ProgramCounter = function->pc;
}

void Program::ExecuteCastFunction(const Value& dstValue, const Value& srcValue, Function* function)
{
	CallFrame callFrame;
	callFrame.basePointer = m_Stack.size();
	callFrame.popThisStack = true;
	callFrame.usesReturnValue = false;
	callFrame.loopCount = m_LoopStack.size();

	m_CurrentScope++;
	m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
	callFrame.scopeCount = m_CurrentScope;

	m_Stack.push_back(srcValue);
	Frame* frame = m_FramePool.Acquire(function->numLocals);
	AddFunctionArgsToFrame(frame, function, false);

	callFrame.returnPC = m_ProgramCounter;

	m_ThisStack.push_back(Value::MakePointer(dstValue.type, 1, dstValue.data, m_StackAllocator));

	m_CallStack.push_back(callFrame);
	m_FrameStack.push_back(frame);

	m_ProgramCounter = function->pc;

	while (m_ProgramCounter != callFrame.returnPC)
	{
		OpCode innerOpCode = ReadOPCode();
		if (innerOpCode == OpCode::END) break;
		ExecuteOpCode(innerOpCode);
	}
}

void Program::AddFunctionArgsToFrame(Frame* frame, Function* function, bool readCastFunctionID)
{
	for (int32 i = function->parameters.size() - 1; i >= 0; i--)
	{
		uint16 castFunctionID = INVALID_ID;
		if (readCastFunctionID)
			castFunctionID = ReadUInt16();

		const FunctionParameter& param = function->parameters[i];
		Value arg = m_Stack.back(); m_Stack.pop_back();

		if (castFunctionID != INVALID_ID)
		{
			Class* toClass = GetClass(param.type.type);
			Function* castFunction = toClass->GetFunction(castFunctionID);
			Value original = arg;
			arg = Value::MakeObject(this, param.type.type, m_StackAllocator);
			m_ScopeStack[m_CurrentScope].objects.push_back(arg);
			ExecuteCastFunction(arg, original, castFunction);
		}

		if (!param.isReference)
		{
			if (!Value::IsPrimitiveType(param.type.type) && param.type.pointerLevel == 0)
			{
				Value original = arg;
				Class* cls = GetClass(param.type.type);
				Function* copyConstructor = cls->GetCopyConstructor();
				arg = Value::MakeObject(this, arg.type, m_StackAllocator);
				ExecuteAssignFunction(arg, original, copyConstructor);
			}
			else
			{
				arg = arg.Clone(this, m_StackAllocator);
			}
		}
		
		if (arg.type != param.type.type)
		{
			if (arg.isReference)
			{
				throw std::runtime_error("Reference type mismatch");
			}
			else
			{
				arg = arg.CastTo(this, param.type.type, param.type.pointerLevel, m_StackAllocator);
			}
		}

		frame->DeclareLocal(param.variableID, arg);
	}
}

void Program::AddDestructorRecursive(const Value& value)
{
	if (value.IsPrimitive() || value.IsPointer())
		return;

	Class* cls = GetClass(value.type);
	if (!cls)
		return;

	const std::vector<ClassField>& members = cls->GetMemberFields();
	for (int32 i = (int32)members.size() - 1; i >= 0; i--)
	{
		const ClassField& field = members[i];

		if (Value::IsPrimitiveType(field.type.type) || field.type.pointerLevel > 0)
			continue;

		if (field.numDimensions > 0)
		{
			uint64 typeSize = GetTypeSize(field.type.type);
			uint32 numElements = 1;
			for (uint32 j = 0; j < field.numDimensions; j++)
				numElements *= field.dimensions[j].first;

			uint8* data = (uint8*)value.data + field.offset;
			for (uint32 j = 0; j < numElements; j++)
			{
				Value element;
				element.type = field.type.type;
				element.pointerLevel = 0;
				element.data = data + j * typeSize;

				AddDestructorRecursive(element);
			}
		}
		else
		{
			Value member;
			member.type = field.type.type;
			member.pointerLevel = 0;
			member.data = (uint8*)value.data + field.offset;

			AddDestructorRecursive(member);
		}
	}

	m_PendingDestructors.push_back(value);
}

void Program::ExecutePendingDestructors(uint32 offset)
{
	if (offset == m_PendingDestructors.size()) return;

	for (uint32 i = offset; i < m_PendingDestructors.size(); i++)
	{
		const Value& object = m_PendingDestructors[i];
		Class* cls = GetClass(object.type);
		Function* destructor = cls->GetDestructor();
		if (!destructor)
			continue;

		CallFrame callFrame;
		callFrame.basePointer = m_Stack.size();
		callFrame.popThisStack = true;
		callFrame.usesReturnValue = false;
		callFrame.loopCount = m_LoopStack.size();

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
		callFrame.scopeCount = m_CurrentScope;

		Frame* frame = m_FramePool.Acquire(destructor->numLocals);
		AddFunctionArgsToFrame(frame, destructor);

		callFrame.returnPC = m_ProgramCounter;

		m_ThisStack.push_back(Value::MakePointer(object.type, 1, object.data, m_StackAllocator));

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = destructor->pc;

		while (m_ProgramCounter != callFrame.returnPC)
		{
			OpCode innerOpCode = ReadOPCode();
			if (innerOpCode == OpCode::END) break;
			ExecuteOpCode(innerOpCode);
		}
	}

	m_PendingDestructors.resize(offset);
}

void Program::AddConstructorRecursive(const Value& value, bool addValue)
{
	if (value.IsPrimitive() || value.IsPointer())
		return;

	Class* cls = GetClass(value.type);
	if (!cls)
		return;

	const std::vector<ClassField>& members = cls->GetMemberFields();
	for (int32 i = (int32)members.size() - 1; i >= 0; i--)
	{
		const ClassField& field = members[i];

		if (Value::IsPrimitiveType(field.type.type) || field.type.pointerLevel > 0)
			continue;

		if (field.numDimensions > 0)
		{
			uint64 typeSize = GetTypeSize(field.type.type);
			uint32 numElements = 1;
			for (uint32 j = 0; j < field.numDimensions; j++)
				numElements *= field.dimensions[j].first;

			uint8* data = (uint8*)value.data + field.offset;
			for (uint32 j = 0; j < numElements; j++)
			{
				Value element;
				element.type = field.type.type;
				element.pointerLevel = 0;
				element.data = data + j * typeSize;

				AddConstructorRecursive(element, true);
			}
		}
		else
		{
			Value member;
			member.type = field.type.type;
			member.pointerLevel = 0;
			member.data = (uint8*)value.data + field.offset;

			AddConstructorRecursive(member, true);
		}
	}

	if (cls->HasDefaultConstructor() && addValue)
		m_PendingConstructors.push_back(std::make_pair(value, cls->GetDefaultConstructor()));
}

void Program::ExecutePendingConstructors(uint32 offset)
{
	if (offset == m_PendingConstructors.size()) return;

	for (uint32 i = offset; i < m_PendingConstructors.size(); i++)
	{
		const Value& object = m_PendingConstructors[i].first;
		Function* constructor = m_PendingConstructors[i].second;

		CallFrame callFrame;
		callFrame.basePointer = m_Stack.size();
		callFrame.popThisStack = true;
		callFrame.usesReturnValue = false;
		callFrame.loopCount = m_LoopStack.size();

		m_CurrentScope++;
		m_ScopeStack[m_CurrentScope].marker = m_StackAllocator->GetMarker();
		callFrame.scopeCount = m_CurrentScope;

		Frame* frame = m_FramePool.Acquire(constructor->numLocals);
		AddFunctionArgsToFrame(frame, constructor);

		callFrame.returnPC = m_ProgramCounter;

		m_ThisStack.push_back(Value::MakePointer(object.type, 1, object.data, m_StackAllocator));

		m_CallStack.push_back(callFrame);
		m_FrameStack.push_back(frame);

		m_ProgramCounter = constructor->pc;

		while (m_ProgramCounter != callFrame.returnPC)
		{
			OpCode innerOpCode = ReadOPCode();
			if (innerOpCode == OpCode::END) break;
			ExecuteOpCode(innerOpCode);
		}
	}

	m_PendingConstructors.resize(offset);
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



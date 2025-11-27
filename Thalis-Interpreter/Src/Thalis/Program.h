#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "Value.h"
#include "Memory/BumpAllocator.h"
#include "Memory/HeapAllocator.h"
#include "FramePool.h"
#include "Function.h"
#include "Operator.h"

enum class OpCode
{
	PUSH_UINT8, PUSH_UINT16, PUSH_UINT32, PUSH_UINT64,
	PUSH_INT8, PUSH_INT16, PUSH_INT32, PUSH_INT64,
	PUSH_REAL32, PUSH_REAL64,
	PUSH_CHAR, PUSH_BOOL, PUSH_CSTR, PUSH_LOCAL,
	PUSH_TYPED_NULL, PUSH_INDEXED, PUSH_STATIC_VARIABLE,
	PUSH_MEMBER, PUSH_THIS,

	PUSH_SCOPE, POP_SCOPE, PUSH_LOOP, POP_LOOP,

	DECLARE_UINT8, DECLARE_UINT16, DECLARE_UINT32, DECLARE_UINT64,
	DECLARE_INT8, DECLARE_INT16, DECLARE_INT32, DECLARE_INT64,
	DECLARE_REAL32, DECLARE_REAL64, DECLARE_CHAR, DECLARE_BOOL,
	DECLARE_POINTER, DECLARE_STACK_ARRAY, DECLARE_OBJECT_WITH_CONSTRUCTOR,
	DECLARE_OBJECT_WITH_ASSIGN, DECLARE_REFERENCE,

	ADD, SUBTRACT, MULTIPLY, DIVIDE, MOD,
	LESS, GREATER, LESS_EQUAL, GREATER_EQUAL, EQUALS, NOT_EQUALS,
	UNARY_UPDATE, NOT, NEGATE, LOGICAL_OR, LOGICAL_AND,
	PLUS_EQUALS, MINUS_EQUALS, TIMES_EQUALS, DIVIDE_EQUALS,
	INVERT,

	BREAK, CONTINUE,

	ADDRESS_OF, DEREFERENCE, CAST,

	SET,

	MODULE_CONSTANT, MEMBER_FUNCTION_CALL, CONSTRUCTOR_CALL, VIRTUAL_FUNCTION_CALL,
	MODULE_FUNCTION_CALL, STATIC_FUNCTION_CALL, RETURN, NEW, NEW_ARRAY,

	STRLEN,

	DELETE, DELETE_ARRAY,

	JUMP, JUMP_IF_FALSE,

	END
};

struct CallFrame
{
	uint32 returnPC;      // Where to continue after function returns
	uint32 basePointer;   // Stack index where this function's locals begin
	bool usesReturnValue;
	bool popThisStack;
	uint32 loopCount;
	uint32 scopeCount;
};

struct ScopeInfo
{
	ScopeInfo()
	{
		objects.reserve(64);
	}

	uint64 marker;
	std::vector<Value> objects;
};

struct LoopFrame
{
	uint32 startPC;
	uint32 endPC;
	uint32 scopeCount;
};

class Class;
struct ASTExpression;
class Program
{
public:
	Program();

	void ExecuteProgram(uint32 pc);

	void AddJumpCommand(uint32 pc);
	void AddPushConstantUInt8Command(uint8 value);
	void AddPushConstantUInt16Command(uint16 value);
	void AddPushConstantUInt32Command(uint32 value);
	void AddPushConstantUInt64Command(uint64 value);
	void AddPushConstantInt8Command(int8 value);
	void AddPushConstantInt16Command(int16 value);
	void AddPushConstantInt32Command(int32 value);
	void AddPushConstantInt64Command(int64 value);
	void AddPushConstantReal32Command(real32 value);
	void AddPushConstantReal64Command(real64 value);
	void AddPushConstantCharCommand(char value);
	void AddPushConstantBoolCommand(bool value);
	void AddPushCStrCommand(char* value);
	void AddPushLocalCommand(uint16 slot);
	void AddPushTypedNullCommand(uint16 type, uint8 pointerLevel);
	void AddPushIndexedCommand(uint64 typeSize, uint8 numIndices, uint16 indexFunctionID, uint16 classID);
	void AddPushStaticVariableCommand(uint16 classID, uint64 offset, uint16 type, uint8 pointerLevel, bool isReference, bool isArray);
	void AddPushMemberCommand(uint16 type, uint8 pointerLevel, uint64 offset, bool isReference, bool isArray);

	uint32 AddPushLoopCommand();
	void AddPopLoopCommand();

	void AddSetCommand(uint16 assignFunctionID);

	void AddDeclarePrimitiveCommand(ValueType type, uint16 slot);
	void AddDeclarePointerCommand(uint16 type, uint8 pointerLevel, uint16 slot);
	void AddDeclareStackArrayCommand(uint16 type, uint8 elementPointerLevel, uint32* dimensions, uint8 numDimensions, uint32 initializerCount, uint16 slot);
	void AddDeclareObjectWithConstructorCommand(uint16 type, uint16 functionID, uint16 slot);
	void AddDeclareObjectWithAssignCommand(uint16 type, uint16 slot, uint16 copyConstructorID);
	void AddDeclareReferenceCommand(uint16 slot);

	void AddModuleConstantCommand(uint16 moduleID, uint16 constant);
	void AddModuleFunctionCallCommand(uint16 moduleID, uint16 functionID, uint8 argCount, bool usesReturnValue);
	void AddStaticFunctionCallCommand(uint16 classID, uint16 functionID, bool usesReturnValue);
	void AddReturnCommand(uint8 returnInfo);
	void AddMemberFunctionCallCommand(uint16 classID, uint16 functionID, bool usesReturnValue);
	void AddConstructorCallCommand(uint16 type, uint16 functionID);
	void AddVirtualFunctionCallCommand(uint16 functionID, bool usesReturnValue);

	void AddUnaryUpdateCommand(uint8 op, bool pushToStack);
	void AddAritmaticCommand(Operator op, uint16 functionID);

	void AddNewCommand(uint16 type, uint16 functionID);
	void AddNewArrayCommand(uint16 type, uint8 pointerLevel);

	void AddCastCommand(uint16 targetType, uint8 targetPointerLevel);

	void WriteUInt64(uint64 value);
	void WriteUInt32(uint32 value);
	void WriteUInt16(uint16 value);
	void WriteUInt8(uint8 value);
	void WriteInt8(int8 value);
	void WriteInt16(int16 value);
	void WriteInt32(int32 value);
	void WriteInt64(int64 value);
	void WriteReal32(real32 value);
	void WriteReal64(real64 value);
	void WriteOPCode(OpCode code);
	void WriteCStr(char* cstr);

	void PatchUInt32(uint32 pos, uint32 value);
	void PatchPushLoopCommand(uint32 pos, uint32 start, uint32 end);

	uint16 AddClass(Class* cls);
	uint16 GetClassID(const std::string& name);
	void SetClassWithMainFunction(uint16 id);
	inline uint16 GetClassIDWithMainFunction() const { return m_ClassWithMainFunction; }
	std::string GetTypeName(uint16 type);
	Class* GetClass(uint16 id);
	Class* GetClassByName(const std::string& name);
	uint16 GetModuleID(const std::string& name);
	void AddModule(const std::string& name, uint16 id);
	uint64 GetTypeSize(uint16 type);
	uint16 GetTypeID(const std::string& name);

	uint32 GetCodeSize() const;

	bool Resolve();
	void BuildVTables();
	void EmitCode();

	inline BumpAllocator* GetStackAllocator() const { return m_StackAllocator; }
	inline HeapAllocator* GetHeapAllocator() const { return m_HeapAllocator; }
	inline BumpAllocator* GetInitializationAllocator() const { return m_InitializationAllocator; }

	inline uint32 GetStackSize() const { return m_Stack.size(); }
	inline uint32 GetScopeStackSize() const { return m_CurrentScope + 1; }
	inline uint32 GetLoopStackSize() const { return m_LoopStack.size(); }

	inline void AddToStringPool(char* str) { m_StringPool.push_back(str); }
	inline void AddCreatedExpression(ASTExpression* expr) { m_CreatedExpressions.push_back(expr); }

	inline Frame* GetFrame(uint32 frameIndex) { return m_FrameStack[frameIndex]; }
public:
	static Program* GetCompiledProgram();
private:
	void ExecuteOpCode(OpCode opcode);
	void ExecuteModuleFunctionCall(uint16 moduleID, uint16 functionID, bool usesReturnValue);
	void ExecuteModuleConstant(uint16 moduleID, uint16 constant);
	void ExecuteAssignFunction(const Value& dstValue, const Value& assignValue, Function* function);
	void ExecuteArithmaticFunction(const Value& lhs, const Value& rhs, Function* function);
	void ExecuteCastFunction(const Value& dstValue, const Value& srcValue, Function* function);

	void AddFunctionArgsToFrame(Frame* frame, Function* function, bool readCastFunctionID = true);

	void AddDestructorRecursive(const Value& value);
	void ExecutePendingDestructors(uint32 offset);

	void CleanUpForExecution();
	void InitStatics();

	uint64 ReadUInt64();
	uint32 ReadUInt32();
	uint16 ReadUInt16();
	uint8 ReadUInt8();
	int8 ReadInt8();
	int16 ReadInt16();
	int32 ReadInt32();
	int64 ReadInt64();
	real32 ReadReal32();
	real64 ReadReal64();
	OpCode ReadOPCode();
	char* ReadCStr();
private:
	std::vector<Class*> m_Classes;
	std::unordered_map<std::string, uint16> m_ClassNameMap;
	std::unordered_map<std::string, uint16> m_ModuleNameMap;
	uint16 m_ClassWithMainFunction;

	std::vector<Value> m_Stack;
	std::vector<uint8> m_Code;
	uint32 m_ProgramCounter;

	std::vector<Value> m_ArgStorage;

	FramePool m_FramePool;
	std::vector<Frame*> m_FrameStack;
	std::vector<CallFrame> m_CallStack;
	std::vector<ScopeInfo> m_ScopeStack;
	int32 m_CurrentScope;
	std::vector<LoopFrame> m_LoopStack;
	std::vector<Value> m_ThisStack;

	std::vector<char*> m_StringPool;
	uint32 m_Dimensions[MAX_ARRAY_DIMENSIONS];

	BumpAllocator* m_StackAllocator;
	HeapAllocator* m_HeapAllocator;
	BumpAllocator* m_InitializationAllocator;
	BumpAllocator* m_ReturnAllocator;

	std::vector<Value> m_PendingDestructors;

	std::vector<ASTExpression*> m_CreatedExpressions;
};
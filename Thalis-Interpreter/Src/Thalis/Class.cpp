#include "Class.h"
#include "Program.h"
#include "ASTExpression.h"

std::string Class::GetName() const
{
    return m_Name;
}

void Class::AddFunction(Function* function)
{
	m_Functions[function->name].push_back(function);

	std::string signature = function->GenerateSignature();

	if (m_FunctionDefinitionMap.find(signature) == m_FunctionDefinitionMap.end())
	{
		uint32 functionID = m_NextFunctionID++;
		m_FunctionDefinitionMap[signature] = functionID;
		m_FunctionMap.push_back(function);
		function->id = functionID;
	}
}

Function* Class::GetFunction(uint16 id)
{
	return m_FunctionMap[id];
}

static int32 GetConversionScore(Program* program, const TypeInfo& from, const TypeInfo& to)
{
	if (from.pointerLevel != to.pointerLevel)
		return -1; // pointer mismatch not allowed

	if (from.type == to.type)
		return 0; // exact match

	if (!Value::IsPrimitiveType(from.type) && !Value::IsPrimitiveType(to.type))
	{
		
	}

	bool fromIsInt = Value::IsIntegerType(from.type);
	bool toIsInt = Value::IsIntegerType(to.type);
	bool fromIsReal = Value::IsRealType(from.type);
	bool toIsReal = Value::IsRealType(to.type);

	// integer -> integer (safe or narrowing)
	if (fromIsInt && toIsInt)
	{
		// Slightly penalize narrowing (e.g. int64 -> int32)
		if (program->GetTypeSize(from.type) > program->GetTypeSize(to.type))
			return 2; // narrowing
		else
			return 1; // widening or same range
	}

	// float -> float (e.g. real64 -> real32)
	if (fromIsReal && toIsReal)
	{
		if (program->GetTypeSize(from.type) > program->GetTypeSize(to.type))
			return 2; // narrowing float
		else
			return 1; // widening float
	}

	// int -> float
	if (fromIsInt && toIsReal)
		return 3; // less ideal but allowed

	// float -> int
	if (fromIsReal && toIsInt)
		return 4; // least ideal primitive cast

	return -1; // incompatible
}

uint16 Class::GetFunctionID(const std::string& name, const std::vector<ASTExpression*>& args)
{
	Program* program = Program::GetCompiledProgram();

	// First try exact signature match
	std::string exactSignature = Function::GenerateSignatureFromArgs(program, name, args);
	auto it = m_FunctionDefinitionMap.find(exactSignature);
	if (it != m_FunctionDefinitionMap.end())
		return it->second;

	Function* bestFunc = nullptr;
	uint32 bestID = INVALID_ID;
	int bestScore = INT_MAX;

	// Now try to find the best compatible function
	for (const auto& [sig, id] : m_FunctionDefinitionMap)
	{
		// Check if signature starts with "<name>-"
		if (sig.rfind(name + "-", 0) != 0)
			continue;

		Function* func = FindFunctionBySignature(sig);
		if (!func || func->parameters.size() != args.size())
			continue;

		int totalScore = 0;
		bool compatible = true;

		for (size_t i = 0; i < args.size(); ++i)
		{
			ASTExpression* arg = args[i];
			TypeInfo argType = arg->GetTypeInfo(program);
			const FunctionParameter& param = func->parameters[i];

			int score = GetConversionScore(program, argType, param.type);
			if (score < 0) { compatible = false; break; }
			totalScore += score;
		}

		if (compatible && totalScore < bestScore)
		{
			bestFunc = func;
			bestID = id;
			bestScore = totalScore;
		}
	}

	if (bestFunc)
		return bestID;

	// No match found
	return INVALID_ID;
}

Function* Class::FindFunctionBySignature(const std::string& signature)
{
	// Extract the function name before the first '-'
	size_t dashPos = signature.find('-');
	std::string name = (dashPos == std::string::npos) ? signature : signature.substr(0, dashPos);

	// Look up all overloads for this function name
	const auto it = m_Functions.find(name);
	if (it == m_Functions.end())
		return nullptr;

	// Search for a function with an exact matching signature
	for (Function* func : it->second)
	{
		std::string funcSig = func->GenerateSignature();
		if (funcSig == signature)
			return func;
	}

	return nullptr;
}

void Class::AddMemberField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, uint32* dimensions, uint8 numDimensions)
{
	ClassField field;
	field.name = name;
	field.type.type = type;
	field.type.pointerLevel = pointerLevel;
	field.offset = offset;
	field.size = size;
	field.numDimensions = numDimensions;
	for (uint32 i = 0; i < numDimensions; i++)
		field.dimensions[i] = dimensions[i];

	m_MemberFields.push_back(field);
}

void Class::AddStaticField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, uint32* dimensions, uint8 numDimensions, ASTExpression* initializeExpr)
{
	ClassField field;
	field.name = name;
	field.type.type = type;
	field.type.pointerLevel = pointerLevel;
	field.offset = offset;
	field.size = size;
	field.initializeExpr = initializeExpr;
	field.numDimensions = numDimensions;
	for (uint32 i = 0; i < numDimensions; i++)
		field.dimensions[i] = dimensions[i];

	m_StaticFields.push_back(field);
}

void Class::EmitCode(Program* program)
{
	for (uint32 i = 0; i < m_FunctionMap.size(); i++)
	{
		Function* function = m_FunctionMap[i];
		function->pc = program->GetCodeSize();
		for (uint32 i = 0; i < function->body.size(); i++)
		{
			function->body[i]->EmitCode(program);
		}
		if (function->returnInfo.type == (uint16)ValueType::VOID_T)
		{
			program->AddReturnCommand(false);
		}
	}
}

void Class::InitStaticData(Program* program)
{
	m_StaticData.data = program->GetStackAllocator()->Alloc(m_StaticData.size);
	memset(m_StaticData.data, 0, m_StaticData.size);

	for (uint32 i = 0; i < m_StaticFields.size(); i++)
	{
		const ClassField& field = m_StaticFields[i];
		if (field.initializeExpr)
		{
			ASTExpressionStaticVariable* variableExpr = new ASTExpressionStaticVariable(m_ID, field.offset, field.type);
			ASTExpressionSet* setExpr = new ASTExpressionSet(variableExpr, field.initializeExpr);
			setExpr->EmitCode(program);
		}
	}
}

uint64 Class::CalculateMemberOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo, uint32 currentMember, uint32 currentOffset)
{
	for (uint32 i = currentMember; i < members.size(); i++)
	{
		for (uint32 j = 0; j < m_MemberFields.size(); j++)
		{
			const ClassField& member = m_MemberFields[j];

			if (members[i] == member.name)
			{
				uint64 memberOffset = member.offset + currentOffset;

				// If it's primitive or the last field, return the offset
				if (Value::IsPrimitiveType(member.type.type) || (i + 1) == members.size())
				{
					*typeInfo = member.type;
					
					return memberOffset;
				}

				// Recurse into sub-class
				Class* subClass = Program::GetCompiledProgram()->GetClass(member.type.type);
				if (!subClass)
					throw std::runtime_error("Invalid class type for member: " + member.name);

				return subClass->CalculateMemberOffset(program, members, typeInfo, i + 1, memberOffset);
			}
		}
	}

	// No member found
	return UINT64_MAX;
}

uint64 Class::CalculateStaticOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo)
{
	if (members.size() == 1)
	{
		for (uint32 i = 0; i < m_StaticFields.size(); i++)
		{
			const ClassField& field = m_StaticFields[i];
			if (members[0] == field.name) 
			{
				*typeInfo = field.type;
				return field.offset;
			}
		}

		return UINT64_MAX;
	}
	else
	{
		uint64 baseOffset = UINT64_MAX;
		uint16 type = INVALID_ID;
		for (uint32 i = 0; i < m_StaticFields.size(); i++)
		{
			const ClassField& field = m_StaticFields[i];
			if (members[0] == field.name)
			{
				baseOffset = field.offset;
				type = field.type.type;
				break;
			}
		}

		if (type == INVALID_ID) return UINT64_MAX;

		return baseOffset + program->GetClass(type)->CalculateMemberOffset(program, members, typeInfo, 1);
	}

	return UINT64_MAX;
}

void* Class::GetStaticData(uint64 offset) const
{
	return (uint8*)m_StaticData.data + offset;
}

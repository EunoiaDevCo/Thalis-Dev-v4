#include "VTable.h"
#include "Function.h"
#include "Value.h"
#include "Class.h"
#include "Program.h"
#include "ASTExpression.h"

VTable& VTable::operator=(const VTable& other)
{
	if (this != &other)
	{
		functions = other.functions;
	}
	return *this;
}

static int32 GetConversionScore(Program* program, const TypeInfo& from, const TypeInfo& to, uint16* castFunctionID)
{
	if (from.pointerLevel != to.pointerLevel)
		return -1; // pointer mismatch not allowed

	if (from.type == to.type)
		return 0; // exact match

	if (!Value::IsPrimitiveType(from.type) && !Value::IsPrimitiveType(to.type))
	{
		Class* fromClass = program->GetClass(from.type);
		if (fromClass->InheritsFrom(to.type)) return 1;
	}

	if (!Value::IsPrimitiveType(to.type) && to.pointerLevel == 0)
	{
		Class* toClass = program->GetClass(to.type);
		std::vector<ASTExpression*> args;
		args.push_back(new ASTExpressionDummy(from));
		std::vector<uint16> cf;
		uint16 functionID = toClass->GetFunctionID(toClass->GetName(), args, cf);
		if (functionID == INVALID_ID) return -1;
		*castFunctionID = functionID;
	}
	else
	{
		*castFunctionID = INVALID_ID;
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

int32 VTable::FindSlot(const std::string& name, const std::vector<TypeInfo>& parameters)
{
	Program* program = Program::GetCompiledProgram();

	Function* bestFunc = nullptr;
	int32 bestID = INVALID_ID;
	int bestScore = INT_MAX;

	for (uint32 i = 0; i < functions.size(); i++)
	{
		Function* function = functions[i];
		std::string sig = function->GenerateSignature();

		if (sig.rfind(name + "-", 0) != 0)
			continue;

		if (function->parameters.size() != parameters.size())
			continue;

		int32 totalScore = 0;
		bool compatible = true;

		for (uint32 j = 0; j < function->parameters.size(); j++)
		{
			TypeInfo argType = parameters[j];
			const FunctionParameter& param = function->parameters[j];
			uint16 castFunctionID = INVALID_ID;
			int score = GetConversionScore(program, argType, param.type, &castFunctionID);
			if (score < 0) { compatible = false; break; }
			totalScore += score;
		}

		if (compatible && totalScore < bestScore)
		{
			bestFunc = function;
			bestID = i;
			bestScore = totalScore;
		}
	}

	if (bestFunc)
		return bestID;

	// No match found
	return INVALID_ID;
}

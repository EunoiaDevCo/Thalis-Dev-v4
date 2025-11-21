#pragma once

#include <vector>
#include "TypeInfo.h"
#include <string>

enum class AccessModifier
{
	PUBLIC,
	PRIVATE
};

struct FunctionParameter
{
	TypeInfo type;
	bool isReference;
	uint16 variableID;
};

struct ASTExpression;
class Program;
struct Function
{
	std::string name;
	uint32 pc;
	AccessModifier accessModifier;
	bool isStatic;
	bool isVirtual;
	TypeInfo returnInfo;
	bool returnsReference;
	std::vector<FunctionParameter> parameters;
	std::vector<ASTExpression*> body;
	uint16 id;
	uint16 numLocals;

	std::string GenerateSignature() const;

	static std::string GenerateSignatureFromArgs(Program* program, const std::string& name, const std::vector<ASTExpression*>& args);
};
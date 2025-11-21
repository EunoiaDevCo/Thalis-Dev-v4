#pragma once

#include <string>
#include <vector>
#include "Scope.h"
#include "Tokenizer.h"
#include "Function.h"

class Program;
class Class;
class Scope;
struct ASTExpression;
struct ASTExpressionModuleFunctionCall;
struct ASTExpressionModuleConstant;
class Parser
{
public:
	Parser(Program* program);

	void Parse(const std::string& path);
private:
	bool ParseImport(Tokenizer* tokenizer);
	bool ParseClass(Tokenizer* tokenizer);
	bool ParseFunction(Tokenizer* tokenizer, Class* cls);
	bool ParseClassVariable(Tokenizer* tokenizer, Class* cls, uint64* memberOffset, uint64* staticOffset);
	uint16 ParseType(const Token& token);
	uint8 ParsePointerLevel(Tokenizer* tokenizer);
	bool ParseStatement(Function* function, Tokenizer* tokenizer);
	ASTExpression* ParseExpression(Tokenizer* tokenizer);
	ASTExpression* ParseUnary(Tokenizer* tokenizer);
	ASTExpression* ParseBinaryOpRHS(int32 exprPrec, ASTExpression* lhs, Tokenizer* tokenizer);
	ASTExpression* ParsePostFix(Tokenizer* tokenizer);
	ASTExpression* ParsePrimary(Tokenizer* tokenizer);
	void ParseArguments(Tokenizer* tokenizer, std::vector<ASTExpression*>& args);
	void ParseArrayDimensions(Tokenizer* tokenizer, std::vector<uint32>& dimensions);
	void ParseArrayInitializers(Tokenizer* tokenizer, std::vector<ASTExpression*>& initializers);
	void ParseArrayIndices(Tokenizer* tokenizer, std::vector<ASTExpression*>& indexExprs);
	ASTExpression* ParseExpressionChain(Tokenizer* tokenizer, ASTExpression* objExpr, const std::vector<std::pair<std::string, bool>>& members, bool functionCall);
	void ParseMembers(Tokenizer* tokenizer, std::vector<std::pair<std::string, bool>>& members, bool* functionCall);

	ASTExpressionModuleFunctionCall* MakeModuleFunctionCall(uint16 moduleID, const std::string& moduleName, const std::string& functionCall, const std::vector<ASTExpression*>& args);
	ASTExpressionModuleConstant* MakeModuleConstant(uint16 moduleID, const std::string& moduleName, const std::string& constantName);

	Program* m_Program;
	std::vector<Scope*> m_ScopeStack;

	std::string m_ErrorMessage;

	std::string m_CurrentClassName;
	bool m_CurrentFunctionReturnsReference;
};
#pragma once

#include "Value.h"
#include "TypeInfo.h"
#include <vector>
#include <new>
#include "Operator.h"

class Program;
class Class;
struct ASTExpression
{
	void* operator new(std::size_t size);
	void operator delete(void* ptr) noexcept;

	ASTExpression() : isStatement(false), setIsStatement(true) {}
	virtual void EmitCode(Program* program) = 0;
	virtual TypeInfo GetTypeInfo(Program* program) = 0;
	virtual bool Resolve(Program* program) { return true; }

	bool isStatement;
	bool setIsStatement;
};

struct ASTExpressionLiteral : public ASTExpression
{
	Value value;

	ASTExpressionLiteral(const Value& value) :
		value(value) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionConstUInt32 : public ASTExpression
{
	uint32 value;

	ASTExpressionConstUInt32(uint32 value) :
		value(value) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionModuleFunctionCall : public ASTExpression
{
	uint16 moduleID;
	uint16 functionID;
	std::vector<ASTExpression*> argExprs;

	ASTExpressionModuleFunctionCall(uint16 moduleID, uint16 functionID, const std::vector<ASTExpression*> argExprs) :
		moduleID(moduleID), functionID(functionID), argExprs(argExprs) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionDeclarePrimitive : public ASTExpression
{
	ValueType type;
	uint16 slot;
	ASTExpression* assignExpr;

	ASTExpressionDeclarePrimitive(ValueType type, uint16 slot, ASTExpression* assignExpr = nullptr) :
		type(type), slot(slot), assignExpr(assignExpr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionPushLocal : public ASTExpression
{
	uint16 slot;
	TypeInfo typeInfo;

	ASTExpressionPushLocal(uint16 slot, const TypeInfo& typeInfo) :
		slot(slot), typeInfo(typeInfo) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionDeclarePointer : public ASTExpression
{
	uint16 type;
	uint8 pointerLevel;
	uint16 slot;
	ASTExpression* assignExpr;

	ASTExpressionDeclarePointer(uint16 type, uint8 pointerLevel, uint16 slot, ASTExpression* assignExpr = nullptr) :
		type(type), pointerLevel(pointerLevel), slot(slot), assignExpr(assignExpr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionSet : public ASTExpression
{
	ASTExpression* expr;
	ASTExpression* assignExpr;
	uint16 assignFunctionID;

	ASTExpressionSet(ASTExpression* expr, ASTExpression* assignExpr) :
		expr(expr), assignExpr(assignExpr), assignFunctionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionAddressOf : public ASTExpression
{
	ASTExpression* expr;

	ASTExpressionAddressOf(ASTExpression* expr) :
		expr(expr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionDereference : public ASTExpression
{
	ASTExpression* expr;

	ASTExpressionDereference(ASTExpression* expr) :
		expr(expr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionStackArrayDeclare : public ASTExpression
{
	uint16 type;
	uint8 elementPointerLevel;
	uint16 slot;
	std::vector<uint32> dimensions;
	std::vector<ASTExpression*> initializerExprs;

	ASTExpressionStackArrayDeclare(uint16 type, uint8 elementPointerLevel, uint16 slot, const std::vector<uint32>& dimensions, const std::vector<ASTExpression*>& initializerExprs) :
		type(type), elementPointerLevel(elementPointerLevel), slot(slot), dimensions(dimensions), initializerExprs(initializerExprs) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionPushIndex : public ASTExpression
{
	ASTExpression* expr;
	std::vector<ASTExpression*> indexExprs;

	ASTExpressionPushIndex(ASTExpression* expr, const std::vector<ASTExpression*> indexExprs) :
		expr(expr), indexExprs(indexExprs) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionBinary : public ASTExpression
{
	ASTExpression* lhs;
	ASTExpression* rhs;
	Operator op;
	uint16 functionID;

	ASTExpressionBinary(ASTExpression* lhs, ASTExpression* rhs, Operator op) :
		lhs(lhs), rhs(rhs), op(op), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionIfElse : public ASTExpression
{
	ASTExpression* conditionExpr;
	bool pushIfScope;
	bool pushElseScope;
	std::vector<ASTExpression*> ifExprs;
	std::vector<ASTExpression*> elseExprs;

	ASTExpressionIfElse(ASTExpression* conditionExpr, bool pushIfScope, bool pushElseScope, const std::vector<ASTExpression*>& ifExprs, const std::vector<ASTExpression*>& elseExprs) :
		conditionExpr(conditionExpr), pushIfScope(pushIfScope), pushElseScope(pushElseScope), ifExprs(ifExprs), elseExprs(elseExprs) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionFor : public ASTExpression
{
	ASTExpression* declareExpr;
	ASTExpression* conditionExpr;
	ASTExpression* incrExpr;
	std::vector<ASTExpression*> forExprs;

	ASTExpressionFor(ASTExpression* declareExpr, ASTExpression* conditionExpr, ASTExpression* incrExpr, const std::vector<ASTExpression*>& forExprs) :
		declareExpr(declareExpr), conditionExpr(conditionExpr), incrExpr(incrExpr), forExprs(forExprs){ }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

enum class ASTUnaryUpdateOp
{
	PRE_INC,
	PRE_DEC,
	POST_INC,
	POST_DEC
};

struct ASTExpressionUnaryUpdate : public ASTExpression
{
	ASTExpression* expr;
	ASTUnaryUpdateOp op;

	ASTExpressionUnaryUpdate(ASTExpression* expr, ASTUnaryUpdateOp op) :
		expr(expr), op(op) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionWhile : public ASTExpression
{
	ASTExpression* conditionExpr;
	std::vector<ASTExpression*> whileExprs;

	ASTExpressionWhile(ASTExpression* conditionExpr, const std::vector<ASTExpression*>& whileExprs) :
		conditionExpr(conditionExpr), whileExprs(whileExprs) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionBreak : public ASTExpression
{
	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionContinue : public ASTExpression
{
	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionStaticFunctionCall : public ASTExpression
{
	uint16 classID;
	std::string functionName;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;

	ASTExpressionStaticFunctionCall(uint16 classID, const std::string& functionName, const std::vector<ASTExpression*> argExprs) :
		classID(classID), functionName(functionName), argExprs(argExprs), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionReturn : public ASTExpression
{
	ASTExpression* expr;
	bool returnsReference;

	ASTExpressionReturn(ASTExpression* expr = nullptr, bool returnsReference = false) :
		expr(expr), returnsReference(returnsReference) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionStaticVariable : public ASTExpression
{
	uint16 classID;
	std::vector<std::string> members;
	uint64 offset;
	TypeInfo typeInfo;
	bool isArray;

	ASTExpressionStaticVariable(uint16 classID, const std::vector<std::string>& members) :
		classID(classID), members(members), offset(UINT64_MAX), typeInfo(INVALID_ID, 0), isArray(false) { }

	ASTExpressionStaticVariable(uint16 classID, uint64 offset, const TypeInfo& typeInfo, bool isArray) :
		classID(classID), offset(offset), typeInfo(typeInfo), isArray(isArray) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionModuleConstant : public ASTExpression
{
	uint16 moduleID;
	uint16 constantID;

	ASTExpressionModuleConstant(uint16 moduleID, uint16 constantID) :
		moduleID(moduleID), constantID(constantID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionDeclareObjectWithConstructor : public ASTExpression
{
	uint16 type;
	std::vector<ASTExpression*> argExprs;
	uint16 slot;
	uint16 functionID;

	ASTExpressionDeclareObjectWithConstructor(uint16 type, const std::vector<ASTExpression*> argExprs, uint16 slot) :
		type(type), argExprs(argExprs), slot(slot), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionDeclareObjectWithAssign : public ASTExpression
{
	uint16 type;
	uint16 slot;
	ASTExpression* assignExpr;
	uint16 copyConstructorID;

	ASTExpressionDeclareObjectWithAssign(uint16 type, uint16 slot, ASTExpression* assignExpr) : 
		type(type), slot(slot), assignExpr(assignExpr), copyConstructorID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionPushMember : public ASTExpression
{
	ASTExpression* expr;
	std::vector<std::string> members;
	TypeInfo typeInfo;
	bool isArray;
	uint64 offset;

	ASTExpressionPushMember(ASTExpression* expr, const std::vector<std::string>& members) :
		expr(expr), members(members), typeInfo(INVALID_ID, 0), isArray(false), offset(UINT64_MAX) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionMemberFunctionCall : public ASTExpression
{
	ASTExpression* objExpr;
	std::string functionName;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;

	ASTExpressionMemberFunctionCall(ASTExpression* objExpr, const std::string& functionName, const std::vector<ASTExpression*> argExprs) :
		objExpr(objExpr), functionName(functionName), argExprs(argExprs), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionThis : public ASTExpression
{
	uint16 classID;

	ASTExpressionThis(uint16 classID) :
		classID(classID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionDeclareReference : public ASTExpression
{
	uint16 type;
	uint8 pointerLevel;
	ASTExpression* assignExpr;
	uint16 slot;

	ASTExpressionDeclareReference(uint16 type, uint8 pointerLevel, ASTExpression* assignExpr, uint16 slot) :
		type(type), pointerLevel(pointerLevel), assignExpr(assignExpr), slot(slot) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionConstructorCall : public ASTExpression
{
	uint16 type;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;

	ASTExpressionConstructorCall(uint16 type, const std::vector<ASTExpression*> argExprs) :
		type(type), argExprs(argExprs), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionNew : public ASTExpression
{
	uint16 type;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;

	ASTExpressionNew(uint16 type, const std::vector<ASTExpression*> argExprs) :
		type(type), argExprs(argExprs), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionDelete : public ASTExpression
{
	ASTExpression* expr;
	bool deleteArray;

	ASTExpressionDelete(ASTExpression* expr, bool deleteArray) :
		expr(expr), deleteArray(deleteArray) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};

struct ASTExpressionNewArray : public ASTExpression
{
	uint16 type;
	uint8 pointerLevel;
	ASTExpression* sizeExpr;

	ASTExpressionNewArray(uint16 type, uint8 pointerLevel, ASTExpression* sizeExpr) :
		type(type), pointerLevel(pointerLevel), sizeExpr(sizeExpr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
};
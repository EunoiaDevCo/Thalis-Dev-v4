#pragma once

#include "Value.h"
#include "TypeInfo.h"
#include <vector>
#include <new>
#include "Operator.h"
#include "Template.h"

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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) = 0;

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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionConstUInt32 : public ASTExpression
{
	uint32 value;

	ASTExpressionConstUInt32(uint32 value) :
		value(value) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionPushLocal : public ASTExpression
{
	uint16 slot;
	TypeInfo typeInfo;
	std::string templateTypeName;
	TemplateInstantiationCommand* instantiationCommand;

	ASTExpressionPushLocal(uint16 slot, const TypeInfo& typeInfo, const std::string& templateTypeName, TemplateInstantiationCommand* instantiationCommand = nullptr) :
		slot(slot), typeInfo(typeInfo), templateTypeName(templateTypeName), instantiationCommand(instantiationCommand) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionDeclarePointer : public ASTExpression
{
	uint16 type;
	uint8 pointerLevel;
	uint16 slot;
	ASTExpression* assignExpr;
	std::string templateTypeName;
	TemplateInstantiationCommand* instantiationCommand;

	ASTExpressionDeclarePointer(uint16 type, uint8 pointerLevel, uint16 slot, ASTExpression* assignExpr = nullptr, const std::string& templateTypeName = "", TemplateInstantiationCommand* instantiationCommand = nullptr) :
		type(type), pointerLevel(pointerLevel), slot(slot), assignExpr(assignExpr), templateTypeName(templateTypeName), instantiationCommand(instantiationCommand) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionSet : public ASTExpression
{
	ASTExpression* expr;
	ASTExpression* assignExpr;
	uint16 assignFunctionID;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionSet(ASTExpression* expr, ASTExpression* assignExpr) :
		expr(expr), assignExpr(assignExpr), assignFunctionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionAddressOf : public ASTExpression
{
	ASTExpression* expr;

	ASTExpressionAddressOf(ASTExpression* expr) :
		expr(expr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionDereference : public ASTExpression
{
	ASTExpression* expr;

	ASTExpressionDereference(ASTExpression* expr) :
		expr(expr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionStackArrayDeclare : public ASTExpression
{
	uint16 type;
	uint8 elementPointerLevel;
	uint16 slot;
	std::vector<std::pair<uint32, std::string>> dimensions;
	std::vector<ASTExpression*> initializerExprs;
	std::string templateTypeName;

	ASTExpressionStackArrayDeclare(uint16 type, uint8 elementPointerLevel, uint16 slot, const std::vector<std::pair<uint32, std::string>>& dimensions,
		const std::vector<ASTExpression*>& initializerExprs, const std::string& templateTypeName) :
		type(type), elementPointerLevel(elementPointerLevel), slot(slot), dimensions(dimensions), initializerExprs(initializerExprs), templateTypeName(templateTypeName) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionPushIndex : public ASTExpression
{
	ASTExpression* expr;
	std::vector<ASTExpression*> indexExprs;
	uint16 indexFunctionID;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionPushIndex(ASTExpression* expr, const std::vector<ASTExpression*> indexExprs) :
		expr(expr), indexExprs(indexExprs), indexFunctionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionBinary : public ASTExpression
{
	ASTExpression* lhs;
	ASTExpression* rhs;
	Operator op;
	uint16 functionID;
	std::vector<uint16> castFunctioIDs;

	ASTExpressionBinary(ASTExpression* lhs, ASTExpression* rhs, Operator op) :
		lhs(lhs), rhs(rhs), op(op), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionWhile : public ASTExpression
{
	ASTExpression* conditionExpr;
	std::vector<ASTExpression*> whileExprs;

	ASTExpressionWhile(ASTExpression* conditionExpr, const std::vector<ASTExpression*>& whileExprs) :
		conditionExpr(conditionExpr), whileExprs(whileExprs) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionBreak : public ASTExpression
{
	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionContinue : public ASTExpression
{
	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionStaticFunctionCall : public ASTExpression
{
	uint16 classID;
	std::string functionName;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionStaticFunctionCall(uint16 classID, const std::string& functionName, const std::vector<ASTExpression*> argExprs) :
		classID(classID), functionName(functionName), argExprs(argExprs), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionReturn : public ASTExpression
{
	ASTExpression* expr;
	bool returnsReference;

	ASTExpressionReturn(ASTExpression* expr = nullptr, bool returnsReference = false) :
		expr(expr), returnsReference(returnsReference) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionModuleConstant : public ASTExpression
{
	uint16 moduleID;
	uint16 constantID;

	ASTExpressionModuleConstant(uint16 moduleID, uint16 constantID) :
		moduleID(moduleID), constantID(constantID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionDeclareObjectWithConstructor : public ASTExpression
{
	uint16 type;
	std::vector<ASTExpression*> argExprs;
	uint16 slot;
	uint16 functionID;
	std::string templateTypeName;
	TemplateInstantiationCommand* instantiationCommand;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionDeclareObjectWithConstructor(uint16 type, const std::vector<ASTExpression*> argExprs, uint16 slot, const std::string& templateTypeName, TemplateInstantiationCommand* instantiationCommand = nullptr) :
		type(type), argExprs(argExprs), slot(slot), templateTypeName(templateTypeName), instantiationCommand(instantiationCommand), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionDeclareObjectWithAssign : public ASTExpression
{
	uint16 type;
	uint16 slot;
	ASTExpression* assignExpr;
	uint16 copyConstructorID;
	std::string templateTypeName;
	TemplateInstantiationCommand* instantiationCommand;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionDeclareObjectWithAssign(uint16 type, uint16 slot, ASTExpression* assignExpr, const std::string& templateTypeName, TemplateInstantiationCommand* instantiationCommand = nullptr) :
		type(type), slot(slot), assignExpr(assignExpr), templateTypeName(templateTypeName), instantiationCommand(instantiationCommand), copyConstructorID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
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
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionMemberFunctionCall : public ASTExpression
{
	ASTExpression* objExpr;
	std::string functionName;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;
	bool isVirtual;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionMemberFunctionCall(ASTExpression* objExpr, const std::string& functionName, const std::vector<ASTExpression*> argExprs) :
		objExpr(objExpr), functionName(functionName), argExprs(argExprs), functionID(INVALID_ID), isVirtual(false) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionThis : public ASTExpression
{
	uint16 classID;

	ASTExpressionThis(uint16 classID) :
		classID(classID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionDeclareReference : public ASTExpression
{
	uint16 type;
	uint8 pointerLevel;
	ASTExpression* assignExpr;
	uint16 slot;
	std::string templateTypeName;
	TemplateInstantiationCommand* instantiationCommand;

	ASTExpressionDeclareReference(uint16 type, uint8 pointerLevel, ASTExpression* assignExpr, uint16 slot, const std::string& templateTypeName, TemplateInstantiationCommand* instantiationCommand = nullptr) :
		type(type), pointerLevel(pointerLevel), assignExpr(assignExpr), slot(slot), templateTypeName(templateTypeName), instantiationCommand(instantiationCommand) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionConstructorCall : public ASTExpression
{
	uint16 type;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;
	std::string templateTypeName;
	TemplateInstantiationCommand* instantiationCommand;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionConstructorCall(uint16 type, const std::vector<ASTExpression*> argExprs, const std::string& templateTypeName, TemplateInstantiationCommand* instantiationCommand = nullptr) :
		type(type), argExprs(argExprs), templateTypeName(templateTypeName), instantiationCommand(instantiationCommand), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionNew : public ASTExpression
{
	uint16 type;
	std::vector<ASTExpression*> argExprs;
	uint16 functionID;
	std::string templateTypeName;
	std::vector<uint16> castFunctionIDs;

	ASTExpressionNew(uint16 type, const std::vector<ASTExpression*> argExprs, const std::string& templateTypeName) :
		type(type), argExprs(argExprs), templateTypeName(templateTypeName), functionID(INVALID_ID) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual bool Resolve(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionDelete : public ASTExpression
{
	ASTExpression* expr;
	bool deleteArray;

	ASTExpressionDelete(ASTExpression* expr, bool deleteArray) :
		expr(expr), deleteArray(deleteArray) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionNewArray : public ASTExpression
{
	uint16 type;
	uint8 pointerLevel;
	ASTExpression* sizeExpr;
	std::string templateTypeName;

	ASTExpressionNewArray(uint16 type, uint8 pointerLevel, ASTExpression* sizeExpr, const std::string& templateTypeName) :
		type(type), pointerLevel(pointerLevel), sizeExpr(sizeExpr), templateTypeName(templateTypeName) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionCast : public ASTExpression
{
	ASTExpression* expr;
	uint16 targetType;
	uint8 targetPointerLevel;
	std::string templateTypeName;

	ASTExpressionCast(ASTExpression* expr, uint16 targetType, uint8 targetPointerLevel, const std::string& templateTypeName) :
		expr(expr), targetType(targetType), targetPointerLevel(targetPointerLevel), templateTypeName(templateTypeName) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionNegate : public ASTExpression
{
	ASTExpression* expr;

	ASTExpressionNegate(ASTExpression* expr) :
		expr(expr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionInvert : public ASTExpression
{
	ASTExpression* expr;

	ASTExpressionInvert(ASTExpression* expr) :
		expr(expr) {
	}

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionDummy : public ASTExpression
{
	TypeInfo typeInfo;

	ASTExpressionDummy(const TypeInfo& typeInfo) :
		typeInfo(typeInfo) { }

	virtual void EmitCode(Program* program) override {}
	virtual TypeInfo GetTypeInfo(Program* program) override { return typeInfo; }
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override { return  new ASTExpressionDummy(typeInfo); }
};

struct ASTExpressionStrlen : public ASTExpression
{
	ASTExpression* expr;

	ASTExpressionStrlen(ASTExpression* expr) :
		expr(expr) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionSizeOfStatic : public ASTExpression
{
	uint16 type;
	bool pointer;
	std::string templateTypeName;

	ASTExpressionSizeOfStatic(uint16 type, bool pointer, const std::string& templateTypeName) :
		type(type), pointer(pointer), templateTypeName(templateTypeName) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};

struct ASTExpressionOffsetOf : public ASTExpression
{
	uint16 classID;
	std::vector<std::string> members;
	uint64 offset;

	ASTExpressionOffsetOf(uint16 classID, const std::vector<std::string>& members) :
		classID(classID), members(members), offset(UINT64_MAX) { }

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
	virtual bool Resolve(Program* program) override;
};

struct ASTExpressionArithmaticEquals : public ASTExpression
{
	ASTExpression* expr;
	ASTExpression* incrementExpr;
	Operator op;

	ASTExpressionArithmaticEquals(ASTExpression* expr, ASTExpression* incrementExpr, Operator op) :
		expr(expr), incrementExpr(incrementExpr), op(op) {}

	virtual void EmitCode(Program* program) override;
	virtual TypeInfo GetTypeInfo(Program* program) override;
	virtual ASTExpression* InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass) override;
};
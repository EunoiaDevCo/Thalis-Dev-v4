#include "ASTExpression.h"
#include "Program.h"
#include "Class.h"
#include "Modules/ModuleID.h"

void* ASTExpression::operator new(std::size_t size) {
	void* ptr = ::operator new(size);
	Program::GetCompiledProgram()->AddCreatedExpression((ASTExpression*)ptr);
	return ptr;
}

void ASTExpression::operator delete(void* ptr) noexcept {
	::operator delete(ptr);
}

void ASTExpressionLiteral::EmitCode(Program* program)
{
	if (isStatement) return;

	if (value.type == (uint16)ValueType::CHAR && value.pointerLevel == 1)
	{
		program->AddPushCStrCommand((char*)value.data);
		return;
	}

	if (value.type == INVALID_ID && value.data == nullptr)
	{
		program->WriteOPCode(OpCode::PUSH_UNTYPED_NULL);
		return;
	}

	switch ((ValueType)value.type)
	{
	case ValueType::UINT8: program->AddPushConstantUInt8Command(value.GetUInt8()); break;
	case ValueType::UINT16: program->AddPushConstantUInt16Command(value.GetUInt16()); break;
	case ValueType::UINT32: program->AddPushConstantUInt32Command(value.GetUInt32()); break;
	case ValueType::UINT64: program->AddPushConstantUInt64Command(value.GetUInt64()); break;

	case ValueType::INT8: program->AddPushConstantInt8Command(value.GetInt8()); break;
	case ValueType::INT16: program->AddPushConstantInt16Command(value.GetInt16()); break;
	case ValueType::INT32: program->AddPushConstantInt32Command(value.GetInt32()); break;
	case ValueType::INT64: program->AddPushConstantInt64Command(value.GetInt64()); break;

	case ValueType::REAL32: program->AddPushConstantReal32Command(value.GetReal32()); break;
	case ValueType::REAL64: program->AddPushConstantReal64Command(value.GetReal64()); break;

	case ValueType::BOOL: program->AddPushConstantBoolCommand(value.GetBool()); break;
	case ValueType::CHAR: program->AddPushConstantCharCommand(value.GetChar()); break;
	}
}

TypeInfo ASTExpressionLiteral::GetTypeInfo(Program* program)
{
	return TypeInfo(value.type, value.pointerLevel);
}

ASTExpression* ASTExpressionLiteral::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionLiteral(value);
}

void ASTExpressionConstUInt32::EmitCode(Program* program)
{
	program->AddPushConstantUInt32Command(value);
}

TypeInfo ASTExpressionConstUInt32::GetTypeInfo(Program* program)
{
	return TypeInfo((uint16)ValueType::UINT32, 0);
}

ASTExpression* ASTExpressionConstUInt32::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionConstUInt32(value);
}

void ASTExpressionModuleFunctionCall::EmitCode(Program* program)
{
	for (int32 i = argExprs.size() - 1; i >= 0; i--)
	{
		argExprs[i]->EmitCode(program);
	}

	program->AddModuleFunctionCallCommand(moduleID, functionID, argExprs.size(), !isStatement);
}

TypeInfo ASTExpressionModuleFunctionCall::GetTypeInfo(Program* program)
{
	return Module::GetFunctionReturnInfo(moduleID, functionID);
}

ASTExpression* ASTExpressionModuleFunctionCall::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	std::vector<ASTExpression*> injectedArgExprs;
	for (uint32 i = 0; i < argExprs.size(); i++)
		injectedArgExprs.push_back(argExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionModuleFunctionCall(moduleID, functionID, injectedArgExprs);
}

void ASTExpressionDeclarePrimitive::EmitCode(Program* program)
{
	if (assignExpr)
	{
		assignExpr->EmitCode(program);
	}
	else
	{
		switch (type)
		{
		case ValueType::UINT8:	program->AddPushConstantUInt8Command(0); break;
		case ValueType::UINT16: program->AddPushConstantUInt16Command(0); break;
		case ValueType::UINT32: program->AddPushConstantUInt32Command(0); break;
		case ValueType::UINT64: program->AddPushConstantUInt64Command(0); break;
		case ValueType::INT8:	program->AddPushConstantInt8Command(0); break;
		case ValueType::INT16:	program->AddPushConstantInt16Command(0); break;
		case ValueType::INT32:	program->AddPushConstantInt32Command(0); break;
		case ValueType::INT64:	program->AddPushConstantInt64Command(0); break;
		case ValueType::REAL32: program->AddPushConstantReal32Command(0.0f); break;
		case ValueType::REAL64: program->AddPushConstantReal64Command(0.0); break;
		case ValueType::BOOL:	program->AddPushConstantBoolCommand(false); break;
		case ValueType::CHAR:	program->AddPushConstantCharCommand(0); break;
		}
	}

	program->AddDeclarePrimitiveCommand(type, slot);
}

TypeInfo ASTExpressionDeclarePrimitive::GetTypeInfo(Program* program)
{
	return TypeInfo((uint16)type, 0);
}

ASTExpression* ASTExpressionDeclarePrimitive::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedAssignExpr = assignExpr ? assignExpr->InjectTemplateType(program, cls, instantiation, templatedClass) : nullptr;
	return new ASTExpressionDeclarePrimitive(type, slot, injectedAssignExpr);
}

void ASTExpressionPushLocal::EmitCode(Program* program)
{
	program->AddPushLocalCommand(slot);
}

TypeInfo ASTExpressionPushLocal::GetTypeInfo(Program* program)
{
	return typeInfo;
}

ASTExpression* ASTExpressionPushLocal::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	TypeInfo injectedTypeInfo = typeInfo;
	
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedTypeInfo.pointerLevel = typeInfo.pointerLevel + instantiation.args[index].pointerLevel;
		injectedTypeInfo.type = instantiation.args[index].value;
	}

	if (instantiationCommand)
	{
		injectedTypeInfo.type = cls->ExecuteInstantiationCommand(program, instantiationCommand, instantiation);
	}

	if (slot == INVALID_ID)
		uint32 bp = 0;

	return new ASTExpressionPushLocal(slot, injectedTypeInfo, "");
}

void ASTExpressionDeclarePointer::EmitCode(Program* program)
{
	if (assignExpr)
	{
		assignExpr->EmitCode(program);
	}
	else
	{
		program->AddPushTypedNullCommand(type, pointerLevel);
	}

	program->AddDeclarePointerCommand(type, pointerLevel, slot);
}

TypeInfo ASTExpressionDeclarePointer::GetTypeInfo(Program* program)
{
	return TypeInfo(type, pointerLevel);
}

ASTExpression* ASTExpressionDeclarePointer::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	uint8 injectedPointerLevel = pointerLevel;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedPointerLevel += instantiation.args[index].pointerLevel;
		injectedType = instantiation.args[index].value;
	}

	if (instantiationCommand)
	{
		injectedType = cls->ExecuteInstantiationCommand(program, instantiationCommand, instantiation);
	}

	ASTExpression* injectedAssignExpr = assignExpr ? assignExpr->InjectTemplateType(program, cls, instantiation, templatedClass) : nullptr;
	return new ASTExpressionDeclarePointer(injectedType, injectedPointerLevel, slot, injectedAssignExpr);
}

void ASTExpressionSet::EmitCode(Program* program)
{
	assignExpr->EmitCode(program);
	expr->EmitCode(program);
	program->AddSetCommand(assignFunctionID);

	if (assignFunctionID != INVALID_ID)
	{
		for (int32 i = castFunctionIDs.size() - 1; i >= 0 ; i--)
			program->WriteUInt16(castFunctionIDs[i]);
	}
}

TypeInfo ASTExpressionSet::GetTypeInfo(Program* program)
{
	return expr->GetTypeInfo(program);
}

bool ASTExpressionSet::Resolve(Program* program)
{
	TypeInfo exprTypeInfo = expr->GetTypeInfo(program);
	if (exprTypeInfo.pointerLevel > 0 || Value::IsPrimitiveType(exprTypeInfo.type) || exprTypeInfo.type == INVALID_ID) 
		return true;

	Class* cls = program->GetClass(exprTypeInfo.type);
	std::vector<ASTExpression*> argExprs;
	argExprs.push_back(assignExpr);
	assignFunctionID = cls->GetFunctionID("operator=", argExprs, castFunctionIDs, true);

	return true;
}

ASTExpression* ASTExpressionSet::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	ASTExpression* injectedAssignExpr = assignExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionSet(injectedExpr, injectedAssignExpr);
}

void ASTExpressionAddressOf::EmitCode(Program* program)
{
	if (isStatement) return;
	expr->EmitCode(program);
	program->WriteOPCode(OpCode::ADDRESS_OF);
}

TypeInfo ASTExpressionAddressOf::GetTypeInfo(Program* program)
{
	TypeInfo typeInfo = expr->GetTypeInfo(program);
	typeInfo.pointerLevel++;
	return typeInfo;
}

ASTExpression* ASTExpressionAddressOf::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionAddressOf(injectedExpr);
}

void ASTExpressionDereference::EmitCode(Program* program)
{
	if (isStatement) return;
	expr->EmitCode(program);
	program->WriteOPCode(OpCode::DEREFERENCE);
}

TypeInfo ASTExpressionDereference::GetTypeInfo(Program* program)
{
	TypeInfo typeInfo = expr->GetTypeInfo(program);
	typeInfo.pointerLevel--;
	return typeInfo;
}

ASTExpression* ASTExpressionDereference::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionDereference(injectedExpr);
}

void ASTExpressionStackArrayDeclare::EmitCode(Program* program)
{
	for (int32 i = initializerExprs.size() - 1; i >= 0; i--)
	{
		initializerExprs[i]->EmitCode(program);
	}

	std::vector<uint32> dims;
	for (uint32 i = 0; i < dimensions.size(); i++)
		dims.push_back(dimensions[i].first);

	program->AddDeclareStackArrayCommand(type, elementPointerLevel, dims.data(), dims.size(), initializerExprs.size(), slot);
}

TypeInfo ASTExpressionStackArrayDeclare::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 1);
}

ASTExpression* ASTExpressionStackArrayDeclare::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	uint8 injectedPointerLevel = elementPointerLevel;

	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
		injectedPointerLevel += instantiation.args[index].pointerLevel;
	}

	std::vector<std::pair<uint32, std::string>> injectedDimensions;
	for (uint32 i = 0; i < dimensions.size(); i++)
	{
		if (dimensions[i].second.empty())
		{
			injectedDimensions.push_back(dimensions[i]);
		}
		else
		{
			uint32 index = cls->InstantiateTemplateGetIndex(program, dimensions[i].second);
			injectedDimensions.push_back(std::make_pair(instantiation.args[index].value, ""));
		}
	}

	std::vector<ASTExpression*> injectedInitialzeExprs;
	for (uint32 i = 0; i < initializerExprs.size(); i++)
		injectedInitialzeExprs.push_back(initializerExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionStackArrayDeclare(injectedType, injectedPointerLevel, slot, injectedDimensions, injectedInitialzeExprs, "");
}

void ASTExpressionPushIndex::EmitCode(Program* program)
{
	if (isStatement) return;
	TypeInfo typeInfo = expr->GetTypeInfo(program);
	expr->EmitCode(program);
	for (int32 i = indexExprs.size() - 1; i >= 0; i--)
		indexExprs[i]->EmitCode(program);

	program->AddPushIndexedCommand(program->GetTypeSize(typeInfo.type), indexExprs.size(), indexFunctionID, typeInfo.type);
	if (indexFunctionID != INVALID_ID)
	{
		for (int32 i = castFunctionIDs.size() - 1; i >= 0; i--)
			program->WriteUInt16(castFunctionIDs[i]);
	}
}

TypeInfo ASTExpressionPushIndex::GetTypeInfo(Program* program)
{
	TypeInfo typeInfo = expr->GetTypeInfo(program);

	if (!Value::IsPrimitiveType(typeInfo.type) && typeInfo.pointerLevel == 0)
	{
		Class* cls = program->GetClass(typeInfo.type);
		indexFunctionID = indexFunctionID == INVALID_ID ? cls->GetFunctionID("operator[]", indexExprs, castFunctionIDs) : indexFunctionID;

		if(indexFunctionID != INVALID_ID)
		{
			const TypeInfo& returnInfo = cls->GetFunction(indexFunctionID)->returnInfo;
			return returnInfo;
		}
	}

	typeInfo.pointerLevel--;

	return typeInfo;
}

ASTExpression* ASTExpressionPushIndex::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	std::vector<ASTExpression*> injectedIndexExprs;
	for (uint32 i = 0; i < indexExprs.size(); i++)
		injectedIndexExprs.push_back(indexExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionPushIndex(injectedExpr, injectedIndexExprs);
}

bool ASTExpressionPushIndex::Resolve(Program* program)
{
	TypeInfo typeInfo = expr->GetTypeInfo(program);
	if (Value::IsPrimitiveType(typeInfo.type) || typeInfo.pointerLevel > 0) return true;

	Class* cls = program->GetClass(typeInfo.type);
	indexFunctionID = indexFunctionID == INVALID_ID ? cls->GetFunctionID("operator[]", indexExprs, castFunctionIDs) : indexFunctionID;
}

void ASTExpressionBinary::EmitCode(Program* program)
{
	if (isStatement) return;

	lhs->EmitCode(program);
	rhs->EmitCode(program);

	program->AddAritmaticCommand(op, functionID);
	if (functionID != INVALID_ID)
	{
		for (uint32 i = 0; i < castFunctioIDs.size(); i++)
			program->WriteUInt16(castFunctioIDs[i]);
	}
}

TypeInfo ASTExpressionBinary::GetTypeInfo(Program* program)
{
	TypeInfo leftType = lhs->GetTypeInfo(program);
	TypeInfo rightType = rhs->GetTypeInfo(program);

	if (leftType.pointerLevel > 0)
	{
		return TypeInfo(leftType.type, leftType.pointerLevel);
	}
	if (Value::IsPrimitiveType(leftType.type) && Value::IsPrimitiveType(rightType.type))
	{
		uint16 resultType = Value::PromoteType(leftType.type, rightType.type);
		return TypeInfo(resultType, 0);
	}
	else
	{
		Class* cls = program->GetClass(leftType.type);
		std::vector<ASTExpression*> args;
		args.push_back(rhs);
		if (functionID == INVALID_ID)
		{
			switch (op)
			{
			case Operator::ADD:		functionID = cls->GetFunctionID("operator+", args, castFunctioIDs); break;
			case Operator::MINUS:	functionID = cls->GetFunctionID("operator-", args, castFunctioIDs); break;
			case Operator::MULTIPLY: functionID = cls->GetFunctionID("operator*", args, castFunctioIDs); break;
			case Operator::DIVIDE:	functionID = cls->GetFunctionID("operator/", args, castFunctioIDs); break;
			case Operator::MOD:		functionID = cls->GetFunctionID("operator%", args, castFunctioIDs); break;
			case Operator::EQUALS:		functionID = cls->GetFunctionID("operator==", args, castFunctioIDs); break;
			case Operator::NOT_EQUALS:		functionID = cls->GetFunctionID("operator!=", args, castFunctioIDs); break;
			case Operator::LESS:		functionID = cls->GetFunctionID("operator<", args, castFunctioIDs); break;
			case Operator::GREATER:		functionID = cls->GetFunctionID("operator>", args, castFunctioIDs); break;
			case Operator::LESS_EQUALS:		functionID = cls->GetFunctionID("operator<=", args, castFunctioIDs); break;
			case Operator::GREATER_EQUALS:		functionID = cls->GetFunctionID("operator>=", args, castFunctioIDs); break;
			}
		}

		if (functionID != INVALID_ID)
		{
			return cls->GetFunction(functionID)->returnInfo;
		}
	}

	return TypeInfo(INVALID_ID, 0);
}

bool ASTExpressionBinary::Resolve(Program* program)
{
	TypeInfo lhsType = lhs->GetTypeInfo(program);

	if (lhsType.type == INVALID_ID) return true;

	if (Value::IsPrimitiveType(lhsType.type)) return true;
	if (lhsType.pointerLevel > 0) return true;

	Class* cls = program->GetClass(lhsType.type);
	std::vector<ASTExpression*> args;
	args.push_back(rhs);

	switch (op)
	{
	case Operator::ADD:				functionID = cls->GetFunctionID("operator+", args, castFunctioIDs); break;
	case Operator::MINUS:			functionID = cls->GetFunctionID("operator-", args, castFunctioIDs); break;
	case Operator::MULTIPLY:			functionID = cls->GetFunctionID("operator*", args, castFunctioIDs); break;
	case Operator::DIVIDE:			functionID = cls->GetFunctionID("operator/", args, castFunctioIDs); break;
	case Operator::MOD:				functionID = cls->GetFunctionID("operator%", args, castFunctioIDs); break;
	case Operator::EQUALS:			functionID = cls->GetFunctionID("operator==", args, castFunctioIDs); break;
	case Operator::NOT_EQUALS:		functionID = cls->GetFunctionID("operator!=", args, castFunctioIDs); break;
	case Operator::LESS:				functionID = cls->GetFunctionID("operator<", args, castFunctioIDs); break;
	case Operator::GREATER:			functionID = cls->GetFunctionID("operator>", args, castFunctioIDs); break;
	case Operator::LESS_EQUALS:		functionID = cls->GetFunctionID("operator<=", args, castFunctioIDs); break;
	case Operator::GREATER_EQUALS:	functionID = cls->GetFunctionID("operator>=", args, castFunctioIDs); break;
	}

	return true;
}

ASTExpression* ASTExpressionBinary::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedLHS = lhs->InjectTemplateType(program, cls, instantiation, templatedClass);
	ASTExpression* injectedRHS = rhs->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionBinary(injectedLHS, injectedRHS, op);
}

void ASTExpressionIfElse::EmitCode(Program* program)
{
	conditionExpr->EmitCode(program);
	program->WriteOPCode(OpCode::JUMP_IF_FALSE);
	uint32 jumpIfFalsePos = program->GetCodeSize();
	program->WriteUInt32(0);

	if (pushIfScope)
		program->WriteOPCode(OpCode::PUSH_SCOPE);

	for (uint32 i = 0; i < ifExprs.size(); i++)
		ifExprs[i]->EmitCode(program);

	if (pushIfScope)
		program->WriteOPCode(OpCode::POP_SCOPE);

	program->WriteOPCode(OpCode::JUMP);
	uint32 jumpToEndPos = program->GetCodeSize();
	program->WriteUInt32(0);

	uint32 elseLablePos = program->GetCodeSize();
	program->PatchUInt32(jumpIfFalsePos, elseLablePos);

	if (pushElseScope)
		program->WriteOPCode(OpCode::PUSH_SCOPE);

	for (uint32 i = 0; i < elseExprs.size(); i++)
		elseExprs[i]->EmitCode(program);

	if (pushElseScope)
		program->WriteOPCode(OpCode::POP_SCOPE);

	uint32 endLabelPos = program->GetCodeSize();
	program->PatchUInt32(jumpToEndPos, endLabelPos);
}

TypeInfo ASTExpressionIfElse::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

ASTExpression* ASTExpressionIfElse::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedConditionExpr = conditionExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	std::vector<ASTExpression*> injectedIfExprs;
	std::vector<ASTExpression*> injectedElseExprs;

	for (uint32 i = 0; i < ifExprs.size(); i++)
		injectedIfExprs.push_back(ifExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	for (uint32 i = 0; i < elseExprs.size(); i++)
		injectedElseExprs.push_back(elseExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionIfElse(injectedConditionExpr, pushIfScope, pushElseScope, injectedIfExprs, injectedElseExprs);
}

void ASTExpressionFor::EmitCode(Program* program)
{
	if (declareExpr)
		declareExpr->EmitCode(program);

	uint32 pushLoopPos = program->AddPushLoopCommand();
	uint32 conditionPos = program->GetCodeSize();
	program->WriteOPCode(OpCode::PUSH_SCOPE);

	if (conditionExpr)
		conditionExpr->EmitCode(program);
	else
	{
		program->AddPushConstantBoolCommand(true);
	}

	program->WriteOPCode(OpCode::JUMP_IF_FALSE);
	uint32 jumpIfFalsePos = program->GetCodeSize();
	program->WriteUInt32(0);

	for (uint32 i = 0; i < forExprs.size(); i++)
		forExprs[i]->EmitCode(program);

	uint32 incrPos = program->GetCodeSize();
	if (incrExpr)
		incrExpr->EmitCode(program);

	program->WriteOPCode(OpCode::POP_SCOPE);
	program->WriteOPCode(OpCode::JUMP);
	program->WriteUInt32(conditionPos);
	uint32 loopEndPos = program->GetCodeSize();
	program->AddPopLoopCommand();
	program->WriteOPCode(OpCode::POP_SCOPE);

	program->PatchUInt32(jumpIfFalsePos, loopEndPos);
	program->PatchPushLoopCommand(pushLoopPos, incrPos, loopEndPos);
}

TypeInfo ASTExpressionFor::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

ASTExpression* ASTExpressionFor::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedDeclareExpr = declareExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	ASTExpression* injectedConditionExpr = conditionExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	ASTExpression* injectedIncrExpr = incrExpr->InjectTemplateType(program, cls, instantiation, templatedClass);

	std::vector<ASTExpression*> injectedForExprs;
	for (uint32 i = 0; i < forExprs.size(); i++)
		injectedForExprs.push_back(forExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionFor(injectedDeclareExpr, injectedConditionExpr, injectedIncrExpr, injectedForExprs);
}

void ASTExpressionUnaryUpdate::EmitCode(Program* program)
{
	expr->EmitCode(program);
	program->AddUnaryUpdateCommand((uint8)op, !isStatement);
}

TypeInfo ASTExpressionUnaryUpdate::GetTypeInfo(Program* program)
{
	return expr->GetTypeInfo(program);
}

ASTExpression* ASTExpressionUnaryUpdate::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionUnaryUpdate(injectedExpr, op);
}

void ASTExpressionWhile::EmitCode(Program* program)
{
	uint32 pushLoopPos = program->AddPushLoopCommand();
	uint32 conditionPos = program->GetCodeSize();
	program->WriteOPCode(OpCode::PUSH_SCOPE);
	conditionExpr->EmitCode(program);

	program->WriteOPCode(OpCode::JUMP_IF_FALSE);
	uint32 jumpIfFalsePos = program->GetCodeSize();
	program->WriteUInt32(0);

	for (uint32 i = 0; i < whileExprs.size(); i++)
		whileExprs[i]->EmitCode(program);

	program->WriteOPCode(OpCode::POP_SCOPE);
	program->WriteOPCode(OpCode::JUMP);
	program->WriteUInt32(conditionPos);
	uint32 loopEndPos = program->GetCodeSize();
	program->AddPopLoopCommand();
	program->WriteOPCode(OpCode::POP_SCOPE);

	program->PatchUInt32(jumpIfFalsePos, loopEndPos);
	program->PatchPushLoopCommand(pushLoopPos, conditionPos, loopEndPos);
}

TypeInfo ASTExpressionWhile::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

ASTExpression* ASTExpressionWhile::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedConditionExpr = conditionExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	std::vector<ASTExpression*> injectedWhileExprs;
	for (uint32 i = 0; i < whileExprs.size(); i++)
		injectedWhileExprs.push_back(whileExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionWhile(injectedConditionExpr, injectedWhileExprs);
}

void ASTExpressionBreak::EmitCode(Program* program)
{
	program->WriteOPCode(OpCode::BREAK);
}

TypeInfo ASTExpressionBreak::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

ASTExpression* ASTExpressionBreak::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionBreak();
}

void ASTExpressionContinue::EmitCode(Program* program)
{
	program->WriteOPCode(OpCode::CONTINUE);
}

TypeInfo ASTExpressionContinue::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

ASTExpression* ASTExpressionContinue::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionContinue();
}

void ASTExpressionStaticFunctionCall::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	program->AddStaticFunctionCallCommand(classID, functionID, !isStatement);
	for (int32 i = castFunctionIDs.size() - 1; i >= 0; i--)
		program->WriteUInt16(castFunctionIDs[i]);
}

TypeInfo ASTExpressionStaticFunctionCall::GetTypeInfo(Program* program)
{
	functionID = functionID == INVALID_ID ? program->GetClass(classID)->GetFunctionID(functionName, argExprs, castFunctionIDs) : functionID;
	return program->GetClass(classID)->GetFunction(functionID)->returnInfo;
}

bool ASTExpressionStaticFunctionCall::Resolve(Program* program)
{
	functionID = functionID == INVALID_ID ? program->GetClass(classID)->GetFunctionID(functionName, argExprs, castFunctionIDs) : functionID;
	return true;
}

ASTExpression* ASTExpressionStaticFunctionCall::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	std::vector<ASTExpression*> injectedArgExprs;
	for (uint32 i = 0; i < argExprs.size(); i++)
		injectedArgExprs.push_back(argExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionStaticFunctionCall(templatedClass->GetID(), functionName, argExprs);
}

void ASTExpressionReturn::EmitCode(Program* program)
{
	if (expr)
		expr->EmitCode(program);

	if (returnsReference)
	{
		program->AddReturnCommand(2);
	}
	else
	{
		if (expr)
		{
			program->AddReturnCommand(1);
		}
		else
		{
			program->AddReturnCommand(0);
		}
	}
}

TypeInfo ASTExpressionReturn::GetTypeInfo(Program* program)
{
	return expr->GetTypeInfo(program);
}

ASTExpression* ASTExpressionReturn::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr ? expr->InjectTemplateType(program, cls, instantiation, templatedClass) : nullptr;
	return new ASTExpressionReturn(injectedExpr, returnsReference);
}

void ASTExpressionStaticVariable::EmitCode(Program* program)
{
	if (isStatement) return;
	program->AddPushStaticVariableCommand(classID, offset, typeInfo.type, typeInfo.pointerLevel, false, isArray);
}

TypeInfo ASTExpressionStaticVariable::GetTypeInfo(Program* program)
{
	if (offset == UINT64_MAX)
		offset = program->GetClass(classID)->CalculateStaticOffset(program, members, &typeInfo, &isArray);
	return typeInfo;
}

bool ASTExpressionStaticVariable::Resolve(Program* program)
{
	if(offset == UINT64_MAX)
		offset = program->GetClass(classID)->CalculateStaticOffset(program, members, &typeInfo, &isArray);
	return offset != UINT64_MAX;
}

ASTExpression* ASTExpressionStaticVariable::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionStaticVariable(classID, offset, typeInfo, isArray);
}

void ASTExpressionModuleConstant::EmitCode(Program* program)
{
	if (isStatement) return;
	program->AddModuleConstantCommand(moduleID, constantID);
}

TypeInfo ASTExpressionModuleConstant::GetTypeInfo(Program* program)
{
	return Module::GetConstantTypeInfo(moduleID, constantID);
}

ASTExpression* ASTExpressionModuleConstant::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionModuleConstant(moduleID, constantID);
}

void ASTExpressionDeclareObjectWithConstructor::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	program->AddDeclareObjectWithConstructorCommand(type, functionID, slot);
	for (int32 i = castFunctionIDs.size() - 1; i >= 0; i--)
		program->WriteUInt16(castFunctionIDs[i]);
}

TypeInfo ASTExpressionDeclareObjectWithConstructor::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 0);
}

bool ASTExpressionDeclareObjectWithConstructor::Resolve(Program* program)
{
	if (type == (uint16)ValueType::TEMPLATE_TYPE) return true;

	Class* cls = program->GetClass(type);
	functionID = cls->GetFunctionID(cls->GetName(), argExprs, castFunctionIDs);
	return true;
}

ASTExpression* ASTExpressionDeclareObjectWithConstructor::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
	}

	if (instantiationCommand)
	{
		injectedType = cls->ExecuteInstantiationCommand(program, instantiationCommand, instantiation);
	}

	std::vector<ASTExpression*> injectedArgExprs;
	for (uint32 i = 0; i < argExprs.size(); i++)
		injectedArgExprs.push_back(argExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionDeclareObjectWithConstructor(injectedType, injectedArgExprs, slot, "");
}

void ASTExpressionDeclareObjectWithAssign::EmitCode(Program* program)
{
	assignExpr->EmitCode(program);
	program->AddDeclareObjectWithAssignCommand(type, slot, copyConstructorID);
	if (copyConstructorID != INVALID_ID)
	{
		for (int32 i = castFunctionIDs.size() - 1; i >= 0; i--)
			program->WriteUInt16(castFunctionIDs[i]);
	}
}

TypeInfo ASTExpressionDeclareObjectWithAssign::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 0);
}

bool ASTExpressionDeclareObjectWithAssign::Resolve(Program* program)
{
	if (type == (uint16)ValueType::TEMPLATE_TYPE)
		return true;
	Class* cls = program->GetClass(type);
	std::vector<ASTExpression*> argExprs;
	argExprs.push_back(assignExpr);
	copyConstructorID = cls->GetFunctionID(cls->GetName(), argExprs, castFunctionIDs);
	return true;
}

ASTExpression* ASTExpressionDeclareObjectWithAssign::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	uint8 injectedPointerLevel = 0;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
		injectedPointerLevel = instantiation.args[index].value;
	}

	if (instantiationCommand)
	{
		injectedType = cls->ExecuteInstantiationCommand(program, instantiationCommand, instantiation);
	}

	ASTExpression* injectedAssignExpr = assignExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	if (injectedPointerLevel > 0)
	{
		return new ASTExpressionDeclarePointer(injectedType, injectedPointerLevel, slot, injectedAssignExpr, "");
	}

	return new ASTExpressionDeclareObjectWithAssign(injectedType, slot, injectedAssignExpr, "");
}

void ASTExpressionPushMember::EmitCode(Program* program)
{
	if (isStatement) return;

	expr->EmitCode(program);

	if (typeInfo.type == INVALID_ID)
		uint32 bp = 0;

	program->AddPushMemberCommand(typeInfo.type, typeInfo.pointerLevel, offset, false, isArray);
}

TypeInfo ASTExpressionPushMember::GetTypeInfo(Program* program)
{
	if (!members.empty())
	{
		TypeInfo exprTypeInfo = expr->GetTypeInfo(program);
		if (exprTypeInfo.type == (uint16)ValueType::TEMPLATE_TYPE)
			return TypeInfo((uint16)ValueType::TEMPLATE_TYPE, 0);
		if(exprTypeInfo.type == INVALID_ID)
			return TypeInfo(INVALID_ID, 0);

		Class* cls = program->GetClass(exprTypeInfo.type);
		offset = cls->CalculateMemberOffset(program, members, &typeInfo, &isArray);
	}
	return typeInfo;
}

bool ASTExpressionPushMember::Resolve(Program* program)
{
	if(!members.empty())
	{
		TypeInfo exprTypeInfo = expr->GetTypeInfo(program);
		if (exprTypeInfo.type == (uint16)ValueType::TEMPLATE_TYPE)
			return true;
		if (exprTypeInfo.type == INVALID_ID) return true;

		Class* cls = program->GetClass(exprTypeInfo.type);
		offset = cls->CalculateMemberOffset(program, members, &typeInfo, &isArray);
		uint32 bp = 0;

		if (typeInfo.type == (uint16)ValueType::TEMPLATE_TYPE)
		{
			uint32 bp = 0;
		}

	}
	return offset != UINT64_MAX;
}

ASTExpression* ASTExpressionPushMember::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionPushMember(injectedExpr, members);
}

void ASTExpressionMemberFunctionCall::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	objExpr->EmitCode(program);
	TypeInfo objTypeInfo = objExpr->GetTypeInfo(program);
	if (objTypeInfo.pointerLevel == 1) 
		program->WriteOPCode(OpCode::DEREFERENCE);

	if (isVirtual)
	{
		program->AddVirtualFunctionCallCommand(functionID, !isStatement);
	}
	else
	{
		program->AddMemberFunctionCallCommand(objTypeInfo.type, functionID, !isStatement);
	}

	for (int32 i = castFunctionIDs.size() - 1; i >= 0; i--)
		program->WriteUInt16(castFunctionIDs[i]);
}

TypeInfo ASTExpressionMemberFunctionCall::GetTypeInfo(Program* program)
{
	TypeInfo objTypeInfo = objExpr->GetTypeInfo(program);
	if (objTypeInfo.type == INVALID_ID)
		return TypeInfo(INVALID_ID, 0);
	if (objTypeInfo.type == (uint16)ValueType::TEMPLATE_TYPE) 
		return TypeInfo((uint16)ValueType::TEMPLATE_TYPE, 0);

	Class* objClass = program->GetClass(objTypeInfo.type);
	uint16 fid = (functionID == INVALID_ID) ? objClass->GetFunctionID(functionName, argExprs, castFunctionIDs) : functionID;
	TypeInfo returnInfo = objClass->GetFunction(functionID)->returnInfo;
	if (!isVirtual)
		functionID = fid;

	return returnInfo;
}

bool ASTExpressionMemberFunctionCall::Resolve(Program* program)
{
	TypeInfo objTypeInfo = objExpr->GetTypeInfo(program);
	if (objTypeInfo.type == INVALID_ID) return true;
	if(objTypeInfo.type == (uint16)ValueType::TEMPLATE_TYPE) return true;

	Class* objClass = program->GetClass(objTypeInfo.type);
	functionID = (functionID == INVALID_ID) ? objClass->GetFunctionID(functionName, argExprs, castFunctionIDs) : functionID;
	if(functionID != INVALID_ID)
	{
		isVirtual = objClass->GetFunction(functionID)->isVirtual;
		if (isVirtual)
		{
			std::vector<TypeInfo> parameters;
			for (uint32 i = 0; i < argExprs.size(); i++)
				parameters.push_back(argExprs[i]->GetTypeInfo(program));
			functionID = objClass->GetVTable()->FindSlot(functionName, parameters);
		}
	}

	return true;
}

ASTExpression* ASTExpressionMemberFunctionCall::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedObjExpr = objExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	std::vector<ASTExpression*> injectedArgExprs;
	for (uint32 i = 0; i < argExprs.size(); i++)
		injectedArgExprs.push_back(argExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionMemberFunctionCall(injectedObjExpr, functionName, injectedArgExprs);
}

void ASTExpressionThis::EmitCode(Program* program)
{
	if (isStatement) return;
	program->WriteOPCode(OpCode::PUSH_THIS);
}

TypeInfo ASTExpressionThis::GetTypeInfo(Program* program)
{
	return TypeInfo(classID, 1);
}

ASTExpression* ASTExpressionThis::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionThis(templatedClass->GetID());
}

void ASTExpressionDeclareReference::EmitCode(Program* program)
{
	TypeInfo assignTypeInfo = assignExpr->GetTypeInfo(program);
	
	assignExpr->EmitCode(program);
	program->AddDeclareReferenceCommand(slot);
}

TypeInfo ASTExpressionDeclareReference::GetTypeInfo(Program* program)
{
	return TypeInfo(type, pointerLevel);
}

ASTExpression* ASTExpressionDeclareReference::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	uint8 injectedPointerLevel = pointerLevel;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
		injectedPointerLevel += instantiation.args[index].pointerLevel;
	}

	if (instantiationCommand)
	{
		injectedType = cls->ExecuteInstantiationCommand(program, instantiationCommand, instantiation);
	}

	ASTExpression* injectedAssignExpr = assignExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionDeclareReference(injectedType, injectedPointerLevel, injectedAssignExpr, slot, "");
}

void ASTExpressionConstructorCall::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	program->AddConstructorCallCommand(type, functionID);
	for (int32 i = castFunctionIDs.size() - 1; i >= 0; i--)
		program->WriteUInt16(castFunctionIDs[i]);
}

TypeInfo ASTExpressionConstructorCall::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 0);
}

bool ASTExpressionConstructorCall::Resolve(Program* program)
{
	if (type == (uint16)ValueType::TEMPLATE_TYPE) return true;
	Class* cls = program->GetClass(type);

	functionID = cls->GetFunctionID(cls->GetName(), argExprs, castFunctionIDs);
	return functionID != INVALID_ID;
}

ASTExpression* ASTExpressionConstructorCall::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
	}

	if (instantiationCommand != nullptr)
	{
		injectedType = cls->ExecuteInstantiationCommand(program, instantiationCommand, instantiation);
	}

	std::vector<ASTExpression*> injectedArgExprs;
	for (uint32 i = 0; i < argExprs.size(); i++)
		injectedArgExprs.push_back(argExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionConstructorCall(injectedType, injectedArgExprs, "");
}

void ASTExpressionNew::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	program->AddNewCommand(type, functionID);
	for (int32 i = castFunctionIDs.size() - 1; i >= 0; i--)
		program->WriteUInt16(castFunctionIDs[i]);
}

TypeInfo ASTExpressionNew::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 1);
}

bool ASTExpressionNew::Resolve(Program* program)
{
	Class* cls = program->GetClass(type);
	functionID = cls->GetFunctionID(cls->GetName(), argExprs, castFunctionIDs);
	return true;
}

ASTExpression* ASTExpressionNew::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
	}

	std::vector<ASTExpression*> injectedArgExprs;
	for (uint32 i = 0; i < argExprs.size(); i++)
		injectedArgExprs.push_back(argExprs[i]->InjectTemplateType(program, cls, instantiation, templatedClass));

	return new ASTExpressionNew(injectedType, injectedArgExprs, "");
}

void ASTExpressionDelete::EmitCode(Program* program)
{
	expr->EmitCode(program);
	if (deleteArray)	program->WriteOPCode(OpCode::DELETE_ARRAY);
	else				program->WriteOPCode(OpCode::DELETE);
}

TypeInfo ASTExpressionDelete::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

ASTExpression* ASTExpressionDelete::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionDelete(injectedExpr, deleteArray);
}

void ASTExpressionNewArray::EmitCode(Program* program)
{
	sizeExpr->EmitCode(program);
	program->AddNewArrayCommand(type, pointerLevel);
}

TypeInfo ASTExpressionNewArray::GetTypeInfo(Program* program)
{
	return TypeInfo(type, pointerLevel + 1);
}

ASTExpression* ASTExpressionNewArray::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	uint8 injectedPointerLevel = pointerLevel;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
		injectedPointerLevel += instantiation.args[index].pointerLevel;
	}

	ASTExpression* injectedSizeExpr = sizeExpr->InjectTemplateType(program, cls, instantiation, templatedClass);

	return new ASTExpressionNewArray(injectedType, injectedPointerLevel, injectedSizeExpr, "");
}

void ASTExpressionCast::EmitCode(Program* program)
{
	if (isStatement) return;

	expr->EmitCode(program);
	program->AddCastCommand(targetType, targetPointerLevel);
}

TypeInfo ASTExpressionCast::GetTypeInfo(Program* program)
{
	return TypeInfo(targetType, targetPointerLevel);
}

ASTExpression* ASTExpressionCast::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = targetType;
	uint8 injectedPointerLevel = targetPointerLevel;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
		injectedPointerLevel += instantiation.args[index].pointerLevel;
	}

	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionCast(injectedExpr, injectedType, injectedPointerLevel, "");
}

void ASTExpressionNegate::EmitCode(Program* program)
{
	if (isStatement) return;

	expr->EmitCode(program);
	program->WriteOPCode(OpCode::NEGATE);
}

TypeInfo ASTExpressionNegate::GetTypeInfo(Program* program)
{
	return expr->GetTypeInfo(program);
}

ASTExpression* ASTExpressionNegate::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionNegate(injectedExpr);
}

void ASTExpressionInvert::EmitCode(Program* program)
{
	if (isStatement) return;

	expr->EmitCode(program);
	program->WriteOPCode(OpCode::INVERT);
}

TypeInfo ASTExpressionInvert::GetTypeInfo(Program* program)
{
	return TypeInfo((uint16)ValueType::BOOL, 0);
}

ASTExpression* ASTExpressionInvert::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionInvert(injectedExpr);
}

void ASTExpressionStrlen::EmitCode(Program* program)
{
	if (isStatement) return;

	expr->EmitCode(program);
	program->WriteOPCode(OpCode::STRLEN);
}

TypeInfo ASTExpressionStrlen::GetTypeInfo(Program* program)
{
	return TypeInfo((uint16)ValueType::UINT32, 0);
}

ASTExpression* ASTExpressionStrlen::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionStrlen(injectedExpr);
}

void ASTExpressionSizeOfStatic::EmitCode(Program* program)
{
	if (isStatement) return;

	if (pointer)
		program->AddPushConstantUInt64Command(sizeof(void*));
	else
		program->AddPushConstantUInt64Command(program->GetTypeSize(type));
}

TypeInfo ASTExpressionSizeOfStatic::GetTypeInfo(Program* program)
{
	return TypeInfo((uint16)ValueType::UINT64, 0);
}

ASTExpression* ASTExpressionSizeOfStatic::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	uint16 injectedType = type;
	bool injectedPointer = pointer;
	if (!templateTypeName.empty())
	{
		uint32 index = cls->InstantiateTemplateGetIndex(program, templateTypeName);
		injectedType = instantiation.args[index].value;
		if (!injectedPointer)
			injectedPointer = instantiation.args[index].pointerLevel > 0;
	}

	return new ASTExpressionSizeOfStatic(type, injectedPointer, "");
}

void ASTExpressionOffsetOf::EmitCode(Program* program)
{
	if (isStatement) return;

	program->AddPushConstantUInt64Command(offset);
}

TypeInfo ASTExpressionOffsetOf::GetTypeInfo(Program* program)
{
	return TypeInfo((uint16)ValueType::UINT64, 0);
}

ASTExpression* ASTExpressionOffsetOf::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	return new ASTExpressionOffsetOf(classID, members);
}

bool ASTExpressionOffsetOf::Resolve(Program* program)
{
	TypeInfo typeInfo;
	bool isArray = false;
	offset = program->GetClass(classID)->CalculateMemberOffset(program, members, &typeInfo, &isArray);
	return true;
}

void ASTExpressionArithmaticEquals::EmitCode(Program* program)
{
	expr->EmitCode(program);
	incrementExpr->EmitCode(program);
	switch (op)
	{
	case Operator::ADD: program->WriteOPCode(OpCode::PLUS_EQUALS); break;
	case Operator::MINUS: program->WriteOPCode(OpCode::MINUS_EQUALS); break;
	case Operator::MULTIPLY: program->WriteOPCode(OpCode::TIMES_EQUALS); break;
	case Operator::DIVIDE: program->WriteOPCode(OpCode::DIVIDE_EQUALS); break;
	}
}

TypeInfo ASTExpressionArithmaticEquals::GetTypeInfo(Program* program)
{
	return expr->GetTypeInfo(program);
}

ASTExpression* ASTExpressionArithmaticEquals::InjectTemplateType(Program* program, Class* cls, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	ASTExpression* injectedExpr = expr->InjectTemplateType(program, cls, instantiation, templatedClass);
	ASTExpression* injectedIncrementExpr = incrementExpr->InjectTemplateType(program, cls, instantiation, templatedClass);
	return new ASTExpressionArithmaticEquals(injectedExpr, injectedIncrementExpr, op);
}

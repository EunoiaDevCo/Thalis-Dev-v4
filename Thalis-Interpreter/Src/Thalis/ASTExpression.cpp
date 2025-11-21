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

void ASTExpressionPushLocal::EmitCode(Program* program)
{
	program->AddPushLocalCommand(slot);
}

TypeInfo ASTExpressionPushLocal::GetTypeInfo(Program* program)
{
	return typeInfo;
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

void ASTExpressionSet::EmitCode(Program* program)
{
	assignExpr->EmitCode(program);
	expr->EmitCode(program);
	program->WriteOPCode(OpCode::SET);
}

TypeInfo ASTExpressionSet::GetTypeInfo(Program* program)
{
	return expr->GetTypeInfo(program);
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

void ASTExpressionStackArrayDeclare::EmitCode(Program* program)
{
	for (int32 i = initializerExprs.size() - 1; i >= 0; i--)
	{
		initializerExprs[i]->EmitCode(program);
	}

	program->AddDeclareStackArrayCommand(type, elementPointerLevel, dimensions.data(), dimensions.size(), initializerExprs.size(), slot);
}

TypeInfo ASTExpressionStackArrayDeclare::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 1);
}

void ASTExpressionPushIndex::EmitCode(Program* program)
{
	if (isStatement) return;
	TypeInfo typeInfo = GetTypeInfo(program);
	expr->EmitCode(program);
	for (int32 i = indexExprs.size() - 1; i >= 0; i--)
		indexExprs[i]->EmitCode(program);

	program->AddPushIndexedCommand(program->GetTypeSize(typeInfo.type), indexExprs.size());
}

TypeInfo ASTExpressionPushIndex::GetTypeInfo(Program* program)
{
	TypeInfo typeInfo = expr->GetTypeInfo(program);
	typeInfo.pointerLevel--;
	return typeInfo;
}

void ASTExpressionBinary::EmitCode(Program* program)
{
	if (isStatement) return;

	lhs->EmitCode(program);
	rhs->EmitCode(program);

	switch (op)
	{
	case ASTOperator::ADD: program->WriteOPCode(OpCode::ADD); break;
	case ASTOperator::MINUS: program->WriteOPCode(OpCode::SUBTRACT); break;
	case ASTOperator::MULTIPLY: program->WriteOPCode(OpCode::MULTIPLY); break;
	case ASTOperator::DIVIDE: program->WriteOPCode(OpCode::DIVIDE); break;
	case ASTOperator::MOD: program->WriteOPCode(OpCode::MOD); break;
	case ASTOperator::LESS: program->WriteOPCode(OpCode::LESS); break;
	case ASTOperator::GREATER: program->WriteOPCode(OpCode::GREATER); break;
	case ASTOperator::LESS_EQUALS: program->WriteOPCode(OpCode::LESS_EQUAL); break;
	case ASTOperator::GREATER_EQUALS: program->WriteOPCode(OpCode::GREATER_EQUAL); break;
	case ASTOperator::EQUALS: program->WriteOPCode(OpCode::EQUALS); break;
	case ASTOperator::NOT_EQUALS: program->WriteOPCode(OpCode::NOT_EQUALS); break;
	case ASTOperator::LOGICAL_OR: program->WriteOPCode(OpCode::LOGICAL_OR); break;
	case ASTOperator::LOGICAL_AND: program->WriteOPCode(OpCode::LOGICAL_AND); break;
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
		return TypeInfo(INVALID_ID, 0);
	}

	return TypeInfo(INVALID_ID, 0);
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

void ASTExpressionUnaryUpdate::EmitCode(Program* program)
{
	expr->EmitCode(program);
	program->AddUnaryUpdateCommand((uint8)op, !isStatement);
}

TypeInfo ASTExpressionUnaryUpdate::GetTypeInfo(Program* program)
{
	return expr->GetTypeInfo(program);
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

void ASTExpressionBreak::EmitCode(Program* program)
{
	program->WriteOPCode(OpCode::BREAK);
}

TypeInfo ASTExpressionBreak::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

void ASTExpressionContinue::EmitCode(Program* program)
{
	program->WriteOPCode(OpCode::CONTINUE);
}

TypeInfo ASTExpressionContinue::GetTypeInfo(Program* program)
{
	return TypeInfo(INVALID_ID, 0);
}

void ASTExpressionStaticFunctionCall::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	program->AddStaticFunctionCallCommand(classID, functionID, !isStatement);
}

TypeInfo ASTExpressionStaticFunctionCall::GetTypeInfo(Program* program)
{
	functionID = functionID == INVALID_ID ? program->GetClass(classID)->GetFunctionID(functionName, argExprs) : functionID;
	return program->GetClass(classID)->GetFunction(functionID)->returnInfo;
}

bool ASTExpressionStaticFunctionCall::Resolve(Program* program)
{
	functionID = functionID == INVALID_ID ? program->GetClass(classID)->GetFunctionID(functionName, argExprs) : functionID;
	return true;
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

void ASTExpressionStaticVariable::EmitCode(Program* program)
{
	if (isStatement) return;
	program->AddPushStaticVariableCommand(classID, offset, typeInfo.type, typeInfo.pointerLevel, false);
}

TypeInfo ASTExpressionStaticVariable::GetTypeInfo(Program* program)
{
	return typeInfo;
}

bool ASTExpressionStaticVariable::Resolve(Program* program)
{
	if(offset == UINT64_MAX)
		offset = program->GetClass(classID)->CalculateStaticOffset(program, members, &typeInfo);
	return offset != UINT64_MAX;
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

void ASTExpressionDeclareObjectWithConstructor::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	program->AddDeclareObjectWithConstructorCommand(type, functionID, slot);
}

TypeInfo ASTExpressionDeclareObjectWithConstructor::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 0);
}

bool ASTExpressionDeclareObjectWithConstructor::Resolve(Program* program)
{
	Class* cls = program->GetClass(type);
	functionID = cls->GetFunctionID(cls->GetName(), argExprs);
	return true;
}

void ASTExpressionDeclareObjectWithAssign::EmitCode(Program* program)
{
	assignExpr->EmitCode(program);
	program->AddDeclareObjectWithAssignCommand(type, slot);
}

TypeInfo ASTExpressionDeclareObjectWithAssign::GetTypeInfo(Program* program)
{
	return TypeInfo(type, 0);
}

void ASTExpressionPushMember::EmitCode(Program* program)
{
	if (isStatement) return;

	expr->EmitCode(program);

	program->AddPushMemberCommand(typeInfo.type, typeInfo.pointerLevel, offset, false);
}

TypeInfo ASTExpressionPushMember::GetTypeInfo(Program* program)
{
	if (!members.empty())
	{
		TypeInfo exprTypeInfo = expr->GetTypeInfo(program);
		offset = program->GetClass(exprTypeInfo.type)->CalculateMemberOffset(program, members, &typeInfo);
	}
	return typeInfo;
}

bool ASTExpressionPushMember::Resolve(Program* program)
{
	if(!members.empty())
	{
		TypeInfo exprTypeInfo = expr->GetTypeInfo(program);
		offset = program->GetClass(exprTypeInfo.type)->CalculateMemberOffset(program, members, &typeInfo);
	}
	return offset != UINT64_MAX;
}

void ASTExpressionMemberFunctionCall::EmitCode(Program* program)
{
	for (uint32 i = 0; i < argExprs.size(); i++)
		argExprs[i]->EmitCode(program);

	objExpr->EmitCode(program);
	TypeInfo objTypeInfo = objExpr->GetTypeInfo(program);
	if (objTypeInfo.pointerLevel == 1) 
		program->WriteOPCode(OpCode::DEREFERENCE);

	program->AddMemberFunctionCallCommand(objTypeInfo.type, functionID, !isStatement);
}

TypeInfo ASTExpressionMemberFunctionCall::GetTypeInfo(Program* program)
{
	TypeInfo objTypeInfo = objExpr->GetTypeInfo(program);
	Class* objClass = program->GetClass(objTypeInfo.type);
	functionID = (functionID == INVALID_ID) ? objClass->GetFunctionID(functionName, argExprs) : functionID;
	return objClass->GetFunction(functionID)->returnInfo;
}

bool ASTExpressionMemberFunctionCall::Resolve(Program* program)
{
	TypeInfo objTypeInfo = objExpr->GetTypeInfo(program);
	Class* objClass = program->GetClass(objTypeInfo.type);
	functionID = (functionID == INVALID_ID) ? objClass->GetFunctionID(functionName, argExprs) : functionID;
	return true;
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

void ASTExpressionDeclareReference::EmitCode(Program* program)
{
	TypeInfo assignTypeInfo = assignExpr->GetTypeInfo(program);
	if (assignTypeInfo.type != type || assignTypeInfo.pointerLevel != pointerLevel)
	{
		return;
	}

	assignExpr->EmitCode(program);
	program->AddDeclareReferenceCommand(slot);
}

TypeInfo ASTExpressionDeclareReference::GetTypeInfo(Program* program)
{
	return TypeInfo(type, pointerLevel);
}

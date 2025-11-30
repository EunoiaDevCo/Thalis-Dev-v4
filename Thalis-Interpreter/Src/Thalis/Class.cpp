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

	if (function->name.find('~') != std::string::npos)
	{
		m_Destructor = function;
	}

	if (function->name == "operator=" && function->parameters.size() == 1 &&
		function->parameters[0].type.type == m_ID &&
		function->parameters[0].type.pointerLevel == 0)
	{
		m_AssignSTFunction = function;
	}

	if (function->name == m_BaseName && function->parameters.size() == 1 &&
		function->parameters[0].type.type == m_ID &&
		function->parameters[0].type.pointerLevel == 0)
	{
		m_CopyConstructor = function;
	}

	if (function->name == m_BaseName && function->parameters.empty())
	{
		m_DefaultConstructor = function;
	}
}

Function* Class::GetFunction(uint16 id)
{
	return m_FunctionMap[id];
}

static int32 GetConversionScore(Program* program, const TypeInfo& from, const TypeInfo& to, uint16* castFunctionID, bool checkParamConversion)
{
	if (from.type == to.type)
		return 0; // exact match

	if (!Value::IsPrimitiveType(from.type) && !Value::IsPrimitiveType(to.type))
	{
		Class* fromClass = program->GetClass(from.type);
		if (fromClass->InheritsFrom(to.type)) return 1;
	}

	*castFunctionID = INVALID_ID;

	if (!Value::IsPrimitiveType(to.type) && to.pointerLevel == 0 && checkParamConversion)
	{
		Class* toClass = program->GetClass(to.type);
		std::vector<ASTExpression*> args;
		args.push_back(new ASTExpressionDummy(from));
		std::vector<uint16> cf;
		uint16 functionID = toClass->GetFunctionID(toClass->GetName(), args, cf, false);
		if (functionID == INVALID_ID) return -1;
		*castFunctionID = functionID;
		return 2;
	}

	if (from.pointerLevel != to.pointerLevel)
		return -1; // pointer mismatch not allowed

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

uint16 Class::GetFunctionID(const std::string& name, const std::vector<ASTExpression*>& args, std::vector<uint16>& castFunctionIDs, bool checkParamConversion)
{
	castFunctionIDs.clear();
	Program* program = Program::GetCompiledProgram();

	// First try exact signature match
	std::string exactSignature = Function::GenerateSignatureFromArgs(program, name, args);
	auto it = m_FunctionDefinitionMap.find(exactSignature);
	if (it != m_FunctionDefinitionMap.end())
	{
		for (uint32 i = 0; i < args.size(); i++)
			castFunctionIDs.push_back(INVALID_ID);
		return it->second;
	}

	Function* bestFunc = nullptr;
	uint32 bestID = INVALID_ID;
	int bestScore = INT_MAX;

	// Now try to find the best compatible function
	for (const auto& [sig, id] : m_FunctionDefinitionMap)
	{
		std::vector<uint16> argCastFunctionIDs;

		// Check if signature starts with "<name>-"
		if (sig.rfind(name + "-", 0) != 0)
			continue;

		Function* func = FindFunctionBySignature(sig);
		if (!func || func->parameters.size() != args.size())
			continue;

		int32 totalScore = 0;
		bool compatible = true;

		for (size_t i = 0; i < args.size(); ++i)
		{
			ASTExpression* arg = args[i];
			TypeInfo argType = arg->GetTypeInfo(program);
			const FunctionParameter& param = func->parameters[i];

			uint16 castFunctionID = INVALID_ID;
			int score = GetConversionScore(program, argType, param.type, &castFunctionID, checkParamConversion);
			if (score < 0) { compatible = false; break; }
			totalScore += score;
			argCastFunctionIDs.push_back(castFunctionID);
		}

		if (compatible && totalScore < bestScore)
		{
			bestFunc = func;
			bestID = id;
			bestScore = totalScore;
			castFunctionIDs = argCastFunctionIDs;
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

void Class::AddMemberField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, const std::vector<std::pair<uint32,
	std::string>>& dimensions, const std::string& templateTypeName, TemplateInstantiationCommand* command)
{
	if (HasBaseClass())
		offset += m_BaseClass->GetSize();

	ClassField field;
	field.name = name;
	field.type.type = type;
	field.type.pointerLevel = pointerLevel;
	field.offset = offset;
	field.size = size;
	field.numDimensions = dimensions.size();
	field.instantiationCommand = command;
	field.templateTypeName = templateTypeName;
	for (uint32 i = 0; i < field.numDimensions; i++)
		field.dimensions[i] = dimensions[i];

	m_MemberFields.push_back(field);
}

void Class::AddStaticField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, const std::vector<std::pair<uint32, std::string>>& dimensions, ASTExpression* initializeExpr)
{
	ClassField field;
	field.name = name;
	field.type.type = type;
	field.type.pointerLevel = pointerLevel;
	field.offset = offset;
	field.size = size;
	field.initializeExpr = initializeExpr;
	field.numDimensions = dimensions.size();
	field.instantiationCommand = nullptr;
	for (uint32 i = 0; i < field.numDimensions; i++)
		field.dimensions[i] = dimensions[i];

	m_StaticFields.push_back(field);
}

void Class::EmitCode(Program* program)
{
	if (IsTemplateClass()) 
		return;

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
			ASTExpressionStaticVariable* variableExpr = new ASTExpressionStaticVariable(m_ID, field.offset, field.type, field.dimensions > 0);
			ASTExpressionSet* setExpr = new ASTExpressionSet(variableExpr, field.initializeExpr);
			setExpr->EmitCode(program);
		}
	}
}

uint64 Class::CalculateMemberOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo, bool* isArray, uint32 currentMember, uint32 currentOffset)
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
					*isArray = member.numDimensions > 0;
					
					return memberOffset;
				}

				// Recurse into sub-class
				Class* subClass = Program::GetCompiledProgram()->GetClass(member.type.type);
				if (!subClass)
					throw std::runtime_error("Invalid class type for member: " + member.name);

				return subClass->CalculateMemberOffset(program, members, typeInfo, isArray, i + 1, memberOffset);
			}
		}

		if (HasBaseClass())
		{
			uint64 baseOffset = currentOffset; // Base data always starts at 0 offset for the derived object
			uint64 result = m_BaseClass->CalculateMemberOffset(program, members, typeInfo, isArray, currentMember, baseOffset);

			if (result != UINT64_MAX)
				return result; // Found in base class
		}
	}

	// No member found
	return UINT64_MAX;
}

uint64 Class::CalculateStaticOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo, bool* isArray)
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
		uint8 pointerLevel = 0;
		bool isArray_ = false;
		for (uint32 i = 0; i < m_StaticFields.size(); i++)
		{
			const ClassField& field = m_StaticFields[i];
			if (members[0] == field.name)
			{
				baseOffset = field.offset;
				type = field.type.type;
				pointerLevel = field.type.pointerLevel;
				isArray_ = field.numDimensions > 0;
				break;
			}
		}

		if (type == INVALID_ID) return UINT64_MAX;

		if (Value::IsPrimitiveType(type))
		{
			typeInfo->type = type;
			typeInfo->pointerLevel = pointerLevel;
			*isArray = isArray_;
			return baseOffset;
		}

		return baseOffset + program->GetClass(type)->CalculateMemberOffset(program, members, typeInfo, isArray, 1);
	}

	return UINT64_MAX;
}

void* Class::GetStaticData(uint64 offset) const
{
	return (uint8*)m_StaticData.data + offset;
}

uint16 Class::InstantiateTemplate(Program* program, const TemplateInstantiation& instantiation)
{
	if (!IsTemplateClass()) return INVALID_ID;
	if (m_TemplateDefinition.parameters.size() != instantiation.args.size()) return INVALID_ID;

	std::string name = GenerateTemplateClassName(program, m_Name, instantiation);
	uint16 classID = program->GetClassID(name);
	if (classID != INVALID_ID)
	{
		return classID;
	}

	Class* cls = new Class(name);
	classID = program->AddClass(cls);
	cls->m_IsTemplateInstance = true;
	
	uint64 memberOffset = 0;
	for (uint32 i = 0; i < m_MemberFields.size(); i++)
	{
		const ClassField& member = m_MemberFields[i];
		TypeInfo typeInfo = member.type;

		if (member.instantiationCommand)
		{
			typeInfo.type = ExecuteInstantiationCommand(program, member.instantiationCommand, instantiation);
		}
		else if (typeInfo.type == (uint16)ValueType::TEMPLATE_TYPE)
		{
			uint32 index = InstantiateTemplateGetIndex(program, member.templateTypeName);
			typeInfo.type = instantiation.args[index].value;
			typeInfo.pointerLevel += instantiation.args[index].pointerLevel;
		}

		uint64 typeSize = (typeInfo.pointerLevel > 0) ? sizeof(void*) : program->GetTypeSize(typeInfo.type);

		std::vector<std::pair<uint32, std::string>> dimensions;
		for (uint32 j = 0; j < member.numDimensions; j++)
		{
			if (!member.dimensions[j].second.empty())
			{
				uint32 index = InstantiateTemplateGetIndex(program, member.dimensions->second);
				uint32 arrayLength = instantiation.args[index].value;
				dimensions.push_back(std::make_pair(arrayLength, ""));
			}
			else
			{
				dimensions.push_back(std::make_pair(member.dimensions[j].first, ""));
			}

			typeSize *= dimensions[j].first;
		}

		cls->AddMemberField(member.name, typeInfo.type, typeInfo.pointerLevel, memberOffset, typeSize, dimensions, "");
		memberOffset += typeSize;
	}

	cls->SetSize(memberOffset);
	cls->SetStaticDataSize(0);

	for (uint32 i = 0; i < m_FunctionMap.size(); i++)
	{
		Function* function = m_FunctionMap[i];
		Function* injectedFunction = InstantiateTemplateInjectFunction(program, function, name, instantiation, cls);
		cls->AddFunction(injectedFunction);
	}

	return classID;
}

//void Class::AddInstantiationCommand(TemplateInstantiationCommand* command)
//{
//	m_InstantiationCommands.push_back(command);
//}

int32 Class::InstantiateTemplateGetIndex(Program* program, const std::string& templateTypeName)
{
	for (uint32 i = 0; i < m_TemplateDefinition.parameters.size(); i++)
	{
		const TemplateParameter& param = m_TemplateDefinition.parameters[i];
		if (param.name == templateTypeName)
			return i;
	}

	return -1;
}

bool Class::InheritsFrom(uint16 type) const
{
	if (HasBaseClass())
	{
		if (m_BaseClass->m_ID == type) return true;
		return m_BaseClass->InheritsFrom(type);
	}

	return false;
}

Function* Class::InstantiateTemplateInjectFunction(Program* program, Function* templatedFunction, const std::string& templatedTypeName, const TemplateInstantiation& instantiation, Class* templatedClass)
{
	Function* injectedFunction = new Function();
	injectedFunction->accessModifier = templatedFunction->accessModifier;
	injectedFunction->isStatic = templatedFunction->isStatic;
	injectedFunction->name = templatedFunction->name;
	injectedFunction->returnInfo = templatedFunction->returnInfo;
	injectedFunction->numLocals = templatedFunction->numLocals;

	if (templatedFunction->name == m_Name) //Constructor so set new templated name
		injectedFunction->name = templatedTypeName;
	else
		injectedFunction->name = templatedFunction->name;

	if (injectedFunction->returnInfo.type == (uint16)ValueType::TEMPLATE_TYPE)
	{
		uint32 index = InstantiateTemplateGetIndex(program, templatedFunction->returnTemplateTypeName);
		injectedFunction->returnInfo.type = instantiation.args[index].value;
	}

	for (uint32 j = 0; j < templatedFunction->parameters.size(); j++)
	{
		FunctionParameter param = templatedFunction->parameters[j];
		if (param.type.type == (uint16)ValueType::TEMPLATE_TYPE)
		{
			uint32 index = InstantiateTemplateGetIndex(program, param.templateTypeName);
			param.type.type = instantiation.args[index].value;
			param.type.pointerLevel += instantiation.args[index].pointerLevel;
		}

		if (param.type.type == m_ID)
			param.type.type = templatedClass->GetID();

		if (param.instantiationCommand)
		{
			param.type.type = ExecuteInstantiationCommand(program, param.instantiationCommand, instantiation);
		}

		param.instantiationCommand = nullptr;

		injectedFunction->parameters.push_back(param);
	}

	for (uint32 j = 0; j < templatedFunction->body.size(); j++)
	{
		ASTExpression* injectedExpr = templatedFunction->body[j]->InjectTemplateType(program, this, instantiation, templatedClass);
		injectedFunction->body.push_back(injectedExpr);
	}

	return injectedFunction;
}

void Class::ExecuteInstantiationCommands(Program* program, const TemplateInstantiation& instantiation)
{
	for (uint32 i = 0; i < m_InstantiationCommands.size(); i++)
		ExecuteInstantiationCommand(program, m_InstantiationCommands[i], instantiation);
}

uint16 Class::ExecuteInstantiationCommand(Program* program, TemplateInstantiationCommand* command, const TemplateInstantiation& instantiation)
{
	TemplateInstantiation result;
	for (uint32 i = 0; i < command->args.size(); i++)
	{
		const TemplateInstantiationCommandArg& arg = command->args[i];
		if (arg.type == 0)
		{
			if (arg.arg.type == TemplateParameterType::TEMPLATE_TYPE)
			{
				TemplateArgument injectedArg;
				injectedArg.type = TemplateParameterType::TYPE;
				uint32 index = InstantiateTemplateGetIndex(program, arg.arg.templateTypeName);
				injectedArg.value = instantiation.args[index].value;
				injectedArg.pointerLevel = instantiation.args[index].pointerLevel;
				result.args.push_back(injectedArg);
			}
			else if (arg.arg.type == TemplateParameterType::INT)
			{
				if (!arg.arg.templateTypeName.empty())
				{
					TemplateArgument injectedArg;
					injectedArg.type = TemplateParameterType::INT;
					uint32 index = InstantiateTemplateGetIndex(program, arg.arg.templateTypeName);
					injectedArg.value = instantiation.args[index].value;
					result.args.push_back(injectedArg);
				}
				else
				{
					result.args.push_back(arg.arg);
				}
			}
			else
			{
				result.args.push_back(arg.arg);
			}
		}
		else if (arg.type == 1)
		{
			TemplateArgument injectedArg;
			injectedArg.type = TemplateParameterType::TYPE;
			injectedArg.value = ExecuteInstantiationCommand(program, arg.command, instantiation);
			injectedArg.pointerLevel = 0;
			result.args.push_back(injectedArg);
		}
	}

	Class* cls = program->GetClass(command->type);
	uint16 type = cls->InstantiateTemplate(program, result);
	return type;
}

void Class::BuildVTable()
{
	VTable* vtable = new VTable();

	if (HasBaseClass())
		*vtable = *m_BaseClass->GetVTable();

	for (uint32 i = 0; i < m_FunctionMap.size(); i++)
	{
		Function* function = m_FunctionMap[i];
		if (!function->isVirtual)
			continue;

		int32 overrideIndex = -1;
		if (HasBaseClass())
		{
			std::vector<TypeInfo> parameters;
			for (uint32 j = 0; j < function->parameters.size(); j++)
				parameters.push_back(function->parameters[j].type);

			overrideIndex = m_BaseClass->GetVTable()->FindSlot(function->name, parameters);
		}

		if (overrideIndex >= 0)
		{
			// Replace base function
			vtable->functions[overrideIndex] = function;
		}
		else
		{
			// New virtual entry
			vtable->functions.push_back(function);
		}
	}

	m_VTable = vtable;
}

static std::string GetPrimitiveTypeName(ValueType type)
{
	switch (type)
	{
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

	case ValueType::BOOL:    return "bool";
	case ValueType::CHAR:    return "char";
	case ValueType::VOID_T:  return "void";

	default:                 return "unknown";
	}
}

std::string Class::GenerateTemplateClassName(Program* program, const std::string& className, const TemplateInstantiation& instantiation)
{
	std::string name = className + "<";
	for (uint32 i = 0; i < instantiation.args.size(); i++)
	{
		const TemplateArgument& arg = instantiation.args[i];
		if (arg.type == TemplateParameterType::TYPE)
		{
			name += "Type=";
			if (Value::IsPrimitiveType(arg.value))
				name += GetPrimitiveTypeName((ValueType)arg.value);
			else
				name += program->GetClass(arg.value)->GetName();
		}
		else if (arg.type == TemplateParameterType::INT)
		{
			name += "Int=";
			name += std::to_string(arg.value);
		}

		if ((i + 1) < instantiation.args.size())
		{
			name += ",";
		}
	}

	name += ">";

	return name;
}

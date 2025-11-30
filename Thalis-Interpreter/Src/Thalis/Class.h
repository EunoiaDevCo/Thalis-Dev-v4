#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Function.h"
#include "Value.h"
#include "TypeInfo.h"
#include "Template.h"
#include "VTable.h"

struct StaticData
{
	void* data;
	uint64 size;
};

struct ClassField
{
	TypeInfo type;
	uint64 offset;
	uint64 size;
	std::string name;
	ASTExpression* initializeExpr;
	std::pair<uint32, std::string> dimensions[MAX_ARRAY_DIMENSIONS];
	uint8 numDimensions;
	std::string templateTypeName;
	TemplateInstantiationCommand* instantiationCommand;
};

class Program;
class Class
{
public:
	Class(const std::string& name, Class* baseClass = nullptr) :
		m_Name(name), m_BaseName(name), m_BaseClass(baseClass), m_NextFunctionID(0),
		m_Destructor(nullptr), m_AssignSTFunction(nullptr), m_CopyConstructor(nullptr), m_DefaultConstructor(nullptr) { }

	std::string GetName() const;

	void AddFunction(Function* function);
	Function* GetFunction(uint16 id);
	uint16 GetFunctionID(const std::string& name, const std::vector<ASTExpression*>& args, std::vector<uint16>& castFunctionIDs, bool checkParamConversion = true);
	Function* FindFunctionBySignature(const std::string& signature);

	void AddMemberField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, const std::vector<std::pair<uint32, std::string>>& dimensions, const std::string& templateTypeName, TemplateInstantiationCommand * command = nullptr);
	void AddStaticField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, const std::vector<std::pair<uint32, std::string>>& dimensions, ASTExpression* initializeExpr);

	inline const std::vector<ClassField>& GetMemberFields() const { return m_MemberFields; }
	inline const std::vector<ClassField>& GetStaticFields() const { return m_StaticFields; }

	void EmitCode(Program* program);
	void InitStaticData(Program* program);

	inline void SetSize(uint64 size) { if (HasBaseClass()) m_Size = m_BaseClass->GetSize() + size; else m_Size = size; }
	inline uint64 GetSize() const { return m_Size; }
	inline void SetStaticDataSize(uint64 size) { m_StaticData.size = size; }

	inline uint16 GetID() const { return m_ID; }
	inline void SetID(uint16 id) { m_ID = id; }

	uint64 CalculateMemberOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo, bool* isArray, uint32 currentMember = 0, uint32 currentOffset = 0);
	uint64 CalculateStaticOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo, bool* isArray);
	void* GetStaticData(uint64 offset) const;

	uint16 InstantiateTemplate(Program* program, const TemplateInstantiation& instantiation);
	//void AddInstantiationCommand(TemplateInstantiationCommand* command);
	int32 InstantiateTemplateGetIndex(Program* program, const std::string& templateTypeName);

	bool InheritsFrom(uint16 type) const;

	inline bool HasDestructor() const { return m_Destructor != nullptr; }
	inline bool HasAssignSTFunction() const { return m_AssignSTFunction != nullptr; }
	inline bool HasCopyConstructor() const { return m_CopyConstructor != nullptr; }
	inline bool HasDefaultConstructor() const { return m_DefaultConstructor != nullptr; }

	inline bool IsTemplateClass() const { return m_TemplateDefinition.HasTemplate(); }
	inline bool IsTemplateInstance() const { return m_IsTemplateInstance; }
	inline void SetTemplateDefinition(const TemplateDefinition& definition) { m_TemplateDefinition = definition; }
	inline const TemplateDefinition& GetTemplateDefinition() const { return m_TemplateDefinition; }

	inline Function* GetDestructor() const { return m_Destructor; }
	inline Function* GetAssignSTFunction() const { return m_AssignSTFunction; }
	inline Function* GetCopyConstructor() const { return m_CopyConstructor; }
	inline Function* GetDefaultConstructor() const { return m_DefaultConstructor; }

	inline bool HasBaseClass() const { return m_BaseClass != nullptr; }
	inline VTable* GetVTable() const { return m_VTable; }

	uint16 ExecuteInstantiationCommand(Program* program, TemplateInstantiationCommand* command, const TemplateInstantiation& instantiation);

	void BuildVTable();
private:
	Function* InstantiateTemplateInjectFunction(Program* program, Function* templatedFunction, const std::string& templatedTypeName,
		const TemplateInstantiation& instantiation, Class* templatedClass);

	void ExecuteInstantiationCommands(Program* program, const TemplateInstantiation& instantiation);
	std::string GenerateTemplateClassName(Program* program, const std::string& className, const TemplateInstantiation& instantiation);
private:
	std::string m_Name;
	std::string m_BaseName;
	uint16 m_ID;
	TemplateDefinition m_TemplateDefinition;
	bool m_IsTemplateInstance;

	Class* m_BaseClass;

	std::unordered_map<std::string, std::vector<Function*>> m_Functions;
	std::unordered_map<std::string, uint32> m_FunctionDefinitionMap;
	std::vector<Function*> m_FunctionMap; //FunctionID is the index
	uint16 m_NextFunctionID;

	Function* m_Destructor;
	Function* m_AssignSTFunction;
	Function* m_CopyConstructor;
	Function* m_DefaultConstructor;

	std::vector<TemplateInstantiationCommand*> m_InstantiationCommands;

	uint64 m_Size;

	std::vector<ClassField> m_MemberFields;
	std::vector<ClassField> m_StaticFields;

	StaticData m_StaticData;
	VTable* m_VTable;
};

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Function.h"
#include "Value.h"

#include "TypeInfo.h"

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
	uint32 dimensions[MAX_ARRAY_DIMENSIONS];
	uint8 numDimensions;
};

class Program;
class Class
{
public:
	Class(const std::string& name) :
		m_Name(name), m_NextFunctionID(0), m_Destructor(nullptr) { }

	std::string GetName() const;

	void AddFunction(Function* function);
	Function* GetFunction(uint16 id);
	uint16 GetFunctionID(const std::string& name, const std::vector<ASTExpression*>& args);
	Function* FindFunctionBySignature(const std::string& signature);

	void AddMemberField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, uint32* dimensions, uint8 numDimensions);
	void AddStaticField(const std::string& name, uint16 type, uint8 pointerLevel, uint64 offset, uint64 size, uint32* dimensions, uint8 numDimensions, ASTExpression* initializeExpr);

	inline const std::vector<ClassField>& GetMemberFields() const { return m_MemberFields; }
	inline const std::vector<ClassField>& GetStaticFields() const { return m_StaticFields; }

	void EmitCode(Program* program);
	void InitStaticData(Program* program);

	inline void SetSize(uint64 size) { m_Size = size; }
	inline uint64 GetSize() const { return m_Size; }
	inline void SetStaticDataSize(uint64 size) { m_StaticData.size = size; }

	inline uint16 GetID() const { return m_ID; }
	inline void SetID(uint16 id) { m_ID = id; }

	uint64 CalculateMemberOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo, bool* isArray, uint32 currentMember = 0, uint32 currentOffset = 0);
	uint64 CalculateStaticOffset(Program* program, const std::vector<std::string>& members, TypeInfo* typeInfo, bool* isArray);
	void* GetStaticData(uint64 offset) const;

	inline Function* GetDestructor() const { return m_Destructor; }
private:

private:
	std::string m_Name;
	uint16 m_ID;

	std::unordered_map<std::string, std::vector<Function*>> m_Functions;
	std::unordered_map<std::string, uint32> m_FunctionDefinitionMap;
	std::vector<Function*> m_FunctionMap; //FunctionID is the index
	uint16 m_NextFunctionID;

	Function* m_Destructor;

	uint64 m_Size;

	std::vector<ClassField> m_MemberFields;
	std::vector<ClassField> m_StaticFields;

	StaticData m_StaticData;
};

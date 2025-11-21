#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Common.h"
#include "TypeInfo.h"

class Scope
{
public:
    Scope(Scope* parent = nullptr);
    uint16 AddLocal(const std::string& name, const TypeInfo& typeInfo);
    uint16 Resolve(const std::string& name);

    TypeInfo GetLocalTypeInfo(uint16 slot);

    uint16 GetNumLocals() const;
private:
    Scope* m_Parent;
    Scope* m_FunctionScope;
    std::unordered_map<std::string, uint16> m_Locals;
    std::unordered_map<uint16, TypeInfo> m_VariableTypes;
    uint16 m_LocalCount = 0;
};
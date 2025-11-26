#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Common.h"
#include "TypeInfo.h"
#include "Template.h"

struct ScopeLocalDeclaration
{
    TypeInfo type;
    std::string templateTypeName;
    TemplateInstantiationCommand* command;
    uint16 derivedType;
};

class Scope
{
public:
    Scope(Scope* parent = nullptr);
    uint16 AddLocal(const std::string& name, const TypeInfo& typeInfo, const std::string& templateTypeName, TemplateInstantiationCommand* command = nullptr);
    uint16 Resolve(const std::string& name);

    TypeInfo GetLocalTypeInfo(uint16 slot) const;
    std::string GetLocalTemplateType(uint16 slot) const;
    ScopeLocalDeclaration GetDeclarationInfo(uint16 slot) const;

    uint16 GetNumLocals() const;
private:
    Scope* m_Parent;
    Scope* m_FunctionScope;
    std::unordered_map<std::string, uint16> m_Locals;
    std::unordered_map<uint16, ScopeLocalDeclaration> m_VariableTypes;
    uint16 m_LocalCount = 0;
};
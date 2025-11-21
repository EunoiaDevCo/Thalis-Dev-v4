#include "Scope.h"

Scope::Scope(Scope* parent)
    : m_Parent(parent)
{
    m_FunctionScope = parent ? parent->m_FunctionScope : this;
}

uint16 Scope::AddLocal(const std::string& name, const TypeInfo& typeInfo)
{
    // This block scope's locals map is only for checking shadowing
    if (m_Locals.find(name) != m_Locals.end())
    {
        // redeclaration in the same block
        return m_Locals[name];
    }

    // This function’s local slot index
    uint16 slot = m_FunctionScope->m_LocalCount++;

    // Track this name in THIS block only for shadowing
    m_Locals[name] = slot;
    m_VariableTypes[slot] = typeInfo;

    return slot;
}

uint16 Scope::Resolve(const std::string& name)
{
    Scope* scope = this;

    while (scope != nullptr)
    {
        auto it = scope->m_Locals.find(name);
        if (it != scope->m_Locals.end())
        {
            return it->second;
        }

        scope = scope->m_Parent;
    }

    return INVALID_ID;
}

TypeInfo Scope::GetLocalTypeInfo(uint16 slot)
{
    const auto&& it = m_VariableTypes.find(slot);
    if (it == m_VariableTypes.end())
    {
        if (m_Parent) return m_Parent->GetLocalTypeInfo(slot);
        return TypeInfo(INVALID_ID, 0);
    }

    return it->second;
}

uint16 Scope::GetNumLocals() const
{
    return m_LocalCount;
}

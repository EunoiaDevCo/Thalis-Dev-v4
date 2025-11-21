#pragma once

#include <vector>
#include "Value.h"

class Frame
{
public:
    Frame(uint16 numLocals = 0)
        : locals(numLocals)
    {
    }

    inline Value& GetLocal(uint16 slot)
    {
        return locals[slot];
    }

    inline void DeclareLocal(uint16 slot, const Value& value)
    {
        locals[slot] = value;
    }

    inline void Reset(uint16 numLocals)
    {
        if (locals.size() < numLocals)
        {
            locals.resize(numLocals);
        }
    }
private:
    std::vector<Value> locals;
};
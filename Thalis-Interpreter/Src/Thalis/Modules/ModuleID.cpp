#include "ModuleID.h"
#include "IOModule.h"

TypeInfo Module::GetFunctionReturnInfo(uint16 moduleID, uint16 function)
{
    switch (moduleID)
    {
    case IO_MODULE_ID: return IOModule::GetFunctionReturnInfo(function);
    }
}

TypeInfo Module::GetConstantTypeInfo(uint16 moduleID, uint16 constant)
{
    switch (moduleID)
    {
    case IO_MODULE_ID: return IOModule::GetConstantTypeInfo(constant);
    }
}

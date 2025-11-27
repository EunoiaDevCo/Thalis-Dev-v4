#include "ModuleID.h"
#include "IOModule.h"
#include "MathModule.h"
#include "WindowModule.h"
#include "GLModule.h"
#include "FSModule.h"
#include "MemModule.h"

TypeInfo Module::GetFunctionReturnInfo(uint16 moduleID, uint16 function)
{
    switch (moduleID)
    {
    case IO_MODULE_ID: return IOModule::GetFunctionReturnInfo(function);
    case MATH_MODULE_ID: return MathModule::GetFunctionReturnInfo(function);
    case WINDOW_MODULE_ID: return WindowModule::GetFunctionReturnInfo(function);
    case GL_MODULE_ID: return GLModule::GetFunctionReturnInfo(function);
    case FS_MODULE_ID: return FSModule::GetFunctionReturnInfo(function);
    case MEM_MODULE_ID: return MemModule::GetFunctionReturnInfo(function);
    }
}

TypeInfo Module::GetConstantTypeInfo(uint16 moduleID, uint16 constant)
{
    switch (moduleID)
    {
    case IO_MODULE_ID: return IOModule::GetConstantTypeInfo(constant);
    case MATH_MODULE_ID: return MathModule::GetConstantTypeInfo(constant);
    case WINDOW_MODULE_ID: return WindowModule::GetConstantTypeInfo(constant);
    case GL_MODULE_ID: return GLModule::GetConstantTypeInfo(constant);
    case FS_MODULE_ID: return FSModule::GetConstantTypeInfo(constant);
    case MEM_MODULE_ID: return MemModule::GetConstantTypeInfo(constant);
    }
}

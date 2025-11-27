#include "FSModule.h"
#include "../Program.h"
#include <fstream>

static std::ifstream g_OpenFiles[16];
static std::vector<uint32> g_FreeFileIDs;
static uint32 g_NexFileID = 1;


bool FSModule::Init()
{
    return true;
}

Value FSModule::CallFunction(Program* program, uint16 function, const std::vector<Value>& args)
{
    switch ((FSModuleFunction)function)
    {
    case FSModuleFunction::READ_TEXT_FILE: {
        FILE* file = fopen(args[0].GetCString(), "rb");
        if (!file)
            return Value::MakeNULL((uint16)ValueType::CHAR, 1);

        fseek(file, 0, SEEK_END);
        uint64 size = ftell(file);
        fseek(file, 0, SEEK_SET);
        uint8* data = (uint8*)program->GetHeapAllocator()->Alloc(size + 1);
        char* characters = (char*)(data);
        fread(characters, 1, size, file);
        characters[size] = 0;
        fclose(file);

        Value pointer = Value::MakePointer((uint16)ValueType::CHAR, 1, characters, program->GetStackAllocator());
        return pointer;
    } break;
    case FSModuleFunction::READ_BINARY_FILE: {
        FILE* file = fopen(args[0].GetCString(), "rb");
        if (!file)
            return Value::MakeNULL((uint16)ValueType::UINT8, 1);

        fseek(file, 0, SEEK_END);
        uint64 size = ftell(file);
        fseek(file, 0, SEEK_SET);
        uint8* bytes = (uint8*)program->GetHeapAllocator()->Alloc(size);
        fread(bytes, 1, size, file);
        fclose(file);

        Value data = Value::MakePointer((uint16)ValueType::UINT8, 1, bytes, program->GetStackAllocator());
        return data;
    } break;
    case FSModuleFunction::OPEN_FILE: {
        std::string path = args[0].GetString();
        int32 lastSlashIndex = path.find_last_of("/");
        std::string relPath = lastSlashIndex == -1 ? "" : path.substr(0, lastSlashIndex + 1);

        uint32 fileID;
        if (!g_FreeFileIDs.empty())
        {
            fileID = g_FreeFileIDs.back();
            g_FreeFileIDs.pop_back();
        }
        else
        {
            fileID = g_NexFileID++;
        }

        g_OpenFiles[fileID - 1].open(path.c_str());

        if (!g_OpenFiles[fileID - 1].good())
        {
            g_FreeFileIDs.push_back(fileID);
            return Value::MakeUInt32(0, program->GetStackAllocator());
        }

        return Value::MakeUInt32(fileID, program->GetStackAllocator());
    } break;
    case FSModuleFunction::CLOSE_FILE: {
        uint32 fileID = args[0].GetUInt32();
        g_OpenFiles[fileID - 1].close();
        g_FreeFileIDs.push_back(fileID);
        return Value::MakeNULL();
    } break;
    case FSModuleFunction::READ_LINE: {
        uint32 fileID = args[0].GetUInt32();
        char* _Str = (char*)*(void**)args[1].data;
        std::streamsize _Count = args[2].GetUInt64();
        if (!g_OpenFiles[fileID - 1].getline(_Str, _Count))
            return Value::MakeBool(false, program->GetStackAllocator());

        return Value::MakeBool(true, program->GetStackAllocator());
    } break;
    }

    return Value::MakeNULL();
}

Value FSModule::Constant(Program* program, uint16 constant)
{
    return Value::MakeNULL();
}

TypeInfo FSModule::GetFunctionReturnInfo(uint16 function)
{
    switch ((FSModuleFunction)function)
    {
    case FSModuleFunction::READ_TEXT_FILE: return TypeInfo((uint16)ValueType::CHAR, 1);
    case FSModuleFunction::READ_BINARY_FILE: return TypeInfo((uint16)ValueType::UINT8, 1);
    case FSModuleFunction::OPEN_FILE: return TypeInfo((uint16)ValueType::UINT32, 0);
    case FSModuleFunction::CLOSE_FILE: return TypeInfo((uint16)ValueType::VOID_T, 0);
    case FSModuleFunction::READ_LINE: return TypeInfo((uint16)ValueType::BOOL, 0);
    }
}

TypeInfo FSModule::GetConstantTypeInfo(uint16 constant)
{
    return TypeInfo(INVALID_ID, 0);
}

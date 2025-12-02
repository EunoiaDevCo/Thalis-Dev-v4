#include "Parser.h"
#include "Program.h"
#include "Class.h"
#include "Memory/Memory.h"

int main()
{
	Program program;
	Parser parser(&program);
	parser.Parse("Main.tls");
	program.BuildVTables();
	program.Resolve();
	program.EmitCode();

	uint32 pc = program.GetCodeSize();
	uint16 mainClassID = program.GetClassIDWithMainFunction();
	std::vector<ASTExpression*> args;
	std::vector<uint16> castFunctionIDs;
	program.AddStaticFunctionCallCommand(mainClassID, program.GetClass(mainClassID)->GetFunctionID("Main", args, castFunctionIDs), false);
	program.WriteOPCode(OpCode::END);
	program.ExecuteProgram(pc);

	HeapAllocator* heapAllocator = program.GetHeapAllocator();
	Allocator* stackAllocator = program.GetStackAllocator();
	Allocator* initAllocator = program.GetInitializationAllocator();

	std::cout << "Max initialization usage: " << Memory::BytesToKB(initAllocator->GetMaxUsage()) << "KB" << std::endl;
	std::cout << "Max stack usage: " << Memory::BytesToKB(stackAllocator->GetMaxUsage()) << "KB" << std::endl;
	std::cout << "Num heap allocations: " << heapAllocator->GetNumAllocs() << std::endl;
	std::cout << "Num heap frees: " << heapAllocator->GetNumFrees() << std::endl;
	std::cout << "Stack size: " << program.GetStackSize() << std::endl;
	std::cout << "Scope stack size: " << program.GetScopeStackSize() << std::endl;
	std::cout << "Loop stack size: " << program.GetLoopStackSize() << std::endl;
	std::cout << "Code size: " << program.GetCodeSize() << std::endl;
	program.PrintClassCodeSizes();

	while (true);
}
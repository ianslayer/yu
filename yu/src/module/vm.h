#ifndef YU_VM_H
#define YU_VM_H
#include "../core/platform.h"
#include "../core/allocator.h"
namespace yu
{

enum OpCode
{
	OP_ADD,
	OP_SUB,

	
	OP_AND,
	OP_OR,

	
};
	
struct Instruction
{
   	u64 opcode : 8;
	u64 dst : 5;
	u64 src0 : 5;
	u64 src1 : 5;
};

struct RegisterFile
{
	u32 gpr[32];
	u32 flags;
	u32 ip;
	u32 bp;
	u32 sp;
};

struct CPU
{
	RegisterFile registers;

	u8*		memory;
	size_t	memorySize;

	u8*		codeSegmentStart;
	size_t	codeSegmentSize;

	u8*		stackStart;
	size_t	stackSize;
	
	int		instrsPerFrame;
};

struct Interface
{
	
};
	
struct Port
{
};

CPU NewCPU(Allocator* allocator);

void RunCPU(int numInstr)
{
}
	
}

#endif

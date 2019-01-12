/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/


#include "snes9x.h"
#include "memmap.h"

#define CPU								SA1
#define ICPU							SA1
#define Registers						SA1Registers
#define OpenBus							SA1OpenBus
#define S9xGetByte						S9xSA1GetByte
#define S9xGetWord						S9xSA1GetWord
#define S9xSetByte						S9xSA1SetByte
#define S9xSetWord						S9xSA1SetWord
#define S9xSetPCBase					S9xSA1SetPCBase
#define S9xOpcodesM1X1					S9xSA1OpcodesM1X1
#define S9xOpcodesM1X0					S9xSA1OpcodesM1X0
#define S9xOpcodesM0X1					S9xSA1OpcodesM0X1
#define S9xOpcodesM0X0					S9xSA1OpcodesM0X0
#define S9xOpcodesE1					S9xSA1OpcodesE1
#define S9xOpcodesSlow					S9xSA1OpcodesSlow
#define S9xOpcode_IRQ					S9xSA1Opcode_IRQ
#define S9xOpcode_NMI					S9xSA1Opcode_NMI
#define S9xUnpackStatus					S9xSA1UnpackStatus
#define S9xPackStatus					S9xSA1PackStatus
#define S9xFixCycles					S9xSA1FixCycles
#define Immediate8						SA1Immediate8
#define Immediate16						SA1Immediate16
#define Relative						SA1Relative
#define RelativeLong					SA1RelativeLong
#define Absolute						SA1Absolute
#define AbsoluteLong					SA1AbsoluteLong
#define AbsoluteIndirect				SA1AbsoluteIndirect
#define AbsoluteIndirectLong			SA1AbsoluteIndirectLong
#define AbsoluteIndexedIndirect			SA1AbsoluteIndexedIndirect
#define Direct							SA1Direct
#define DirectIndirectIndexed			SA1DirectIndirectIndexed
#define DirectIndirectIndexedLong		SA1DirectIndirectIndexedLong
#define DirectIndexedIndirect			SA1DirectIndexedIndirect
#define DirectIndexedX					SA1DirectIndexedX
#define DirectIndexedY					SA1DirectIndexedY
#define AbsoluteIndexedX				SA1AbsoluteIndexedX
#define AbsoluteIndexedY				SA1AbsoluteIndexedY
#define AbsoluteLongIndexedX			SA1AbsoluteLongIndexedX
#define DirectIndirect					SA1DirectIndirect
#define DirectIndirectLong				SA1DirectIndirectLong
#define StackRelative					SA1StackRelative
#define StackRelativeIndirectIndexed	SA1StackRelativeIndirectIndexed

//#undef CPU_SHUTDOWN
#define SA1_OPCODES

#include "cpuops.cpp"


void S9xSA1MainLoop (void)
{
	if (SA1.Flags & NMI_FLAG)
	{
		if (Memory.FillRAM[0x2200] & 0x10)
		{
			SA1.Flags &= ~NMI_FLAG;
			Memory.FillRAM[0x2301] |= 0x10;

			if (SA1.WaitingForInterrupt)
			{
				SA1.WaitingForInterrupt = FALSE;
				SA1Registers.PCw++;
			}

			S9xSA1Opcode_NMI();
		}
	}

	if (SA1.Flags & IRQ_FLAG)
	{
		if (SA1.IRQActive)
		{
			if (SA1.WaitingForInterrupt)
			{
				SA1.WaitingForInterrupt = FALSE;
				SA1Registers.PCw++;
			}

			if (!SA1CheckFlag(IRQ))
				S9xSA1Opcode_IRQ();
		}
		else
			SA1.Flags &= ~IRQ_FLAG;
	}

	for (int i = 0; i < Timings.SA1Cycles && SA1.Executing; i++)
	{
	#ifdef CPU_SHUTDOWN
		SA1.PBPCAtOpcodeStart = SA1Registers.PBPC;
	#endif

		register uint8				Op;
		register struct SOpcodes	*Opcodes;

		if (SA1.PCBase)
		{
			SA1OpenBus = Op = SA1.PCBase[Registers.PCw];
			Opcodes = SA1.S9xOpcodes;
		}
		else
		{
			Op = S9xSA1GetByte(Registers.PBPC);
			Opcodes = S9xOpcodesSlow;
		}

		if ((SA1Registers.PCw & MEMMAP_MASK) + SA1.S9xOpLengths[Op] >= MEMMAP_BLOCK_SIZE)
		{
			uint32	oldPC = SA1Registers.PBPC;
			S9xSA1SetPCBase(SA1Registers.PBPC);
			SA1Registers.PBPC = oldPC;
			Opcodes = S9xSA1OpcodesSlow;
		}

		Registers.PCw++;
		(*Opcodes[Op].S9xOpcode)();
	}
}


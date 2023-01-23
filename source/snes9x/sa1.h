/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _SA1_H_
#define _SA1_H_

struct SSA1Registers
{
	uint8	DB;
	pair	P;
	pair	A;
	pair	D;
	pair	S;
	pair	X;
	pair	Y;
	PC_t	PC;
};

struct SSA1
{
	struct SOpcodes	*S9xOpcodes;
	uint8	*S9xOpLengths;
	uint8	_Carry;
	uint8	_Zero;
	uint8	_Negative;
	uint8	_Overflow;
	bool8	CPUExecuting;
	uint32	ShiftedPB;
	uint32	ShiftedDB;

	uint32	Flags;
	uint8	*PCBase;
	bool8	IRQActive;
	bool8	Waiting;
	bool8	WaitingForInterrupt;
	uint32	WaitAddress;
	uint32	WaitCounter;
	uint32	PBPCAtOpcodeStart;
	uint8	*WaitByteAddress1;
	uint8	*WaitByteAddress2;

	uint8	*Map[MEMMAP_NUM_BLOCKS];
	uint8	*WriteMap[MEMMAP_NUM_BLOCKS];
	uint8	*BWRAM;

	bool8	Executing;
	bool8	overflow;
	bool8	in_char_dma;
	int16	op1;
	int16	op2;
	int32	arithmetic_op;
	int64	sum;
	uint8	VirtualBitmapFormat;
	uint8	variable_bit_pos;
};

#define SA1CheckCarry()		(SA1._Carry)
#define SA1CheckZero()		(SA1._Zero == 0)
#define SA1CheckIRQ()		(SA1Registers.PL & IRQ)
#define SA1CheckDecimal()	(SA1Registers.PL & Decimal)
#define SA1CheckIndex()		(SA1Registers.PL & IndexFlag)
#define SA1CheckMemory()	(SA1Registers.PL & MemoryFlag)
#define SA1CheckOverflow()	(SA1._Overflow)
#define SA1CheckNegative()	(SA1._Negative & 0x80)
#define SA1CheckEmulation()	(SA1Registers.P.W & Emulation)

#define SA1SetFlags(f)		(SA1Registers.P.W |= (f))
#define SA1ClearFlags(f)	(SA1Registers.P.W &= ~(f))
#define SA1CheckFlag(f)		(SA1Registers.PL & (f))

extern struct SSA1Registers	SA1Registers;
extern struct SSA1			SA1;
extern uint8				SA1OpenBus;
extern struct SOpcodes		S9xSA1OpcodesM1X1[256];
extern struct SOpcodes		S9xSA1OpcodesM1X0[256];
extern struct SOpcodes		S9xSA1OpcodesM0X1[256];
extern struct SOpcodes		S9xSA1OpcodesM0X0[256];
extern uint8				S9xOpLengthsM1X1[256];
extern uint8				S9xOpLengthsM1X0[256];
extern uint8				S9xOpLengthsM0X1[256];
extern uint8				S9xOpLengthsM0X0[256];

uint8 S9xSA1GetByte (uint32);
void S9xSA1SetByte (uint8, uint32);
uint16 S9xSA1GetWord (uint32, enum s9xwrap_t w = WRAP_NONE);
void S9xSA1SetWord (uint16, uint32, enum s9xwrap_t w = WRAP_NONE, enum s9xwriteorder_t o = WRITE_01);
void S9xSA1SetPCBase (uint32);
uint8 S9xGetSA1 (uint32);
void S9xSetSA1 (uint8, uint32);
void S9xSA1Init (void);
void S9xSA1MainLoop (void);
void S9xSA1ExecuteDuringSleep (void);
void S9xSA1PostLoadState (void);

#define SNES_IRQ_SOURCE		(1 << 7)
#define TIMER_IRQ_SOURCE	(1 << 6)
#define DMA_IRQ_SOURCE		(1 << 5)

static inline void S9xSA1UnpackStatus (void)
{
	SA1._Zero = (SA1Registers.PL & Zero) == 0;
	SA1._Negative = (SA1Registers.PL & Negative);
	SA1._Carry = (SA1Registers.PL & Carry);
	SA1._Overflow = (SA1Registers.PL & Overflow) >> 6;
}

static inline void S9xSA1PackStatus (void)
{
	SA1Registers.PL &= ~(Zero | Negative | Carry | Overflow);
	SA1Registers.PL |= SA1._Carry | ((SA1._Zero == 0) << 1) | (SA1._Negative & 0x80) | (SA1._Overflow << 6);
}

static inline void S9xSA1FixCycles (void)
{
	if (SA1CheckEmulation())
	{
		SA1.S9xOpcodes = S9xSA1OpcodesM1X1;
		SA1.S9xOpLengths = S9xOpLengthsM1X1;
	}
	else
	if (SA1CheckMemory())
	{
		if (SA1CheckIndex())
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM1X1;
			SA1.S9xOpLengths = S9xOpLengthsM1X1;
		}
		else
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM1X0;
			SA1.S9xOpLengths = S9xOpLengthsM1X0;
		}
	}
	else
	{
		if (SA1CheckIndex())
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM0X1;
			SA1.S9xOpLengths = S9xOpLengthsM0X1;
		}
		else
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM0X0;
			SA1.S9xOpLengths = S9xOpLengthsM0X0;
		}
	}
}

#endif

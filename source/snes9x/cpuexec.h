/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/


#ifndef _CPUEXEC_H_
#define _CPUEXEC_H_

#include "ppu.h"

struct SOpcodes
{
	void (*S9xOpcode) (void);
};

struct SICPU
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
	uint32	Frame;
	uint32	FrameAdvanceCount;
};

extern struct SICPU		ICPU;

extern struct SOpcodes	S9xOpcodesE1[256];
extern struct SOpcodes	S9xOpcodesM1X1[256];
extern struct SOpcodes	S9xOpcodesM1X0[256];
extern struct SOpcodes	S9xOpcodesM0X1[256];
extern struct SOpcodes	S9xOpcodesM0X0[256];
extern struct SOpcodes	S9xOpcodesSlow[256];
extern uint8			S9xOpLengthsM1X1[256];
extern uint8			S9xOpLengthsM1X0[256];
extern uint8			S9xOpLengthsM0X1[256];
extern uint8			S9xOpLengthsM0X0[256];

void S9xMainLoop (void);
void S9xReset (void);
void S9xSoftReset (void);
void S9xDoHEventProcessing (void);
void S9xClearIRQ (uint32);
void S9xSetIRQ (uint32);

static inline void S9xUnpackStatus (void)
{
	ICPU._Zero = (Registers.PL & Zero) == 0;
	ICPU._Negative = (Registers.PL & Negative);
	ICPU._Carry = (Registers.PL & Carry);
	ICPU._Overflow = (Registers.PL & Overflow) >> 6;
}

static inline void S9xPackStatus (void)
{
	Registers.PL &= ~(Zero | Negative | Carry | Overflow);
	Registers.PL |= ICPU._Carry | ((ICPU._Zero == 0) << 1) | (ICPU._Negative & 0x80) | (ICPU._Overflow << 6);
}

static inline void S9xFixCycles (void)
{
	if (CheckEmulation())
	{
		ICPU.S9xOpcodes = S9xOpcodesE1;
		ICPU.S9xOpLengths = S9xOpLengthsM1X1;
	}
	else
	if (CheckMemory())
	{
		if (CheckIndex())
		{
			ICPU.S9xOpcodes = S9xOpcodesM1X1;
			ICPU.S9xOpLengths = S9xOpLengthsM1X1;
		}
		else
		{
			ICPU.S9xOpcodes = S9xOpcodesM1X0;
			ICPU.S9xOpLengths = S9xOpLengthsM1X0;
		}
	}
	else
	{
		if (CheckIndex())
		{
			ICPU.S9xOpcodes = S9xOpcodesM0X1;
			ICPU.S9xOpLengths = S9xOpLengthsM0X1;
		}
		else
		{
			ICPU.S9xOpcodes = S9xOpcodesM0X0;
			ICPU.S9xOpLengths = S9xOpLengthsM0X0;
		}
	}
}

static inline void S9xReschedule (void)
{
	uint8	next = 0;
	int32	hpos = 0;

	switch (CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
		case HC_IRQ_1_3_EVENT:
			next = HC_HDMA_START_EVENT;
			hpos = Timings.HDMAStart;
			break;

		case HC_HDMA_START_EVENT:
		case HC_IRQ_3_5_EVENT:
			next = HC_HCOUNTER_MAX_EVENT;
			hpos = Timings.H_Max;
			break;

		case HC_HCOUNTER_MAX_EVENT:
		case HC_IRQ_5_7_EVENT:
			next = HC_HDMA_INIT_EVENT;
			hpos = Timings.HDMAInit;
			break;

		case HC_HDMA_INIT_EVENT:
		case HC_IRQ_7_9_EVENT:
			next = HC_RENDER_EVENT;
			hpos = Timings.RenderPos;
			break;

		case HC_RENDER_EVENT:
		case HC_IRQ_9_A_EVENT:
			next = HC_WRAM_REFRESH_EVENT;
			hpos = Timings.WRAMRefreshPos;
			break;

		case HC_WRAM_REFRESH_EVENT:
		case HC_IRQ_A_1_EVENT:
			next = HC_HBLANK_START_EVENT;
			hpos = Timings.HBlankStart;
			break;
	}

	if (((int32) PPU.HTimerPosition > CPU.NextEvent) && ((int32) PPU.HTimerPosition < hpos))
	{
		hpos = (int32) PPU.HTimerPosition;

		switch (next)
		{
			case HC_HDMA_START_EVENT:
				next = HC_IRQ_1_3_EVENT;
				break;

			case HC_HCOUNTER_MAX_EVENT:
				next = HC_IRQ_3_5_EVENT;
				break;

			case HC_HDMA_INIT_EVENT:
				next = HC_IRQ_5_7_EVENT;
				break;

			case HC_RENDER_EVENT:
				next = HC_IRQ_7_9_EVENT;
				break;

			case HC_WRAM_REFRESH_EVENT:
				next = HC_IRQ_9_A_EVENT;
				break;

			case HC_HBLANK_START_EVENT:
				next = HC_IRQ_A_1_EVENT;
				break;
		}
	}

	CPU.NextEvent  = hpos;
	CPU.WhichEvent = next;
}

#endif

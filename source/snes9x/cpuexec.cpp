/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/


#include "snes9x.h"
#include "memmap.h"
#include "cpuops.h"
#include "dma.h"
#include "apu/apu.h"
#include "fxemu.h"
#include "snapshot.h"

static inline void S9xReschedule (void);

static int endframe = 0;

void S9xMainLoop (void)
{
	for (;;)
	{
		if (CPU.Flags)
		{
			if (CPU.Flags & NMI_FLAG)
			{
				if (Timings.NMITriggerPos <= CPU.Cycles)
				{
					CPU.Flags &= ~NMI_FLAG;
					Timings.NMITriggerPos = 0xffff;
					if (CPU.WaitingForInterrupt)
					{
						CPU.WaitingForInterrupt = FALSE;
						Registers.PCw++;
					}

					S9xOpcode_NMI();
				}
			}

			if (CPU.Flags & IRQ_FLAG)
			{
				if (CPU.IRQPending)
					// FIXME: In case of IRQ during WRAM refresh
					CPU.IRQPending--;
				else
				{
					if (CPU.WaitingForInterrupt)
					{
						CPU.WaitingForInterrupt = FALSE;
						Registers.PCw++;
					}

					if (CPU.IRQActive && !Settings.DisableIRQ)
					{
						if (!CheckFlag(IRQ))
						// in IRQ handler $4211 is supposed to be read, so IRQ_FLAG should be cleared.
							S9xOpcode_IRQ();
					}
					else
						CPU.Flags &= ~IRQ_FLAG;
				}
			}

			if (CPU.Flags & SCAN_KEYS_FLAG)
				break;

		}

	#ifdef CPU_SHUTDOWN
		CPU.PBPCAtOpcodeStart = Registers.PBPC;
	#endif

		register uint8				Op;
		register struct	SOpcodes	*Opcodes;

		CPU.PrevCycles = CPU.Cycles;

		if (CPU.PCBase)
		{
			Op = CPU.PCBase[Registers.PCw];
			CPU.Cycles += CPU.MemSpeed;
			Opcodes = ICPU.S9xOpcodes;
		}
		else
		{
			Op = S9xGetByte(Registers.PBPC);
			OpenBus = Op;
			Opcodes = S9xOpcodesSlow;
		}

		if ((Registers.PCw & MEMMAP_MASK) + ICPU.S9xOpLengths[Op] >= MEMMAP_BLOCK_SIZE)
		{
			uint8	*oldPCBase = CPU.PCBase;

			CPU.PCBase = S9xGetBasePointer(ICPU.ShiftedPB + ((uint16) (Registers.PCw + 4)));
			if (oldPCBase != CPU.PCBase || (Registers.PCw & ~MEMMAP_MASK) == (0xffff & ~MEMMAP_MASK))
				Opcodes = S9xOpcodesSlow;
		}

		Registers.PCw++;
		(*Opcodes[Op].S9xOpcode)();

		if (SA1.Executing)
			S9xSA1MainLoop();

	#if (S9X_ACCURACY_LEVEL <= 2)
		while (CPU.Cycles >= CPU.NextEvent)
			S9xDoHEventProcessing();
	#endif
	}

	S9xPackStatus();

	CPU.Flags &= ~SCAN_KEYS_FLAG;

	if (endframe)
	{
		S9xSyncSpeed();
		endframe = 0;
	}
}

void S9xSetIRQ (uint32 source)
{
	CPU.IRQActive |= source;
	CPU.IRQPending = Timings.IRQPendCount;
	CPU.Flags |= IRQ_FLAG;

	if (CPU.WaitingForInterrupt)
	{
		// Force IRQ to trigger immediately after WAI -
		// Final Fantasy Mystic Quest crashes without this.
		CPU.WaitingForInterrupt = FALSE;
		Registers.PCw++;
	}
}

void S9xClearIRQ (uint32 source)
{
	CPU.IRQActive &= ~source;
	if (!CPU.IRQActive)
		CPU.Flags &= ~IRQ_FLAG;

}

void S9xDoHEventProcessing (void)
{
#ifdef CPU_SHUTDOWN
	CPU.WaitCounter++;
#endif

	switch (CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
			S9xCheckMissingHTimerPosition(Timings.HBlankStart);
			S9xReschedule();
			break;

		case HC_HDMA_START_EVENT:
			S9xCheckMissingHTimerPosition(Timings.HDMAStart);
			S9xReschedule();

			if (PPU.HDMA && CPU.V_Counter <= PPU.ScreenHeight)
			{
				PPU.HDMA = S9xDoHDMA(PPU.HDMA);
			}

			break;

		case HC_HCOUNTER_MAX_EVENT:
			if (Settings.SuperFX)
			{
				if (!SuperFX.oneLineDone)
					S9xSuperFXExec();
				SuperFX.oneLineDone = FALSE;
			}

			S9xAPUEndScanline();
			CPU.Cycles -= Timings.H_Max;
			S9xAPUSetReferenceTime(CPU.Cycles);

			if ((Timings.NMITriggerPos != 0xffff) && (Timings.NMITriggerPos >= Timings.H_Max))
				Timings.NMITriggerPos -= Timings.H_Max;

			CPU.V_Counter++;
			if (CPU.V_Counter >= Timings.V_Max)	// V ranges from 0 to Timings.V_Max - 1
			{
				CPU.V_Counter = 0;
				Timings.InterlaceField ^= 1;

				// From byuu:
				// [NTSC]
				// interlace mode has 525 scanlines: 263 on the even frame, and 262 on the odd.
				// non-interlace mode has 524 scanlines: 262 scanlines on both even and odd frames.
				// [PAL] <PAL info is unverified on hardware>
				// interlace mode has 625 scanlines: 313 on the even frame, and 312 on the odd.
				// non-interlace mode has 624 scanlines: 312 scanlines on both even and odd frames.
				if (IPPU.Interlace && !Timings.InterlaceField)
					Timings.V_Max = Timings.V_Max_Master + 1;	// 263 (NTSC), 313?(PAL)
				else
					Timings.V_Max = Timings.V_Max_Master;		// 262 (NTSC), 312?(PAL)

				Memory.FillRAM[0x213F] ^= 0x80;
				PPU.RangeTimeOver = 0;

				// FIXME: reading $4210 will wait 2 cycles, then perform reading, then wait 4 more cycles.
				Memory.FillRAM[0x4210] = Model->_5A22;
				CPU.Flags &= ~NMI_FLAG;
				Timings.NMITriggerPos = 0xffff;

				ICPU.Frame++;
				endframe = 1;
				PPU.HVBeamCounterLatched = 0;
				CPU.Flags |= SCAN_KEYS_FLAG;
			}

			// From byuu:
			// In non-interlace mode, there are 341 dots per scanline, and 262 scanlines per frame.
			// On odd frames, scanline 240 is one dot short.
			// In interlace mode, there are always 341 dots per scanline. Even frames have 263 scanlines,
			// and odd frames have 262 scanlines.
			// Interlace mode scanline 240 on odd frames is not missing a dot.
			if (CPU.V_Counter == 240 && !IPPU.Interlace && Timings.InterlaceField)	// V=240
				Timings.H_Max = Timings.H_Max_Master - ONE_DOT_CYCLE;	// HC=1360
			else
				Timings.H_Max = Timings.H_Max_Master;					// HC=1364

			if (Model->_5A22 == 2)
			{
				if (CPU.V_Counter != 240 || IPPU.Interlace || !Timings.InterlaceField)	// V=240
				{
					if (Timings.WRAMRefreshPos == SNES_WRAM_REFRESH_HC_v2 - ONE_DOT_CYCLE)	// HC=534
						Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2;					// HC=538
					else
						Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2 - ONE_DOT_CYCLE;	// HC=534
				}
			}
			else
				Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v1;

			S9xCheckMissingHTimerPosition(0);

			if (CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE)	// VBlank starts from V=225(240).
			{
				S9xEndScreenRefresh();
				
				CPU.Flags |= SCAN_KEYS_FLAG;
				
				PPU.HDMA = 0;
				// Bits 7 and 6 of $4212 are computed when read in S9xGetPPU.
				IPPU.MaxBrightness = PPU.Brightness;
				PPU.ForcedBlanking = (Memory.FillRAM[0x2100] >> 7) & 1;

				if (!PPU.ForcedBlanking)
				{
					PPU.OAMAddr = PPU.SavedOAMAddr;

					uint8	tmp = 0;

					if (PPU.OAMPriorityRotation)
						tmp = (PPU.OAMAddr & 0xFE) >> 1;
					if ((PPU.OAMFlip & 1) || PPU.FirstSprite != tmp)
					{
						PPU.FirstSprite = tmp;
						IPPU.OBJChanged = TRUE;
					}

					PPU.OAMFlip = 0;
				}

				// FIXME: writing to $4210 will wait 6 cycles.
				Memory.FillRAM[0x4210] = 0x80 | Model->_5A22;
				if (Memory.FillRAM[0x4200] & 0x80)
				{
					// FIXME: triggered at HC=6, checked just before the final CPU cycle,
					// then, when to call S9xOpcode_NMI()?
					CPU.Flags |= NMI_FLAG;
					Timings.NMITriggerPos = 6 + 6;
				}

			}

			if (CPU.V_Counter == PPU.ScreenHeight + 3)	// FIXME: not true
			{
				if (Memory.FillRAM[0x4200] & 1)
					S9xDoAutoJoypad();
			}

			if (CPU.V_Counter == FIRST_VISIBLE_LINE)	// V=1
				S9xStartScreenRefresh();

			CPU.NextEvent = -1;
			S9xReschedule();

			break;

		case HC_HDMA_INIT_EVENT:
			S9xCheckMissingHTimerPosition(Timings.HDMAInit);
			S9xReschedule();

			if (CPU.V_Counter == 0)
			{
				S9xStartHDMA();
			}

			break;

		case HC_RENDER_EVENT:
			if (CPU.V_Counter >= FIRST_VISIBLE_LINE && CPU.V_Counter <= PPU.ScreenHeight)
				RenderLine((uint8) (CPU.V_Counter - FIRST_VISIBLE_LINE));

			S9xCheckMissingHTimerPosition(Timings.RenderPos);
			S9xReschedule();

			break;

		case HC_WRAM_REFRESH_EVENT:

			S9xCheckMissingHTimerHalt(Timings.WRAMRefreshPos, SNES_WRAM_REFRESH_CYCLES);
			CPU.Cycles += SNES_WRAM_REFRESH_CYCLES;

			S9xCheckMissingHTimerPosition(Timings.WRAMRefreshPos);
			S9xReschedule();

			break;

		case HC_IRQ_1_3_EVENT:
		case HC_IRQ_3_5_EVENT:
		case HC_IRQ_5_7_EVENT:
		case HC_IRQ_7_9_EVENT:
		case HC_IRQ_9_A_EVENT:
		case HC_IRQ_A_1_EVENT:
			if (PPU.HTimerEnabled && (!PPU.VTimerEnabled || (CPU.V_Counter == PPU.VTimerPosition)))
				S9xSetIRQ(PPU_IRQ_SOURCE);
			else
			if (PPU.VTimerEnabled && (CPU.V_Counter == PPU.VTimerPosition))
				S9xSetIRQ(PPU_IRQ_SOURCE);

			S9xReschedule();
			break;
	}
}

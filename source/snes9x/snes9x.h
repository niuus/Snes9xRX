/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/


#ifndef _SNES9X_H_
#define _SNES9X_H_

#ifndef VERSION
#define VERSION	"1.53"
#endif

#include "port.h"
#include "65c816.h"
#include "messages.h"

#define S9X_ACCURACY_LEVEL		3

#ifdef ZLIB
#include <zlib.h>
#define STREAM					gzFile
#define READ_STREAM(p, l, s)	gzread(s, p, l)
#define WRITE_STREAM(p, l, s)	gzwrite(s, p, l)
#define GETS_STREAM(p, l, s)	gzgets(s, p, l)
#define GETC_STREAM(s)			gzgetc(s)
#define OPEN_STREAM(f, m)		gzopen(f, m)
#define REOPEN_STREAM(f, m)		gzdopen(f, m)
#define FIND_STREAM(f)			gztell(f)
#define REVERT_STREAM(f, o, s)	gzseek(f, o, s)
#define CLOSE_STREAM(s)			gzclose(s)
#else
#define STREAM					FILE *
#define READ_STREAM(p, l, s)	fread(p, 1, l, s)
#define WRITE_STREAM(p, l, s)	fwrite(p, 1, l, s)
#define GETS_STREAM(p, l, s)	fgets(p, l, s)
#define GETC_STREAM(s)			fgetc(s)
#define OPEN_STREAM(f, m)		fopen(f, m)
#define REOPEN_STREAM(f, m)		fdopen(f, m)
#define FIND_STREAM(f)			ftell(f)
#define REVERT_STREAM(f, o, s)	fseek(f, o, s)
#define CLOSE_STREAM(s)			fclose(s)
#endif

#define SNES_WIDTH					256
#define SNES_HEIGHT					224
#define SNES_HEIGHT_EXTENDED		239
#define MAX_SNES_WIDTH				(SNES_WIDTH * 2)
#define MAX_SNES_HEIGHT				(SNES_HEIGHT_EXTENDED * 2)
#define IMAGE_WIDTH					(Settings.SupportHiRes ? MAX_SNES_WIDTH : SNES_WIDTH)
#define IMAGE_HEIGHT				(Settings.SupportHiRes ? MAX_SNES_HEIGHT : SNES_HEIGHT_EXTENDED)

#define	NTSC_MASTER_CLOCK			21477272.727272 // 21477272 + 8/11 exact
#define	PAL_MASTER_CLOCK			21281370.0

#define SNES_MAX_NTSC_VCOUNTER		262
#define SNES_MAX_PAL_VCOUNTER		312
#define SNES_HCOUNTER_MAX			341

#define ONE_CYCLE					6
#define SLOW_ONE_CYCLE				8
#define TWO_CYCLES					12
#define	ONE_DOT_CYCLE				4

#define SNES_CYCLES_PER_SCANLINE	(SNES_HCOUNTER_MAX * ONE_DOT_CYCLE)
#define SNES_SCANLINE_TIME			(SNES_CYCLES_PER_SCANLINE / NTSC_MASTER_CLOCK)

#define SNES_WRAM_REFRESH_HC_v1		530
#define SNES_WRAM_REFRESH_HC_v2		538
#define SNES_WRAM_REFRESH_CYCLES	40

#define SNES_HBLANK_START_HC		1096					// H=274
#define	SNES_HDMA_START_HC			1106					// FIXME: not true
#define	SNES_HBLANK_END_HC			4						// H=1
#define	SNES_HDMA_INIT_HC			20						// FIXME: not true
#define	SNES_RENDER_START_HC		(48 * ONE_DOT_CYCLE)	// FIXME: Snes9x renders a line at a time.

#define SNES_TR_MASK		(1 <<  4)
#define SNES_TL_MASK		(1 <<  5)
#define SNES_X_MASK			(1 <<  6)
#define SNES_A_MASK			(1 <<  7)
#define SNES_RIGHT_MASK		(1 <<  8)
#define SNES_LEFT_MASK		(1 <<  9)
#define SNES_DOWN_MASK		(1 << 10)
#define SNES_UP_MASK		(1 << 11)
#define SNES_START_MASK		(1 << 12)
#define SNES_SELECT_MASK	(1 << 13)
#define SNES_Y_MASK			(1 << 14)
#define SNES_B_MASK			(1 << 15)

#define DEBUG_MODE_FLAG		(1 <<  0)	// debugger
#define TRACE_FLAG			(1 <<  1)	// debugger
#define SINGLE_STEP_FLAG	(1 <<  2)	// debugger
#define BREAK_FLAG			(1 <<  3)	// debugger
#define NMI_FLAG			(1 <<  7)	// CPU
#define IRQ_FLAG			(1 << 11)	// CPU
#define SCAN_KEYS_FLAG		(1 <<  4)	// CPU
#define HALTED_FLAG			(1 << 12)	// APU
#define FRAME_ADVANCE_FLAG	(1 <<  9)

#define ROM_NAME_LEN	23
#define AUTO_FRAMERATE	200

struct SCPUState
{
	int32	Cycles;
	int32	PrevCycles;
	int32	V_Counter;
	uint32	Flags;
	uint8	*PCBase;
	bool8	IRQActive;
	int32	IRQPending;
	int32	MemSpeed;
	int32	MemSpeedx2;
	int32	FastROMSpeed;
	bool8	InDMA;
	bool8	InHDMA;
	bool8	InDMAorHDMA;
	bool8	InWRAMDMAorHDMA;
	uint8	HDMARanInDMA;
	int32	CurrentDMAorHDMAChannel;
	uint8	WhichEvent;
	int32	NextEvent;
	bool8	WaitingForInterrupt;
	uint32	WaitAddress;
	uint32	WaitCounter;
	uint32	PBPCAtOpcodeStart;
	uint32	AutoSaveTimer;
	bool8	SRAMModified;
};

enum
{
	HC_HBLANK_START_EVENT = 1,
	HC_IRQ_1_3_EVENT      = 2,
	HC_HDMA_START_EVENT   = 3,
	HC_IRQ_3_5_EVENT      = 4,
	HC_HCOUNTER_MAX_EVENT = 5,
	HC_IRQ_5_7_EVENT      = 6,
	HC_HDMA_INIT_EVENT    = 7,
	HC_IRQ_7_9_EVENT      = 8,
	HC_RENDER_EVENT       = 9,
	HC_IRQ_9_A_EVENT      = 10,
	HC_WRAM_REFRESH_EVENT = 11,
	HC_IRQ_A_1_EVENT      = 12
};

struct STimings
{
	int32	H_Max_Master;
	int32	H_Max;
	int32	V_Max_Master;
	int32	V_Max;
	int32	HBlankStart;
	int32	HBlankEnd;
	int32	HDMAInit;
	int32	HDMAStart;
	int32	NMITriggerPos;
	int32	WRAMRefreshPos;
	int32	RenderPos;
	bool8	InterlaceField;
	int32	DMACPUSync;		// The cycles to synchronize DMA and CPU. Snes9x cannot emulate correctly.
	int32	NMIDMADelay;	// The delay of NMI trigger after DMA transfers. Snes9x cannot emulate correctly.
	int32	IRQPendCount;	// This value is just a hack, because Snes9x cannot emulate any events in an opcode.
	int32	APUSpeedup;
	bool8	APUAllowTimeOverflow;
#ifdef HW_RVL
	int32	SA1Cycles;
#endif
};

struct SSettings
{
	bool8	TraceDMA;
	bool8	TraceHDMA;
	bool8	TraceVRAM;
	bool8	TraceUnknownRegisters;
	bool8	TraceDSP;
	bool8	TraceHCEvent;

	bool8	SuperFX;
	uint8	DSP;
	bool8	SA1;
	bool8	C4;
	bool8	SDD1;
	bool8	SPC7110;
	bool8	SPC7110RTC;
	bool8	OBC1;
	uint8	SETA;
	bool8	SRTC;
	bool8	BS;
	bool8	BSXItself;
	bool8	BSXBootup;
	bool8	MSU1;
	bool8	MouseMaster;
	bool8	SuperScopeMaster;
	bool8	JustifierMaster;
	bool8	MultiPlayer5Master;

	bool8	ForceLoROM;
	bool8	ForceHiROM;
	bool8	ForceHeader;
	bool8	ForceNoHeader;
	bool8	ForceInterleaved;
	bool8	ForceInterleaved2;
	bool8	ForceInterleaveGD24;
	bool8	ForceNotInterleaved;
	bool8	ForcePAL;
	bool8	ForceNTSC;
	bool8	PAL;
	uint32	FrameTimePAL;
	uint32	FrameTimeNTSC;
	uint32	FrameTime;

	bool8	SoundSync;
	bool8	SixteenBitSound;
	uint32	SoundPlaybackRate;
	uint32	SoundInputRate;
	bool8	Stereo;
	bool8	ReverseStereo;
	bool8	Mute;
	bool8	DynamicRateControl;
	int32	InterpolationMethod;
	int32	DynamicRateLimit; /* Multiplied by 1000 */

	bool8	SupportHiRes;
	bool8	Transparency;
	uint8	BG_Forced;
	bool8	DisableGraphicWindows;

	bool8	DisplayTime;
	bool8	DisplayFrameRate;
	bool8	DisplayWatchedAddresses;
	bool8	DisplayPressedKeys;
	bool8	DisplayMovieFrame;
	bool8	AutoDisplayMessages;
	uint32	InitialInfoStringTimeout;
	uint16	DisplayColor;

	bool8	Multi;
	char	CartAName[PATH_MAX + 1];
	char	CartBName[PATH_MAX + 1];

	bool8	DisableGameSpecificHacks;
	bool8	ShutdownMaster;
	bool8	Shutdown;
	bool8	BlockInvalidVRAMAccessMaster;
	bool8	BlockInvalidVRAMAccess;
	bool8	DisableIRQ;
	bool8	DisableHDMA;
	int32	HDMATimingHack;

	bool8	ForcedPause;
	bool8	Paused;
	bool8	StopEmulation;

	uint32	SkipFrames;
	uint32	TurboSkipFrames;
	uint32	AutoMaxSkipFrames;
	bool8	TurboMode;
	uint32	HighSpeedSeek;
	bool8	FrameAdvance;

	bool8	NetPlay;
	bool8	NetPlayServer;
	char	ServerName[128];
	int		Port;

	bool8	MovieTruncate;
	bool8	MovieNotifyIgnored;
	bool8	WrongMovieStateProtection;
	bool8	DumpStreams;
	int		DumpStreamsMaxFrames;

	bool8	TakeScreenshot;
	int8	StretchScreenshots;
	bool8	SnapshotScreenshots;

	bool8	ApplyCheats;
	bool8	NoPatch;
	int32	AutoSaveDelay;
	bool8	DontSaveOopsSnapshot;
	bool8	UpAndDown;

	float	SuperFXSpeedPerLine;
};

struct SSNESGameFixes
{
	uint8	SRAMInitialValue;
	uint8	Uniracers;
};

void S9xExit(void);
void S9xMessage(int, int, const char *);

extern struct SSettings			Settings;
extern struct SCPUState			CPU;
extern struct STimings			Timings;
extern struct SSNESGameFixes	SNESGameFixes;
extern char						String[513];

#endif

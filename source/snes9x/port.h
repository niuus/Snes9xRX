/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _PORT_H_
#define _PORT_H_

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#ifndef GEKKO
#include <memory.h>
#endif
#include <time.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <sys/types.h>

#ifdef __WIN32__
#define NOMINMAX
#include <windows.h>
#endif

#define GFX_MULTI_FORMAT

#ifdef __WIN32__
//#define RIGHTSHIFT_IS_SAR
#define RIGHTSHIFT_int8_IS_SAR
#define RIGHTSHIFT_int16_IS_SAR
#define RIGHTSHIFT_int32_IS_SAR
#define SNES_JOY_READ_CALLBACKS
#endif

#ifdef __MACOSX__
#undef GFX_MULTI_FORMAT
#define PIXEL_FORMAT RGB555
#endif

#ifndef snes9x_types_defined
#define snes9x_types_defined
typedef unsigned char		bool8;
#ifdef HAVE_STDINT_H
#include <stdint.h>
typedef intptr_t			pint;
typedef int8_t				int8;
typedef uint8_t				uint8;
typedef int16_t				int16;
typedef uint16_t			uint16;
typedef int32_t				int32;
typedef uint32_t			uint32;
typedef int64_t				int64;
typedef uint64_t			uint64;
#else	// HAVE_STDINT_H
#ifdef __WIN32__
typedef intptr_t			pint;
#else	// __WIN32__
#ifdef PTR_NOT_INT
typedef long				pint;
#else
typedef int					pint;
#endif
#endif	// __WIN32__
#ifdef __WIN32__
#ifdef __BORLANDC__
#include <systypes.h>
#else
typedef signed char			int8;
typedef unsigned char		uint8;
typedef signed short		int16;
typedef unsigned short		uint16;
#ifndef WSAAP
// winsock2.h typedefs int32 as well
typedef signed int			int32;
#endif
typedef unsigned int		uint32;
#endif
typedef unsigned char		uint8_t;
typedef signed __int64		int64;
typedef unsigned __int64	uint64;
typedef int					socklen_t;
#else	// __WIN32__
typedef signed char			int8;
typedef unsigned char		uint8;
typedef signed short		int16;
typedef unsigned short		uint16;
typedef signed int			int32;
typedef unsigned int		uint32;
#ifdef __GNUC__
// long long is not part of ISO C++ 
__extension__
#endif
typedef long long			int64;
typedef unsigned long long	uint64;
#endif	//  __WIN32__
#endif	// HAVE_STDINT_H
#endif	// snes9x_types_defined

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#define START_EXTERN_C	extern "C" {
#define END_EXTERN_C	}

#ifndef __WIN32__
#ifndef PATH_MAX
#define PATH_MAX	1024
#endif
#define _MAX_DRIVE	1
#define _MAX_DIR	PATH_MAX
#define _MAX_FNAME	PATH_MAX
#define _MAX_EXT	PATH_MAX
#define _MAX_PATH	PATH_MAX
#else
#ifndef PATH_MAX
#define PATH_MAX	_MAX_PATH
#endif
#endif

#ifndef __WIN32__
#define ZeroMemory(a, b)	memset((a), 0, (b))
void _splitpath (const char *, char *, char *, char *, char *);
void _makepath (char *, const char *, const char *, const char *, const char *);
#define S9xDisplayString	DisplayStringFromBottom
#else
#define snprintf _snprintf
#define strcasecmp	stricmp
#define strncasecmp	strnicmp
void WinDisplayStringFromBottom(const char *string, int linesFromBottom, int pixelsFromLeft, bool allowWrap);
#define S9xDisplayString	WinDisplayStringFromBottom
void SetInfoDlgColor(unsigned char, unsigned char, unsigned char);
#define SET_UI_COLOR(r,g,b) SetInfoDlgColor(r,g,b)
#endif

#ifdef __DJGPP
#define SLASH_STR	"\\"
#define SLASH_CHAR	'\\'
#else
#define SLASH_STR	"/"
#define SLASH_CHAR	'/'
#endif

#ifndef SIG_PF
#define SIG_PF	void (*) (int)
#endif

#ifdef __linux
#define TITLE "Snes9x: Linux"
#define SYS_CONFIG_FILE "/etc/snes9x/snes9x.conf"
#endif

#ifndef TITLE
#define TITLE "Snes9x"
#endif

#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__x86_64__) || defined(__alpha__) || defined(__MIPSEL__) || defined(_M_IX86) || defined(_M_X64)
#define LSB_FIRST
#define FAST_LSB_WORD_ACCESS
#else
#define MSB_FIRST
#endif

#ifdef FAST_LSB_WORD_ACCESS
#define READ_WORD(s)		(*(uint16 *) (s))
#define READ_3WORD(s)		(*(uint32 *) (s) & 0x00ffffff)
#define READ_DWORD(s)		(*(uint32 *) (s))
#define WRITE_WORD(s, d)	*(uint16 *) (s) = (d)
#define WRITE_3WORD(s, d)	*(uint16 *) (s) = (uint16) (d), *((uint8 *) (s) + 2) = (uint8) ((d) >> 16)
#define WRITE_DWORD(s, d)	*(uint32 *) (s) = (d)
#else
#define READ_WORD(s)		(*(uint8 *) (s) | (*((uint8 *) (s) + 1) << 8))
#define READ_3WORD(s)		(*(uint8 *) (s) | (*((uint8 *) (s) + 1) << 8) | (*((uint8 *) (s) + 2) << 16))
#define READ_DWORD(s)		(*(uint8 *) (s) | (*((uint8 *) (s) + 1) << 8) | (*((uint8 *) (s) + 2) << 16) | (*((uint8 *) (s) + 3) << 24))
#define WRITE_WORD(s, d)	*(uint8 *) (s) = (uint8) (d), *((uint8 *) (s) + 1) = (uint8) ((d) >> 8)
#define WRITE_3WORD(s, d)	*(uint8 *) (s) = (uint8) (d), *((uint8 *) (s) + 1) = (uint8) ((d) >> 8), *((uint8 *) (s) + 2) = (uint8) ((d) >> 16)
#define WRITE_DWORD(s, d)	*(uint8 *) (s) = (uint8) (d), *((uint8 *) (s) + 1) = (uint8) ((d) >> 8), *((uint8 *) (s) + 2) = (uint8) ((d) >> 16), *((uint8 *) (s) + 3) = (uint8) ((d) >> 24)
#endif

#define SWAP_WORD(s)		(s) = (((s) & 0xff) <<  8) | (((s) & 0xff00) >> 8)
#define SWAP_DWORD(s)		(s) = (((s) & 0xff) << 24) | (((s) & 0xff00) << 8) | (((s) & 0xff0000) >> 8) | (((s) & 0xff000000) >> 24)

#include "pixform.h"

#endif

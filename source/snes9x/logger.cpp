/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifdef GEKKO
void S9xResetLogger (void) { }
void S9xCloseLogger (void) { }
#else

#include "snes9x.h"
#include "logger.h"

static int	resetno = 0;
static int	framecounter = 0;


void S9xResetLogger (void)
{
	if (!Settings.DumpStreams)
		return;

	char	buffer[128];

	S9xCloseLogger();
	framecounter = 0;

	sprintf(buffer, "videostream%d.dat", resetno);
	video = fopen(buffer, "wb");
	if (!video)
	{
		printf("Opening %s failed. Logging cancelled.\n", buffer);
		return;
	}

	sprintf(buffer, "audiostream%d.dat", resetno);
	audio = fopen(buffer, "wb");
	if (!audio)
	{
		printf("Opening %s failed. Logging cancelled.\n", buffer);
		fclose(video);
		return;
	}

	resetno++;
}

void S9xCloseLogger (void)
{
	if (video)
	{
		fclose(video);
		video = NULL;
	}

	if (audio)
	{
		fclose(audio);
		audio = NULL;
	}
}	

#endif

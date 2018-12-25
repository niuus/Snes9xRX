/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/


// Abstract the details of reading from zip files versus FILE *'s.

#include <string>
#include "snes9x.h"
#include "reader.h"


// Generic constructor/destructor

Reader::Reader (void)
{
	return;
}

Reader::~Reader (void)
{
	return;
}

// Generic getline function, based on gets. Reimlpement if you can do better.

char * Reader::getline (void)
{
	bool		eof;
	std::string	ret;

	ret = getline(eof);
	if (ret.size() == 0 && eof)
		return (NULL);

	return (strdup(ret.c_str()));
}

std::string Reader::getline (bool &eof)
{
	char		buf[1024];
	std::string	ret;

	eof = false;
	ret.clear();

	do
	{
		if (gets(buf, sizeof(buf)) == NULL)
		{
			eof = true;
			break;
		}

		ret.append(buf);
	}
	while (*ret.rbegin() != '\n');

	return (ret);
}

// snes9x.h STREAM reader

fReader::fReader (STREAM f)
{
	fp = f;
}

fReader::~fReader (void)
{
	return;
}

int fReader::get_char (void)
{
	return (GETC_STREAM(fp));
}

char * fReader::gets (char *buf, size_t len)
{
	return (GETS_STREAM(buf, len, fp));
}

size_t fReader::read (char *buf, size_t len)
{
	return (READ_STREAM(buf, len, fp));
}

// unzip reader

#ifdef UNZIP_SUPPORT

unzReader::unzReader (unzFile &v)
{
	file = v;
	head = NULL;
	numbytes = 0;
}

unzReader::~unzReader (void)
{
	return;
}

int unzReader::get_char (void)
{
	unsigned char	c;

	if (numbytes <= 0)
	{
		numbytes = unzReadCurrentFile(file, buffer, unz_BUFFSIZ);
		if (numbytes <= 0)
			return (EOF);
		head = buffer;
	}

	c = *head;
	head++;
	numbytes--;

	return ((int) c);
}

char * unzReader::gets (char *buf, size_t len)
{
	size_t	i;
	int		c;

	for (i = 0; i < len - 1; i++)
	{
		c = get_char();
		if (c == EOF)
		{
			if (i == 0)
				return (NULL);
			break;
		}

		buf[i] = (char) c;
		if (buf[i] == '\n')
			break;
	}

	buf[i] = '\0';

	return (buf);
}

size_t unzReader::read (char *buf, size_t len)
{
	if (len == 0)
		return (len);

	if (len <= numbytes)
	{
		memcpy(buf, head, len);
		numbytes -= len;
		head += len;
		return (len);
	}

	size_t	numread = 0;
	if (numbytes > 0)
	{
		memcpy(buf, head, numbytes);
		numread += numbytes;
		head = NULL;
		numbytes = 0;
	}

	int	l = unzReadCurrentFile(file, buf + numread, len - numread);
	if (l > 0)
		numread += l;

	return (numread);
}

#endif

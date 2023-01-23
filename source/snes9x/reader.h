/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _READER_H_
#define _READER_H_

class Reader
{
	public:
		Reader (void);
		virtual ~Reader (void);
		virtual int get_char (void) = 0;
		virtual char * gets (char *, size_t) = 0;
		virtual char * getline (void);	// free() when done
		virtual std::string getline (bool &);
		virtual size_t read (char *, size_t) = 0;
};

class fReader : public Reader
{
	public:
		fReader (STREAM);
		virtual ~fReader (void);
		virtual int get_char (void);
		virtual char * gets (char *, size_t);
		virtual size_t read (char *, size_t);

	private:
		STREAM	fp;
};

#ifdef UNZIP_SUPPORT

#define unz_BUFFSIZ	1024

class unzReader : public Reader
{
	public:
		unzReader (unzFile &);
		virtual ~unzReader (void);
		virtual int get_char (void);
		virtual char * gets (char *, size_t);
		virtual size_t read (char *, size_t);

	private:
		unzFile	file;
		char	buffer[unz_BUFFSIZ];
		char	*head;
		size_t	numbytes;
};

#endif

#endif

/***************************************************************************
 *   Copyright (C) 2005 to 2007 by Jonathan Duddington                     *
 *   email: jonsd@users.sourceforge.net                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write see:                           *
 *               <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/

#ifdef ENV_WINDOWS


#define PLATFORM_WINDOWS
#define __WIN32__
#define NEED_WCSTOF
#define NEED_GETOPT

#define PATHSEP '\\'
#define USE_PORTAUDIO
#define ESPEAK_API
//#define INCLUDE_KLATT

// will look for espeak_data directory here, and also in user's home directory
#define PATH_ESPEAK_DATA  "/usr/share/espeak-data"

typedef unsigned short USHORT;
typedef unsigned char  UCHAR;
typedef double DOUBLEX;




typedef struct {
   const char *mnem;
   int  value;
} MNEM_TAB;
int LookupMnem(MNEM_TAB *table, char *string);



#define N_PATH_HOME  220
extern char path_home[N_PATH_HOME];    // this is the espeak-data directory

extern void strncpy0(char *to,const char *from, int size);
int  GetFileLength(const char *filename);
char *Alloc(int size);
void Free(void *ptr);


#else

#include <sys/types.h>

// conditional compilation options
#define INCLUDE_KLATT

#if defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN
#define ARCH_BIG
#endif

#define PLATFORM_POSIX
#define PATHSEP  '/'
// USE_PORTAUDIO or USE_PULSEAUDIO are now defined in the makefile
//#define USE_PORTAUDIO
//#define USE_PULSEAUDIO
#define USE_NANOSLEEP
#define __cdecl 
#define ESPEAK_API  extern "C"

#ifdef LIBRARY
#define USE_ASYNC
//#define USE_MBROLA_LIB
#endif

#ifdef _ESPEAKEDIT
#define USE_PORTAUDIO
#define USE_ASYNC
#define LOG_FRAMES      // write keyframe info to log-espeakedit
#endif

// will look for espeak_data directory here, and also in user's home directory
#ifndef PATH_ESPEAK_DATA
   #define PATH_ESPEAK_DATA  "/usr/share/espeak-data"
#endif

typedef unsigned short USHORT;
typedef unsigned char  UCHAR;
typedef double DOUBLEX;




typedef struct {
   const char *mnem;
   int  value;
} MNEM_TAB;
int LookupMnem(MNEM_TAB *table, char *string);


#ifdef PLATFORM_WINDOWS
#define N_PATH_HOME  220
#else
#define N_PATH_HOME  150
#endif

extern char path_home[N_PATH_HOME];    // this is the espeak-data directory

extern void strncpy0(char *to,const char *from, int size);
int  GetFileLength(const char *filename);
char *Alloc(int size);
void Free(void *ptr);
#endif


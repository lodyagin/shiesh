/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Cohors Concurro library.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU Lesser General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License
  for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <string>
#include <stdarg.h>
#include <atlbase.h>

#include <sstream>
#include <ostream>
#include <iomanip>

//#define WIN32_LEAN_AND_MEAN 
//#include <windows.h>


using std::string;
using std::wstring;
using std::istream;
using std::ostream;
using std::iostream;

typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned char uchar;


// cut out not more than maxCount chars that equal to passed one. If maxCount == -1 then cut out all
string trimLeft ( const string &, char = ' ', int maxCount = -1 );
string trimRight( const string &, char = ' ', int maxCount = -1 );
string trimBoth ( const string &, char = ' ', int maxCount = -1 );

const char * strnchr( const char * str, int chr, size_t maxLen );
//int strnlen( const char * str, size_t maxLen );  // returns -1 if len > maxLen

 /* Copy string src to buffer dest (of buffer size dest_size).  At most
 * dest_size-1 characters will be copied.  Always NUL terminates
 * (unless dest_size == 0).  This function does NOT allocate memory.
 * Unlike strncpy, this function doesn't pad dest (so it's often faster).
 * Returns size of attempted result, strlen(src),
 * so if retval >= dest_size, truncation occurred.
 */
size_t strlcpy (char       *dest,
                const char *src,
                size_t      dest_size);

/* It is got from glib 2.0.
 *
 * Appends string src to buffer dest (of buffer size dest_size).
 * At most dest_size-1 characters will be copied.
 * Unlike strncat, dest_size is the full size of dest, not the space left over.
 * This function does NOT allocate memory.
 * This always NUL terminates (unless siz == 0 or there were no NUL characters
 * in the dest_size characters of dest to start with).
 * Returns size of attempted result, which is
 * MIN (dest_size, strlen (original dest)) + strlen (src),
 * so if retval >= dest_size, truncation occurred.
 */
size_t
strlcat (char       *dest,
         const char *src,
         size_t      dest_size);

/**
 * It is got from glib 2.0.
 *
 * @string: the return location for the newly-allocated string.
 * @format: a standard printf() format string, but notice
 *          <link linkend="string-precision">string precision pitfalls</link>.
 * @args: the list of arguments to insert in the output.
 *
 * This function allocates a 
 * string to hold the output, instead of putting the output in a buffer 
 * you allocate in advance.
 *
 * Returns: the number of bytes printed.
 **/
int 
vasprintf (char      **string,
           char const *format,
           va_list      args);

char *
strsep (char **stringp, const char *delim);

// convert windows error code to string
wstring sWinErrMsg (DWORD errorCode);

// formats string a la sprintf, max 10k size
wstring sFormat( wstring format, ... );

#define WSFORMAT(e) ((dynamic_cast<const std::wostringstream&>(std::wostringstream().flush() << e)).str())
#ifndef UNICODE
#define SFORMAT(e) ((dynamic_cast<const std::ostringstream&>(std::ostringstream().flush() << e)).str())
#else
#define SFORMAT WSFORMAT
#endif

string AmountFormat(double amt, int precision = 2);
string StripDotZeros (const string& s);
string RateFormat(double rate);	//2 or 4 fractional digits
string FixFormat(double lot, int precision = 1);	//0 or 1 fractional digit

wstring sFormatVa( const wstring & format, va_list list );

// ansi version
string sFormatVaA( const string & format, va_list list );

//string uni2ascii (const wstring & str);
//wstring ascii2uni (const string & str);

#define FORMAT_SYS_ERR(sysFun, sysErr) \
   (SFORMAT("When calling '" << sysFun << " (...)' the system error '" \
   << sWinErrMsg (sysErr) << "' has occured (#" << sysErr << ")."))

// throws std::logic_error, formating string first
__declspec(noreturn) void sThrow( const wchar_t * format, ... );

// load string with given id from resources. Maximum string length is 10k
string loadResourceStr( int id );

void checkHR( HRESULT );

wstring str2wstr( const string & );
string wstr2str( const wstring & );

string toUTF8 (const wstring&);
wstring fromUTF8 (const string&);

inline const char * ptr2ptr( const string & s )
{
  return s.c_str();
}

inline const wchar_t * ptr2ptr( const wstring & s )
{
  return s.c_str();
}

inline const char * sptr( const char * p )
{
  return p ? p : "";
}

inline const char * szptr( const char * p )
{
  return p && *p ? p : 0;
}


template<class T>
inline const char * ptr2sptr( const T & s )
{
  return sptr(ptr2ptr(s));
}

template<class T>
inline const char * ptr2szptr( const T & s )
{
  return szptr(ptr2ptr(s));
}

// append a string representation of an object
// to a string
template <class T>
void toString (const T& object, std::string & s)
{
  std::ostringstream os;
  os << object;
  s += os.str();
}

template <class T>
T fromString (const std::string& s)
{
  T object;
  std::istringstream is (s);
  is >> object;
  return object;
}

#define SMAKE_THROW_FN_DECL(name, XClass)  \
void name( const wchar_t * fmt, ... ); void name(const wstring& msg); 
//void name( const char * fmt, ... ); void name(const string& msg); \

SMAKE_THROW_FN_DECL(sThrow,SException)

#define SMAKE_THROW_MEMBER_DECL(name, XClass)  \
static void name( const wchar_t * fmt, ... );
//static void name( const char * fmt, ... ); \


#define SMAKE_THROW_FN_IMPL(name, XClass)  \
  \
void name( const wchar_t * fmt, ... )  \
{  \
  va_list va;  \
  va_start(va, fmt);  \
  wstring msg(sFormatVa(fmt, va));  \
  va_end(va);  \
  throw XClass(msg);  \
}; void name(const wstring& msg) { throw XClass(msg); };

//template<class X>
//SMAKE_THROW_FN_IMPL(sThrowX, X)

FILETIME TimetToFileTime (time_t t);
time_t FileTimeToTimet (FILETIME ft);

// copy if (see Stroustrup 3rd ed, 18.6.1)

template<class In, class Out, class Pred>
Out copy_if (In first, In last, Out res, Pred p)
{
  while (first != last)
  {
    if (p (*first))
      *res++ = *first;
    ++first;
  }
  return res;
}

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
/***************************************************************
  time.c

  author : uema2
  date   : Nov 30, 2002

  You can freely use, copy, modify, and redistribute
  the whole contents.
***************************************************************/

#include "StdAfx.h"
#include "time_wce.h"

/* globals */
static const __int64 _onesec_in100ns = (__int64)10000000;
//int   timezone, _timezone, altzone;
//int   daylight;
//char *tzname[2];


/* __int64 <--> FILETIME */
static __int64 wce_FILETIME2int64(FILETIME f)
{
	__int64 t;

	t = f.dwHighDateTime;
	t <<= 32;
	t |= f.dwLowDateTime;
	return t;
}

static FILETIME wce_int642FILETIME(__int64 t)
{
	FILETIME f;

	f.dwHighDateTime = (DWORD)((t >> 32) & 0x00000000FFFFFFFF);
	f.dwLowDateTime  = (DWORD)( t        & 0x00000000FFFFFFFF);
	return f;
}

/* FILETIME utility */
static FILETIME wce_getFILETIMEFromYear(WORD year)
{
	SYSTEMTIME s={0};
	FILETIME f;

	s.wYear      = year;
	s.wMonth     = 1;
	s.wDayOfWeek = 1;
	s.wDay       = 1;

	SystemTimeToFileTime( &s, &f );
	return f;
}

static time_t wce_getYdayFromSYSTEMTIME(const SYSTEMTIME* s)
{
	__int64 t;
	FILETIME f1, f2;

	f1 = wce_getFILETIMEFromYear( s->wYear );
	SystemTimeToFileTime( s, &f2 );

	t = wce_FILETIME2int64(f2)-wce_FILETIME2int64(f1);

	return (time_t)((t/_onesec_in100ns)/(60*60*24));
}

/* tm <--> SYSTEMTIME */
SYSTEMTIME wce_tm2SYSTEMTIME(struct tm *t)
{
	SYSTEMTIME s;

	s.wYear      = t->tm_year + 1900;
	s.wMonth     = t->tm_mon  + 1;
	s.wDayOfWeek = t->tm_wday;
	s.wDay       = t->tm_mday;
	s.wHour      = t->tm_hour;
	s.wMinute    = t->tm_min;
	s.wSecond    = t->tm_sec;
	s.wMilliseconds = 0;

	return s;
}

struct tm wce_SYSTEMTIME2tm(SYSTEMTIME *s)
{
	struct tm t;

	t.tm_year  = s->wYear - 1900;
	t.tm_mon   = s->wMonth- 1;
	t.tm_wday  = s->wDayOfWeek;
	t.tm_mday  = s->wDay;
	t.tm_yday  = (int) wce_getYdayFromSYSTEMTIME(s);
	t.tm_hour  = s->wHour;
	t.tm_min   = s->wMinute;
	t.tm_sec   = s->wSecond;
	t.tm_isdst = 0;

	return t;
}

#if 0 // already defined in SCommon.h
/* FILETIME <--> time_t */
time_t wce_FILETIME2time_t(const FILETIME* f)
{
	FILETIME f1601, f1970;
	__int64 t, offset;

	f1601 = wce_getFILETIMEFromYear(1601);
	f1970 = wce_getFILETIMEFromYear(1970);

	offset = wce_FILETIME2int64(f1970) - wce_FILETIME2int64(f1601);

	t = wce_FILETIME2int64(*f);

	t -= offset;
	return (time_t)(t / _onesec_in100ns);
}

FILETIME wce_time_t2FILETIME(const time_t t)
{
	FILETIME f, f1970;
	__int64 time;

	f1970 = wce_getFILETIMEFromYear(1970);

	time = t;
	time *= _onesec_in100ns;
	time += wce_FILETIME2int64(f1970);

	f = wce_int642FILETIME(time);

	return f;
}
#endif

#if 0 
// is it needed ?

/* time.h difinition */
time_t time( time_t *timer )
{
	SYSTEMTIME s;
	FILETIME   f;

	if( timer==NULL ) return 0;

	GetSystemTime( &s );

	SystemTimeToFileTime( &s, &f );

	*timer = wce_FILETIME2time_t(&f);
	return *timer;
}

struct tm *localtime( const time_t *timer )
{
	SYSTEMTIME ss, ls, s;
	FILETIME   sf, lf, f;
	__int64 t, diff;
	static struct tm tms;

	GetSystemTime(&ss);
	GetLocalTime(&ls);

	SystemTimeToFileTime( &ss, &sf );
	SystemTimeToFileTime( &ls, &lf );

	diff = wce_FILETIME2int64(sf) - wce_FILETIME2int64(lf);

	f = wce_time_t2FILETIME(*timer);
	t = wce_FILETIME2int64(f) - diff;
	f = wce_int642FILETIME(t);

	FileTimeToSystemTime( &f, &s );

	tms = wce_SYSTEMTIME2tm(&s);

	return &tms;
}

time_t mktime(struct tm* pt)
{
	SYSTEMTIME ss, ls, s;
	FILETIME   sf, lf, f;
	__int64 diff;

	GetSystemTime(&ss);
	GetLocalTime(&ls);
	SystemTimeToFileTime( &ss, &sf );
	SystemTimeToFileTime( &ls, &lf );

	diff = (wce_FILETIME2int64(lf)-wce_FILETIME2int64(sf))/_onesec_in100ns;

	s = wce_tm2SYSTEMTIME(pt);
	SystemTimeToFileTime( &s, &f );
	return wce_FILETIME2time_t(&f) - (time_t)diff;
}

struct tm *gmtime(const time_t *t)
{
	FILETIME f;
	SYSTEMTIME s;
	static struct tm tms;
	
	f = wce_time_t2FILETIME(*t);
	FileTimeToSystemTime(&f, &s);
	tms = wce_SYSTEMTIME2tm(&s);
	return &tms;
}

char* ctime( const time_t *t )
{
	// Wed Jan 02 02:03:55 1980\n\0
	static char buf[30]={0};
	char week[] = "Sun Mon Tue Wed Thr Fri Sat ";
	char month[]= "Jan Feb Mar Apl May Jun Jul Aug Sep Oct Nov Dec ";
	struct tm tms;

	tms = *localtime(t);

	strncpy( buf,    week+tms.tm_wday*4, 4 );
	strncpy( buf+4,  month+tms.tm_mon*4, 4 );
	sprintf( buf+8,  "%02d ", tms.tm_mday );
	sprintf( buf+11, "%02d:%02d:%02d %d\n", 
		tms.tm_hour, tms.tm_min, tms.tm_sec, tms.tm_year+1900 );
	return buf;
}

char *asctime(const struct tm *pt)
{
	static char buf[30]={0};
	char week[] = "Sun Mon Tue Wed Thr Fri Sat ";
	char month[]= "Jan Feb Mar Apl May Jun Jul Aug Sep Oct Nov Dec ";

	strncpy( buf,    week+pt->tm_wday*4, 4 );
	strncpy( buf+4,  month+pt->tm_mon*4, 4 );
	sprintf( buf+8,  "%02d ", pt->tm_mday );
	sprintf( buf+11, "%02d:%02d:%02d %d\n", 
		pt->tm_hour, pt->tm_min, pt->tm_sec, pt->tm_year+1900 );
	return buf;
}

void tzset()
{
	daylight = 1;
	_timezone = 28800;
	timezone = 28800;
}

int clock(void)
{
	return 1;
}
#endif

//---------------------------------------------------------------
#ifdef __SCRATCH_TIMEC_DEBUG__

int main()
{
	time_t t1, t2;
	struct tm tm1, tm2;

	time( &t1 );
	tm1 = *localtime(&t1);
	t1 = mktime(&tm1);
	tm1 = *gmtime(&t1);

	_time( &t2 );
	tm2 = *_localtime(&t2);
	t2 = _mktime(&tm2);
	tm2 = *_gmtime(&t2);

	// time, mktime
	if( t1==t2 )
		OutputDebugString( "ok\n" );
	else
	{
		static char buf[128];
		wsprintf( buf, "ng : %d, %d\n", t1, t2 );
		OutputDebugString( buf );
	}

	// localtime, gmtime
	if( 0==memcmp( &tm1, &tm2, sizeof(struct tm) ) )
		OutputDebugString( "ok\n" );
	else
		OutputDebugString( "ng\n" );

	// ctime
	OutputDebugString( ctime(&t1) );
	OutputDebugString( _ctime(&t2) );

	// asctime
	OutputDebugString( asctime(&tm1) );
	OutputDebugString( _asctime(&tm2) );

	return 0;
}

#endif



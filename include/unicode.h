/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: unicode.h
**
**/

#ifndef _unicode_h_
#define _unicode_h_

// Undef UNICODE_UCS2 and UNICODE_UCS4 if they are defined to 0
#if !UNICODE_UCS2
#undef UNICODE_UCS2
#endif
#if !UNICODE_UCS4
#undef UNICODE_UCS4
#endif

#ifdef _WIN32

// Ensure that both UNICODE and _UNICODE are defined
#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include <tchar.h>

#ifdef UNICODE

// Windows always use UCS-2
#define UNICODE_UCS2				1

#define _tcstoll  wcstoll
#define _tcstoull wcstoull

#define _ERR_error_tstring		ERR_error_string_W

#else

#define _tcstoll  strtoll
#define _tcstoull strtoull

#define _ERR_error_tstring		ERR_error_string

#endif

#define ICONV_DEFAULT_CODEPAGE	"ACP"

#define ucs2_strlen	wcslen
#define ucs2_strdup	wcsdup
#define ucs2_strncpy	wcsncpy

#define ucs2_to_mb(wstr, wlen, mstr, mlen)	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wstr, wlen, mstr, mlen, NULL, NULL)
#define mb_to_ucs2(mstr, mlen, wstr, wlen)	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mstr, mlen, wstr, wlen)

#define UCS2CHAR	WCHAR

#else    /* not _WIN32 */

#if HAVE_WCHAR_H
#include <wchar.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_WCHAR_T

#define WCHAR     wchar_t
#if UNICODE_UCS2
#define UCS2CHAR  wchar_t
#else
#define UCS2CHAR  unsigned short
#endif

#else	/* wchar_t not presented */

#define WCHAR     unsigned short
#define UCS2CHAR  unsigned short
#undef UNICODE_UCS2
#undef UNICODE_UCS4
#define UNICODE_UCS2 1

#endif

// Use system wide character functions if system's wchar_t is 2 bytes long
#if UNICODE_UCS2

#if HAVE_WCSLEN
#define ucs2_strlen	wcslen
#else
#define wcslen          ucs2_strlen
#endif

#if HAVE_WCSDUP
#define ucs2_strdup	wcsdup
#else
#define wcsdup          ucs2_strdup
#endif

#if HAVE_WCSNCPY
#define ucs2_strncpy	wcsncpy
#else
#define wcsncpy         ucs2_strncpy
#endif

#endif

// On some old systems, ctype.h defines _T macro, so we include it
// before our definition and do an undef
#include <ctype.h>
#undef _T

#ifdef UNICODE

#define _T(x)     L##x
#define TCHAR     wchar_t
#define _TINT     int

#define _tcscpy   wcscpy
#define _tcsncpy  wcsncpy
#define _tcslen   wcslen
#define _tcschr   wcschr
#define _tcsrchr  wcsrchr
#define _tcscmp   wcscmp
#define _tcsicmp  wcsicmp
#define _tcsncmp  wcsncmp
#define _tcsnicmp wcsnicmp
#define _tprintf  nx_wprintf
#define _ftprintf nx_fwprintf
#define _sntprintf nx_swprintf
#define _vtprintf nx_vwprintf
#define _vftprintf nx_vfwprintf
#define _vsntprintf nx_vswprintf
#define _tfopen   wfopen
#define _fgetts   fgetws
#define _fputts   fputws
#define _tcstol   wcstol
#define _tcstoul  wcstoul
#define _tcstoll  wcstoll
#define _tcstoull wcstoull
#define _tcstod   wcstod
#define _tcsdup   wcsdup
#define _tcsupr   wcsupr
#define _tcsspn   wcsspn
#define _tcscspn  wcscspn
#define _tcsstr   wcsstr
#define _tcscat   wcscat
#define _topen    wopen
#define _taccess  waccess
#define _tstat    wstat
#define _tunlink  wunlink
#define _tcsftime wcsftime
#define _tctime   wctime
#define _istspace iswspace
#define _istdigit iswdigit
#define _istxdigit iswxdigit
#define _istalpha iswalpha
#define _istupper iswupper
#define _tgetenv  wgetenv

#else

#define _T(x)     x
#define TCHAR     char
#define _TINT     int

#define _tcscpy   strcpy
#define _tcsncpy  strncpy
#define _tcslen   strlen
#define _tcschr   strchr
#define _tcsrchr  strrchr
#define _tcscmp   strcmp
#define _tcsicmp  stricmp
#define _tcsncmp  strncmp
#define _tcsnicmp strnicmp
#define _tprintf  printf
#define _stprintf sprintf
#define _ftprintf fprintf
#define _sntprintf snprintf
#define _vtprintf vprintf
#define _vstprintf vsprintf
#define _vsntprintf vsnprintf
#define _tfopen   fopen
#define _fgetts   fgets
#define _fputts   fputs
#define _tcstol   strtol
#define _tcstoul  strtoul
#define _tcstoll  strtoll
#define _tcstoull strtoull
#define _tcstod   strtod
#define _tcsdup   strdup
#define _tcsupr   strupr
#define _tcsspn   strspn
#define _tcscspn  strcspn
#define _tcsstr   strstr
#define _tcscat   strcat
#define _topen    open
#define _taccess  access
#define _tstat    stat
#define _tunlink  unlink
#define _tcsftime strftime
#define _tctime   ctime
#define _istspace isspace
#define _istdigit isdigit
#define _istxdigit isxdigit
#define _istalpha isalpha
#define _istupper isupper
#define _tgetenv  getenv

#define _ERR_error_tstring		ERR_error_string

#endif

#define CP_ACP             0
#define CP_UTF8				65001
#define MB_PRECOMPOSED     0x00000001
#define WC_COMPOSITECHECK  0x00000002
#define WC_DEFAULTCHAR     0x00000004

// Default codepage for iconv()
#if HAVE_ICONV_ISO_8859_1
#define ICONV_DEFAULT_CODEPAGE	"ISO-8859-1"
#elif HAVE_ICONV_ISO8859_1
#define ICONV_DEFAULT_CODEPAGE	"ISO8859-1"
#elif HAVE_ICONV_ASCII
#define ICONV_DEFAULT_CODEPAGE	"ASCII"
#else
#define ICONV_DEFAULT_CODEPAGE	""
#endif

#endif	/* _WIN32 */


#ifdef UNICODE
#define _t_inet_addr    inet_addr_w
#else
#define _t_inet_addr    inet_addr
#endif


// Check that either UNICODE_UCS2 or UNICODE_UCS4 are defined
#ifdef UNICODE
#if !defined(UNICODE_UCS2) && !defined(UNICODE_UCS4)
#error Neither UNICODE_UCS2 nor UNICODE_UCS4 are defined
#endif
#endif

#endif   /* _unicode_h_ */


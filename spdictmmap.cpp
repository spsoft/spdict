/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 *
 * These sources are based on ftp://g.oswego.edu/pub/misc/malloc.c
 * file by Doug Lea, released to the public domain.
 *
 * These sources are based on http://websvn.kde.org/trunk/KDE/kdelibs/
 * by Jaroslaw Staniek, released under LGPL.
 *
 */

#include "spdictmmap.hpp"

#ifdef WIN32

#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <io.h>

#ifndef SECTION_MAP_EXECUTE_EXPLICIT
//not defined in the February 2003 version of the Platform  SDK 
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020
#endif

#ifndef FILE_MAP_EXECUTE
//not defined in the February 2003 version of the Platform  SDK 
#define FILE_MAP_EXECUTE SECTION_MAP_EXECUTE_EXPLICIT
#endif

#define MUNMAP_FAILURE  (-1)

#define USE_MALLOC_LOCK 1

struct mmapInfos {
    HANDLE hFile;   // the duplicated fd
    HANDLE hMap;    // handle returned by CreateFileMapping
    void* start;    // ptr returned by MapViewOfFile
};

CRITICAL_SECTION cs;

static g_curMMapInfos = 0;
static g_maxMMapInfos = -1;
static struct mmapInfos *g_mmapInfos = NULL;
#define NEW_MMAP_STRUCT_CNT 10

int _set_errno(int value)
{
	errno = value;
	return errno;
}

static int mapProtFlags(int flags, DWORD *dwAccess)
{
    if ( ( flags & PROT_READ ) == PROT_READ ) {
        if ( ( flags & PROT_WRITE ) == PROT_WRITE ) {
            *dwAccess = FILE_MAP_WRITE;
            if ( ( flags & PROT_EXEC ) == PROT_EXEC ) {
                return PAGE_EXECUTE_READWRITE;
            }
            return PAGE_READWRITE;
        }
        if ( ( flags & PROT_EXEC ) == PROT_EXEC ) {
            *dwAccess = FILE_MAP_EXECUTE;
            return PAGE_EXECUTE_READ;
        }
        *dwAccess = FILE_MAP_READ;
        return PAGE_READONLY;
    }
    if ( ( flags & PROT_WRITE ) == PROT_WRITE ) {
        *dwAccess = FILE_MAP_COPY;
        return PAGE_WRITECOPY;
    }   
    if ( ( flags & PROT_EXEC ) == PROT_EXEC ) {
        *dwAccess = FILE_MAP_EXECUTE;
        return PAGE_EXECUTE_READ;
    }
    *dwAccess = 0;
    return 0;   
}

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    struct mmapInfos mmi;
    DWORD dwAccess;
    DWORD flProtect;
    HANDLE hfd;

    if ( g_maxMMapInfos == -1 ) {
        g_maxMMapInfos = 0;
        InitializeCriticalSection( &cs );
    }

    flProtect = mapProtFlags( prot, &dwAccess );
	if ( flProtect == 0 ) {
        _set_errno( EINVAL );
		return MAP_FAILED;
	}
	// we don't support this atm
	if ( flags == MAP_FIXED ) {
		_set_errno( ENOTSUP );
		return MAP_FAILED;
	}

	if ( fd == -1 ) {
		_set_errno( EBADF );
        return MAP_FAILED;
	}

	hfd = (HANDLE)_get_osfhandle( fd );
	if ( hfd == INVALID_HANDLE_VALUE )
		return MAP_FAILED;

	if ( !DuplicateHandle( GetCurrentProcess(), hfd, GetCurrentProcess(),
                           &mmi.hFile, 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
#ifdef _DEBUG
        DWORD dwLastErr = GetLastError();
#endif
        return MAP_FAILED;
    }
    mmi.hMap = CreateFileMapping( mmi.hFile, NULL, flProtect,
                                  0, length, NULL );
    if ( mmi.hMap == 0 ) {
        _set_errno( EACCES );
		return MAP_FAILED;
    }

    mmi.start = MapViewOfFile( mmi.hMap, dwAccess, 0, offset, 0 );
	if ( mmi.start == 0 ) {
		DWORD dwLastErr = GetLastError();
		if ( dwLastErr == ERROR_MAPPED_ALIGNMENT )
			_set_errno( EINVAL );
		else
			_set_errno( EACCES );
		return MAP_FAILED;
	}
    EnterCriticalSection( &cs );
    if ( g_mmapInfos == NULL ) {
        g_maxMMapInfos = NEW_MMAP_STRUCT_CNT;
        g_mmapInfos = ( struct mmapInfos* )calloc( g_maxMMapInfos,
                      sizeof( struct mmapInfos ) );
    }
    if( g_curMMapInfos == g_maxMMapInfos) {
        g_maxMMapInfos += NEW_MMAP_STRUCT_CNT;
        g_mmapInfos = ( struct mmapInfos* )realloc( g_mmapInfos,
                      g_maxMMapInfos * sizeof( struct mmapInfos ) );
    }
    memcpy( &g_mmapInfos[g_curMMapInfos], &mmi, sizeof( struct mmapInfos) );
    g_curMMapInfos++;
    
    LeaveCriticalSection( &cs );
    
    return mmi.start;
}

int munmap(void *start, size_t length)
{
    int i, j;

    for( i = 0; i < g_curMMapInfos; i++ ) {
        if( g_mmapInfos[i].start == start )
            break;
    }
    if( i == g_curMMapInfos ) {
        _set_errno( EINVAL );
        return -1;
    }

    UnmapViewOfFile( g_mmapInfos[g_curMMapInfos].start );
    CloseHandle( g_mmapInfos[g_curMMapInfos].hMap );
    CloseHandle( g_mmapInfos[g_curMMapInfos].hFile );

    EnterCriticalSection( &cs );
    for( j = i + 1; j < g_curMMapInfos; j++ ) {
        memcpy( &g_mmapInfos[ j - 1 ], &g_mmapInfos[ j ],
                sizeof( struct mmapInfos ) );
    }
    g_curMMapInfos--;
    
    if( g_curMMapInfos == 0 ) {
        free( g_mmapInfos );
        g_mmapInfos = NULL;
        g_maxMMapInfos = 0;
    }
    LeaveCriticalSection( &cs );
    
    return 0;
}

#endif


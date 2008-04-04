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

#ifndef __spdictmmap_hpp__
#define __spdictmmap_hpp__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32

#include <io.h>
#define ftruncate chsize

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

/* These values don't really matter in windows mmap emulation */
#define MAP_FILE 0
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_TYPE 0xF
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_ANON MAP_ANONYMOUS

#define MAP_FAILED ((void *)-1)

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *start, size_t length);

int __cdecl _set_errno(int value);

#define ENOTSUP ENOSYS

#endif

#ifdef __cplusplus
}
#endif

#ifndef WIN32

#include <sys/mman.h>
#include <unistd.h>

#endif

#endif


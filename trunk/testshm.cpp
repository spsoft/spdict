/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "spdictshm.hpp"

int main( int argc, char * argv[] )
{
	const char * filePath = "./testshm.map";
	int len = 1024;

	int isNewFile = 0;
	void * ptrBase = SP_DictShmAllocator::getMmapPtr(
			filePath, len, &isNewFile );

	assert( NULL != ptrBase );

	SP_DictShmAllocator allocator( ptrBase, len, 7 );
	allocator.reset();

	srand( time( NULL ) );

	for( int i = 0; ; i++ )
	{
		off_t offset = allocator.alloc();

		if( offset <= 0 )
		{
			printf( "%d\n", i );
			break;
		}

		void * buff = allocator.getPtr( offset );

		strncpy( (char*)buff, "123456", 7 );

		if( 0 == i % 2  ) allocator.free( offset );
	}

	SP_DictShmAllocator::freeMmapPtr( ptrBase, len );

	return 0;
}


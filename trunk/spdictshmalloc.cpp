/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <set>

using namespace std;

#include "spdictshmalloc.hpp"
#include "spdictmmap.hpp"

SP_DictShmAllocator :: SP_DictShmAllocator( void * ptrBase, size_t len, size_t itemSize )
{
	mPtrBase = (char*)ptrBase;
	mLen = len;
	mItemSize = itemSize;

	mRecordSize = sizeof( char ) + itemSize;

	mFirst = (Chunk_t*)ptrBase;
}

SP_DictShmAllocator :: ~SP_DictShmAllocator()
{
}

int SP_DictShmAllocator :: getFreeCount() const
{
	int count = 0;

	for( int idx = mFirst->mNext; idx > 0; ) {
		Chunk_t * chunk = (Chunk_t*)( mPtrBase + idx * mRecordSize );
		idx = chunk->mNext;
		count++;
	}

	return count;
}

size_t SP_DictShmAllocator :: alloc()
{
	size_t ret = 0;

	if( mFirst->mNext > 0 ) {
		Chunk_t * chunk = (Chunk_t*)( mPtrBase + mFirst->mNext * mRecordSize );

		assert( FLAG_FREE == chunk->mFlags );

		mFirst->mNext = chunk->mNext;

		memset( chunk, 0, mRecordSize );
		chunk->mFlags = FLAG_USED;

		ret = ( chunk->mPtr - mPtrBase );
	}

	return ret;
}

void SP_DictShmAllocator :: free( size_t offset )
{
	assert( isValid( offset ) );

	int index = ( offset / mRecordSize );

	Chunk_t * chunk = (Chunk_t*)( mPtrBase + index * mRecordSize );

	assert( FLAG_USED == chunk->mFlags );

	memset( chunk, 0, mRecordSize );
	chunk->mFlags = FLAG_FREE;
	chunk->mNext = mFirst->mNext;
	mFirst->mNext = index;
}

int SP_DictShmAllocator :: isValid( size_t offset ) const
{
	int ret = 0;

	if( offset > 0 && offset < (size_t)mLen ) {
		int index = ( offset / mRecordSize );

		Chunk_t * chunk = (Chunk_t*)( mPtrBase + index * mRecordSize );

		if( (int)offset == ( chunk->mPtr - mPtrBase ) ) ret = 1;
	}

	return ret;
}

int SP_DictShmAllocator :: isUsed( size_t offset ) const
{
	int ret = 0;

	if( offset > 0 && offset < (size_t)mLen ) {
		int index = ( offset / mRecordSize );

		Chunk_t * chunk = (Chunk_t*)( mPtrBase + index * mRecordSize );

		if( (int)offset == ( chunk->mPtr - mPtrBase )
				&& FLAG_USED == chunk->mFlags ) ret = 1;
	}

	return ret;
}

void * SP_DictShmAllocator :: getPtr( size_t offset ) const
{
	assert( isValid( offset ) );

	return mPtrBase + offset;
}

size_t SP_DictShmAllocator :: getOffset( void * ptr ) const
{
	size_t ret = ((char*)ptr) - mPtrBase;

	assert( isValid( ret ) );

	return ret;
}

void SP_DictShmAllocator :: reset()
{
	int count = mLen / mRecordSize;

	for( int i = 0; i < count; i++ ) {
		Chunk_t * chunk = (Chunk_t*)( mPtrBase + i * mRecordSize );

		memset( chunk, 0, mRecordSize );

		chunk->mFlags = FLAG_FREE;

		if( i < count - 1 ) {
			chunk->mNext = i + 1;
		} else {
			chunk->mNext = 0;
		}
	}
}

void SP_DictShmAllocator :: check( CheckFunc_t checkFunc, void * arg )
{
	int count = mLen / mRecordSize;

	memset( mFirst, 0, mRecordSize );

	int i = 0;

	for( i = 0; i < count; i++ ) {
		Chunk_t * chunk = (Chunk_t*)( mPtrBase + i * mRecordSize );

		if( FLAG_USED == chunk->mFlags ) {
			if( 0 == checkFunc( chunk->mPtr, arg ) ) {
				memset( chunk, 0, mRecordSize );
				chunk->mFlags = FLAG_USED;
			}
		}
	}

	for( i = count - 1; i > 0; i-- ) {
		Chunk_t * chunk = (Chunk_t*)( mPtrBase + i * mRecordSize );

		if( FLAG_FREE == chunk->mFlags ) {
			chunk->mNext = mFirst->mNext;
			mFirst->mNext = i;
		}
	}
}

void SP_DictShmAllocator :: selfCheck( size_t * freeCount, size_t * usedCount )
{
	int count = mLen / mRecordSize;

	// 1. every record is free or used
	assert( 0 == mFirst->mFlags );

	for( int i = 1; i < count; i++ ) {
		Chunk_t * chunk = (Chunk_t*)( mPtrBase + i * mRecordSize );

		if( FLAG_FREE == chunk->mFlags ) {
			(*freeCount) ++;
		} else if( FLAG_USED == chunk->mFlags ) {
			(*usedCount) ++;
		} else {
			assert( 0 );
		}
	}

	set<int> freeSet;

	// 2. check loop free record
	for( int idx = mFirst->mNext; idx > 0; ) {
		Chunk_t * chunk = (Chunk_t*)( mPtrBase + idx * mRecordSize );

		assert( FLAG_FREE == chunk->mFlags );

		set<int>::iterator it = freeSet.find( idx );
		assert( freeSet.end() == it );
		freeSet.insert( idx );

		idx = chunk->mNext;
	}

	// 3. all free record is in freelist
	assert( *freeCount == freeSet.size() );
}

void * SP_DictShmAllocator :: getMmapPtr( const char * filePath,
		size_t len, int * isNewFile )
{
	*isNewFile = 0;

	int fd = open( filePath, O_RDWR );
	if( fd < 0 ) {
		if( ENOENT == errno ) {
			fd = open( filePath, O_RDWR | O_CREAT, 0666 );
			if( fd >= 0 ) {
				*isNewFile = 1;

				char buffer[ 1024 ] = { 0 };
				memset( buffer, 0, sizeof( buffer ) );
				for( int i = 0; i < (int)len; i += sizeof( buffer ) ) {
					write( fd, buffer, sizeof( buffer ) );
				}

				ftruncate( fd, len );

				lseek( fd, 0, SEEK_SET );
			} else {
				printf( "Create %s fail, errno %d, %s\n",
						filePath, errno, strerror( errno ) );
			}
		} else {
			printf( "Open %s fail, errno %d, %s\n",
					filePath, errno, strerror( errno ) );
		}
	}

	void * ret = NULL;

	if( fd >= 0 ) {
		struct stat fileStat;
		if( 0 == fstat( fd, &fileStat ) ) {
			if( fileStat.st_size == (int)len ) {
				ret = mmap( 0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
				if( MAP_FAILED == ret ) {
					printf( "mmap %s fail, errno %d, %s\n",
							filePath, errno, strerror( errno ) );
					ret = NULL;
				}
			} else {
				printf( "invalid file size, real %li, except %i",
						fileStat.st_size, len );
			}
		} else {
			printf( "Stat %s fail, errno %d, %s\n",
					filePath, errno, strerror( errno ) );
		}

		close( fd );
	}

	return ret;
}

void SP_DictShmAllocator :: freeMmapPtr( void * ptr, size_t len )
{
	if( 0 != munmap( ptr, len ) ) {
		printf( "munmap fail, errno %d, %s\n",
				errno, strerror( errno ) );
	}
}


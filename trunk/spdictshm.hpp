/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spdictshm_hpp__
#define __spdictshm_hpp__

#include <sys/types.h>

class SP_DictShmAllocator {
public:
	SP_DictShmAllocator( void * ptrBase, size_t len, size_t itemSize );
	~SP_DictShmAllocator();

	// @return 0 : out of memory, > 0 : the buffer offset
	size_t alloc();

	void free( size_t offset );

	// @return 1 : valid offset, 0 : invalid offset
	int isValid( size_t offset ) const;

	// @return 1 : has been used, 0 : free
	int isUsed( size_t offset ) const;

	// convert offset to pointer
	void * getPtr( size_t offset ) const;

	// convert pointer to offset
	size_t getOffset( void * ptr ) const;

	void reset();

	int getFreeCount() const;

	// @return 1 : check OK, 0 : check Fail
	typedef int ( * CheckFunc_t ) ( void * ptr, void * arg );

	void check( CheckFunc_t checkFunc, void * arg );

	void selfCheck( size_t * freeCount, size_t * usedCount );

	static void * getMmapPtr( const char * filePath, size_t len, int * isNewFile );

	static void freeMmapPtr( void * ptr, size_t len );

private:

	typedef struct tagChunk {
		char mFlags;
		union {
			int mNext;
			char mPtr[1];
		};
	} Chunk_t;

	char * mPtrBase;
	size_t mLen, mItemSize;
	size_t mRecordSize;

	Chunk_t * mFirst;

	enum { FLAG_FREE = 0x01, FLAG_USED = 0x02 };
};

#endif


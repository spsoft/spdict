/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spdictshmhashmap_hpp__
#define __spdictshmhashmap_hpp__

#include <sys/types.h>

class SP_DictShmAllocator;

class SP_DictShmHashMapHandler {
public:
	virtual ~SP_DictShmHashMapHandler() {}

	virtual unsigned int hash( const void * item ) = 0;

	// @return 1 : item1 > item2, 0 : item1 == item2, -1 : item1 < item2
	virtual int compare( const void * item1, const void * item2 ) = 0;
};

typedef struct tagSP_DictShmHashMapEntry {
	size_t mEvictPrev, mEvictNext;
	size_t mKeyNext;
	unsigned long int mCheckSum;
	time_t mExpTime;
	char mPtr[1];
} SP_DictShmHashMapEntry_t;

class SP_DictShmHashMapEntryList {
public:
	SP_DictShmHashMapEntryList( size_t * evictHeader, size_t * evictTail,
			const SP_DictShmAllocator * allocator );
	~SP_DictShmHashMapEntryList();

	SP_DictShmHashMapEntry_t * getHead();

	void append( SP_DictShmHashMapEntry_t * entry );

	void remove( SP_DictShmHashMapEntry_t * entry );

	void update( SP_DictShmHashMapEntry_t * entry );

private:
	const SP_DictShmAllocator * mAllocator;

	size_t * mEvictHeader;
	size_t * mEvictTail;
};

class SP_DictShmHashMap {
public:
	SP_DictShmHashMap( size_t * bucketList, size_t maxBucket,
			const SP_DictShmAllocator * allocator, SP_DictShmHashMapHandler * handler );
	~SP_DictShmHashMap();

	size_t getCount();

	void put( SP_DictShmHashMapEntry_t * entry );

	SP_DictShmHashMapEntry_t * get( const void * keyItem );

	SP_DictShmHashMapEntry_t * remove( const void * keyItem );

private:
	const SP_DictShmAllocator * mAllocator;
	SP_DictShmHashMapHandler * mHandler;
	size_t mCount;

	size_t * mBucketList;
	size_t mMaxBucket;
};

#endif


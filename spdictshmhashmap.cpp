/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <assert.h>

#include "spdictshmhashmap.hpp"
#include "spdictshmalloc.hpp"

SP_DictShmHashMapEntryList :: SP_DictShmHashMapEntryList( size_t * evictHeader,
		size_t * evictTail, const SP_DictShmAllocator * allocator )
{
	mAllocator = allocator;
	mEvictHeader = evictHeader;
	mEvictTail = evictTail;
}

SP_DictShmHashMapEntryList :: ~SP_DictShmHashMapEntryList()
{
}

SP_DictShmHashMapEntry_t * SP_DictShmHashMapEntryList :: getHead()
{
	SP_DictShmHashMapEntry_t * entry = NULL;

	if( *mEvictHeader  > 0 ) {
		entry = (SP_DictShmHashMapEntry_t*)( mAllocator->getPtr( *mEvictHeader ) );
	}

	return entry;
}

void SP_DictShmHashMapEntryList :: append( SP_DictShmHashMapEntry_t * entry )
{
	size_t offset = mAllocator->getOffset( entry );

	entry->mEvictPrev = 0;
	entry->mEvictNext = 0;

	if( 0 == *mEvictTail ) {
		* mEvictHeader = * mEvictTail = offset;
	} else {
		SP_DictShmHashMapEntry_t * tailEntry = (SP_DictShmHashMapEntry_t*)( mAllocator->getPtr( *mEvictTail ) );

		tailEntry->mEvictNext = offset;
		entry->mEvictPrev = * mEvictTail;

		* mEvictTail = offset;
	}
}

void SP_DictShmHashMapEntryList :: remove( SP_DictShmHashMapEntry_t * entry )
{
	size_t prev = entry->mEvictPrev, next = entry->mEvictNext;

	size_t curr = mAllocator->getOffset( entry );

	if( curr == *mEvictHeader ) assert( 0 == prev );
	if( curr == *mEvictTail ) assert( 0 == next );

	if( 0 == prev ) {
		*mEvictHeader = next;
	} else {
		SP_DictShmHashMapEntry_t * prevEntry = (SP_DictShmHashMapEntry_t*)( mAllocator->getPtr( prev ) );
		prevEntry->mEvictNext = next;
	}

	if( 0 == next ) {
		* mEvictTail = prev;
	} else {
		SP_DictShmHashMapEntry_t * nextEntry = (SP_DictShmHashMapEntry_t*)( mAllocator->getPtr( next ) );
		nextEntry->mEvictPrev = prev;
	}

	entry->mEvictPrev = 0;
	entry->mEvictNext = 0;
}

void SP_DictShmHashMapEntryList :: update( SP_DictShmHashMapEntry_t * entry )
{
	remove( entry );
	append( entry );
}

//---------------------------------------------------------------------------

SP_DictShmHashMap :: SP_DictShmHashMap( size_t * bucketList, size_t maxBucket,
		const SP_DictShmAllocator * allocator, SP_DictShmHashMapHandler * handler )
{
	mAllocator = allocator;
	mHandler = handler;

	mBucketList = bucketList;
	mMaxBucket = maxBucket;

	mCount = 0;

	for( int i = 0; i < (int)mMaxBucket; i++ ) {
		size_t iter = mBucketList[i];
		for( ; iter > 0; ) {
			SP_DictShmHashMapEntry_t * entry = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( iter );
			iter = entry->mKeyNext;

			mCount++;
		}
	}
}

SP_DictShmHashMap :: ~SP_DictShmHashMap()
{
}

size_t SP_DictShmHashMap :: getCount()
{
	return mCount;
}

void SP_DictShmHashMap :: put( SP_DictShmHashMapEntry_t * entry )
{
	size_t offset = mAllocator->getOffset( entry );

	size_t bucket = mHandler->hash( entry->mPtr ) % mMaxBucket;
	entry->mKeyNext = mBucketList[ bucket ];
	mBucketList[ bucket ] = offset;

	mCount++;
}

SP_DictShmHashMapEntry_t * SP_DictShmHashMap :: get( const void * keyItem )
{
	SP_DictShmHashMapEntry_t * ret = NULL;

	size_t bucket = mHandler->hash( keyItem ) % mMaxBucket;

	for( size_t iter = mBucketList[ bucket ]; iter > 0; ) {
		SP_DictShmHashMapEntry_t * entry = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( iter );

		if( 0 == mHandler->compare( entry->mPtr, keyItem ) ) {
			ret = entry;
			break;
		}

		iter = entry->mKeyNext;
	}

	return ret;
}

SP_DictShmHashMapEntry_t * SP_DictShmHashMap :: remove( const void * keyItem )
{
	SP_DictShmHashMapEntry_t * ret = NULL;

	int bucket = mHandler->hash( keyItem ) % mMaxBucket;

	for( size_t * iter = &( mBucketList[ bucket ] ); *iter > 0; ) {
		SP_DictShmHashMapEntry_t * entry = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( *iter );

		if( 0 == mHandler->compare( entry->mPtr, keyItem ) ) {
			ret = entry;
			*iter = entry->mKeyNext;

			mCount--;

			break;
		}

		iter = &( entry->mKeyNext );
	}

	return ret;
}


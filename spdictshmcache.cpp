/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>

#pragma warning(disable : 4786)

#include <map>
#include <set>

using namespace std;

#include "spdictshmcache.hpp"
#include "spdictshmalloc.hpp"
#include "spdictshmhashmap.hpp"

class SP_DictShmHashMapHandlerAdapter : public SP_DictShmHashMapHandler {
public:
	SP_DictShmHashMapHandlerAdapter( SP_DictShmCacheHandler * handler ) {
		mHandler = handler;
	}

	unsigned int hash( const void * item ) {
		return mHandler->hash( item );
	}

	virtual int compare( const void * item1, const void * item2 ) {
		return mHandler->compare( item1, item2 );
	}

private:
	SP_DictShmCacheHandler * mHandler;
};

//---------------------------------------------------------------------------

SP_DictShmCacheStatistics :: SP_DictShmCacheStatistics()
{
	mHits = mAccesses = mSize = 0;
}

SP_DictShmCacheStatistics :: SP_DictShmCacheStatistics( SP_DictShmCacheStatistics & other )
{
	mHits = other.getHits();
	mAccesses = other.getAccesses();
	mSize = other.getSize();
}

SP_DictShmCacheStatistics :: ~SP_DictShmCacheStatistics()
{
}

int SP_DictShmCacheStatistics :: getHits() const
{
	return mHits;
}

int SP_DictShmCacheStatistics :: getAccesses() const
{
	return mAccesses;
}

int SP_DictShmCacheStatistics :: getSize() const
{
	return mSize;
}

void SP_DictShmCacheStatistics :: setSize( int size )
{
	mSize = size;
}

void SP_DictShmCacheStatistics :: markHit()
{
	mHits++;
	mAccesses++;
}

void SP_DictShmCacheStatistics :: markMiss()
{
	mAccesses++;
}

//---------------------------------------------------------------------------

SP_DictShmCache :: SP_DictShmCache( SP_DictShmCacheHandler * handler,
		size_t maxBucket, size_t itemSize )
{
	mAllocator = NULL;
	mHeader = NULL;

	mHandler = handler;
	mMaxBucket = maxBucket;
	mItemSize = itemSize;
	mRecordSize = sizeof( SP_DictShmHashMapEntry_t ) + itemSize;

	mEvictAlgo = eFIFO;
}

SP_DictShmCache :: ~SP_DictShmCache()
{
	if( NULL != mAllocator ) delete mAllocator;
	mAllocator = NULL;

	if( NULL != mEvictList ) delete mEvictList;
	mEvictList = NULL;

	if( NULL != mHandler ) delete mHandler;
	mHandler = NULL;

	if( NULL != mHeader ) {
		size_t headerLen = sizeof( Header_t ) + sizeof( size_t ) * mMaxBucket;
		size_t totalLen = mHeader->mLen + headerLen;
		SP_DictShmAllocator::freeMmapPtr( (void*) mHeader, totalLen );

		mHeader = NULL;
	}
}

void SP_DictShmCache :: setEvictAlgo( int evictAlgo )
{
	mEvictAlgo = evictAlgo;
}

int SP_DictShmCache :: init( const char * filePath, size_t len )
{
	int retCode = -1;

	int isNewFile = 0;

	size_t headerLen = sizeof( Header_t ) + sizeof( size_t ) * mMaxBucket;
	void * ptrHeader = SP_DictShmAllocator::getMmapPtr(
			filePath, len + headerLen, &isNewFile );

	if( NULL != ptrHeader ) {
		//printf( "%s file %s", isNewFile ? "create" : "reuse", filePath );

		mHeader = (Header_t*)ptrHeader;
		mAllocator = new SP_DictShmAllocator( (char*)ptrHeader + headerLen, len, mRecordSize );

		int isHeaderValid = 1;

		if( isNewFile ) {
			retCode = 0;

			mHeader->mType0 = 'S';
			mHeader->mType1 = 'P';
			mHeader->mLen = len;
			mHeader->mMaxBucket = mMaxBucket;
			mHeader->mItemSize = mItemSize;
			mHeader->mEvictHeader = 0;
			mHeader->mEvictTail = 0;
			memset( mHeader->mBucket, 0, sizeof( size_t ) * mMaxBucket );

			mAllocator->reset();
		} else {
			retCode = 1;

			if( mHeader->mType0 != 'S' || mHeader->mType1 != 'P' ) {
				printf( "init %s fail, invalid type, %c%c",
						filePath, mHeader->mType0, mHeader->mType1 );
				isHeaderValid = 0;
				retCode = -1;
			}

			if( mHeader->mLen != len || mHeader->mMaxBucket != mMaxBucket
					|| mHeader->mItemSize != mItemSize ) {
				printf( "init %s fail, invalid metadata, "
						"len %d %d, max.bucket %d %d, item.size %d %d",
						filePath, mHeader->mLen, len, mHeader->mMaxBucket, mMaxBucket,
						mHeader->mItemSize, mItemSize );
				isHeaderValid = 0;
				retCode = -1;
			}
		}

		if( isHeaderValid ) {
			CheckArg checkArg;
			checkArg.first = mItemSize;

			// check allocator
			mAllocator->check( checkFunc, &checkArg );

			// rebuild hashmap and evictlist
			mHeader->mEvictHeader = 0;
			mHeader->mEvictTail = 0;
			memset( mHeader->mBucket, 0, sizeof( size_t ) * mMaxBucket );

			mEvictList = new SP_DictShmHashMapEntryList( &( mHeader->mEvictHeader ),
					&( mHeader->mEvictTail ), mAllocator );
			mHashMap = new SP_DictShmHashMap( mHeader->mBucket, mMaxBucket,
					mAllocator, new SP_DictShmHashMapHandlerAdapter( mHandler ) );

			for( EntryMap::iterator it = checkArg.second.begin();
					checkArg.second.end() != it; it++ ) {
				mHashMap->put( it->second );
				mEvictList->append( it->second );
			}
		}

		//printf( "allocator.count %d", mAllocator->getFreeCount() );
	}

	return retCode;
}

int SP_DictShmCache :: checkFunc( void * ptr, void * arg )
{
	CheckArg * checkArg = ( CheckArg * ) arg;

	SP_DictShmHashMapEntry_t * entry = (SP_DictShmHashMapEntry_t*)ptr;

	int ret = 0;

	unsigned long int checksum = fnvHash( entry->mPtr, checkArg->first );

	if( entry->mCheckSum == checksum ) {
		ret = 1;

		checkArg->second.insert( make_pair( entry->mExpTime, entry ) );
	} else {
		printf( "checksum fail, %ld, %ld\n", entry->mCheckSum, checksum );
	}

	return ret;
}

void SP_DictShmCache :: selfCheck()
{
	size_t freeCount = 0, usedCount = 0;

	// 1. check allocator
	mAllocator->selfCheck( &freeCount, &usedCount );

	set<size_t> entrySet;

	// 2. check hashmap
	for( int i = 0; i < (int)mMaxBucket; i++ ) {
		size_t iter = mHeader->mBucket[i];
		for( ; iter > 0; ) {
			// 2.1 check loop entry
			set<size_t>::iterator it = entrySet.find( iter );
			assert( entrySet.end() == it );
			entrySet.insert( iter );

			// 2.2 entry must been used
			assert( mAllocator->isUsed( iter ) );

			SP_DictShmHashMapEntry_t * entry = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( iter );

			iter = entry->mKeyNext;
		}
	}

	// 2.3. all used record must been in hashmap
	assert( usedCount == entrySet.size() );

	entrySet.clear();

	// 3. check evictlist
	size_t evictPrev = 0;
	for( SP_DictShmHashMapEntry_t * entry = mEvictList->getHead(); NULL != entry; ) {
		size_t iter = mAllocator->getOffset( entry );

		// 3.1 check loop entry
		set<size_t>::iterator it = entrySet.find( iter );
		assert( entrySet.end() == it );
		entrySet.insert( iter );

		// 3.2 entry must been in hashmap
		assert( entry == mHashMap->get( entry->mPtr ) );

		// 3.3 check prev point
		assert( evictPrev == entry->mEvictPrev );
		evictPrev = iter;

		if( entry->mEvictNext > 0 ) {
			entry = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( entry->mEvictNext );
		} else {
			break;
		}
	}

	// 3.5 all used count must been in evictlist
	assert( usedCount == entrySet.size() );
}

int SP_DictShmCache :: get( const void * keyItem, void * resultHolder )
{
	SP_DictShmHashMapEntry_t * entry = mHashMap->get( keyItem );

	if( NULL != entry ) {
		if( entry->mExpTime > 0 && entry->mExpTime < time( NULL ) ) {
			mHandler->onDestroy( entry->mPtr );

			entry = mHashMap->remove( keyItem );
			mEvictList->remove( entry );
			mAllocator->free( mAllocator->getOffset( entry ) );

			entry = NULL;
		} else {
			mHandler->onHit( entry->mPtr, resultHolder );

			if( eLRU == mEvictAlgo ) {
				mEvictList->update( entry );
			}
		}
	}

	if( NULL != entry ) {
		mStat.markHit();
	} else {
		mStat.markMiss();
	}

	return NULL == entry ? 0 : 1;
}

int SP_DictShmCache :: put( void * item, time_t expTime )
{
	int retCode = -1;

	SP_DictShmHashMapEntry_t * entry = mHashMap->get( item );

	if( NULL != entry ) {
		retCode = 1;

		memcpy( entry->mPtr, item, mItemSize );
		entry->mCheckSum = fnvHash( entry->mPtr, mItemSize );

		entry->mExpTime = expTime;
		mEvictList->update( entry );
	} else {
		size_t offset = mAllocator->alloc();
		if( 0 == offset ) {
			SP_DictShmHashMapEntry_t * iter = mEvictList->getHead();

			if( NULL != iter && iter->mExpTime > 0 && iter->mExpTime < time( NULL ) ) {
				mHandler->onDestroy( iter->mPtr );

				mHashMap->remove( iter->mPtr );
				mEvictList->remove( iter );
				mAllocator->free( mAllocator->getOffset( iter ) );
			}

			offset = mAllocator->alloc();
		}

		if( offset > 0 ) {
			retCode = 0;

			entry = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( offset );

			memcpy( entry->mPtr, item, mItemSize );
			entry->mCheckSum = fnvHash( entry->mPtr, mItemSize );
			mHashMap->put( entry );

			entry->mExpTime = expTime;
			mEvictList->append( entry );
		}
	}

	return retCode;
}

int SP_DictShmCache :: erase( const void * keyItem )
{
	int ret = 0;

	SP_DictShmHashMapEntry_t * entry = mHashMap->remove( keyItem );

	if( NULL != entry ) {
		ret = 1;

		mHandler->onDestroy( entry->mPtr );

		mEvictList->remove( entry );
		mAllocator->free( mAllocator->getOffset( entry ) );
	}

	return ret;
}

const SP_DictShmCacheStatistics * SP_DictShmCache :: getStatistics()
{
	SP_DictShmCacheStatistics * ret = new SP_DictShmCacheStatistics( mStat );
	ret->setSize( mHashMap->getCount() );

	return ret;
}

void SP_DictShmCache :: dumpHash()
{
	for( int i = 0; i < (int)mMaxBucket; i++ ) {
		size_t iter = mHeader->mBucket[i];
		for( ; iter > 0; ) {
			SP_DictShmHashMapEntry_t * entry = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( iter );

			mHandler->onDumpHash( i, entry->mPtr );

			iter = entry->mKeyNext;
		}
	}
}

void SP_DictShmCache :: dumpEvict()
{
	for( SP_DictShmHashMapEntry_t * iter = mEvictList->getHead(); NULL != iter; ) {
		mHandler->onDumpEvict( iter->mExpTime, iter->mPtr );

		if( iter->mEvictNext > 0 ) {
			iter = (SP_DictShmHashMapEntry_t*)mAllocator->getPtr( iter->mEvictNext );
		} else {
			break;
		}
	}
}

unsigned int SP_DictShmCache :: fnvHash( const char * key, size_t len )
{
	static const unsigned int FNV_32_INIT = 2166136261UL;
	static const unsigned int FNV_32_PRIME = 16777619;

	unsigned int hash = FNV_32_INIT;

	for( unsigned int x = 0; x < len; x++ ) {
		hash *= FNV_32_PRIME;
		hash ^= key[x];
	}

	return hash;
}


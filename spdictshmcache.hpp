/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spdictshmcache_hpp__
#define __spdictshmcache_hpp__

#include <sys/types.h>
#include <time.h>
#include <map>

using namespace std;

class SP_DictShmAllocator;
class SP_DictShmHashMapEntryList;
class SP_DictShmHashMap;
typedef struct tagSP_DictShmHashMapEntry SP_DictShmHashMapEntry_t;

class SP_DictShmCacheHandler {
public:
	virtual ~SP_DictShmCacheHandler() {}

	virtual unsigned int hash( const void * item ) = 0;

	// @return 1 : item1 > item2, 0 : item1 == item2, -1 : item1 < item2
	virtual int compare( const void * item1, const void * item2 ) = 0;

	virtual void onHit( const void * item, void * resultHolder ) = 0;

	virtual void onDestroy( const void * item ) {}

	virtual void onDumpHash( int bucket, const void * item ) {}

	virtual void onDumpEvict( time_t expTime, const void * item ) {}
};

class SP_DictShmCacheStatistics {
public:
	SP_DictShmCacheStatistics();
	SP_DictShmCacheStatistics( SP_DictShmCacheStatistics & other );
	virtual ~SP_DictShmCacheStatistics();

	virtual int getHits() const;
	virtual int getAccesses() const;
	virtual int getSize() const;

	void setSize( int size );
	void markHit();
	void markMiss();

private:
	int mHits, mAccesses, mSize;
};

class SP_DictShmCache {
public:
	SP_DictShmCache( SP_DictShmCacheHandler * handler, size_t maxBucket, size_t itemSize );

	~SP_DictShmCache();

	// @return 0 : init ok, create file, 1 : init ok, reuse file, -1 : init Fail
	int init( const char * filePath, size_t len );

	enum { eFIFO, eLRU };

	// default is fifo
	void setEvictAlgo( int evictAlgo );

	// @return 0 : no such key, 1 : found it
	int get( const void * keyItem, void * resultHolder );

	// @return 0 : insert ok, 1 : update ok, -1 : Out of memory
	int put( void * item, time_t expTime = 0 );

	// @return 0 : no such key, 1 : erase it
	int erase( const void * keyItem );

	// caller need to delete the return object
	const SP_DictShmCacheStatistics * getStatistics();

	void dumpHash();

	void dumpEvict();

	void selfCheck();

	static unsigned int fnvHash( const char * key, size_t len );

private:
	typedef struct tagHeader {
		char mType0;
		char mType1;
		size_t mLen;
		size_t mMaxBucket;
		size_t mItemSize;
		size_t mEvictHeader, mEvictTail;
		size_t mBucket[1];
	} Header_t;

	SP_DictShmCacheHandler * mHandler;
	size_t mItemSize, mRecordSize;
	size_t mMaxBucket;
	Header_t * mHeader;

	SP_DictShmAllocator * mAllocator;
	SP_DictShmHashMapEntryList * mEvictList;
	SP_DictShmHashMap * mHashMap;

	SP_DictShmCacheStatistics mStat;

	int mEvictAlgo;

	typedef multimap< time_t, SP_DictShmHashMapEntry_t * > EntryMap;
	typedef pair< int, EntryMap > CheckArg;

	static int checkFunc( void * ptr, void * arg );
};

#endif


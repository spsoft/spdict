/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#ifndef WIN32
#include <pthread.h>
#else
#include <windows.h>
#endif

#include "spcache.hpp"
#include "spdictionary.hpp"

//===========================================================================

SP_CacheHandler :: ~SP_CacheHandler()
{
}

//===========================================================================

SP_CacheStatistics :: ~SP_CacheStatistics()
{
}

class SP_CacheStatisticsImpl : public SP_CacheStatistics {
public:
	SP_CacheStatisticsImpl();
	SP_CacheStatisticsImpl( SP_CacheStatisticsImpl & other );
	virtual ~SP_CacheStatisticsImpl();

	virtual int getHits() const;
	virtual int getAccesses() const;
	virtual int getSize() const;

	void setSize( int size );
	void markHit();
	void markMiss();

private:
	int mHits, mAccesses, mSize;
};

SP_CacheStatisticsImpl :: SP_CacheStatisticsImpl()
{
	mHits = mAccesses = mSize = 0;
}

SP_CacheStatisticsImpl :: SP_CacheStatisticsImpl( SP_CacheStatisticsImpl & other )
{
	mHits = other.getHits();
	mAccesses = other.getAccesses();
	mSize = other.getSize();
}

SP_CacheStatisticsImpl :: ~SP_CacheStatisticsImpl()
{
}

int SP_CacheStatisticsImpl :: getHits() const
{
	return mHits;
}

int SP_CacheStatisticsImpl :: getAccesses() const
{
	return mAccesses;
}

int SP_CacheStatisticsImpl :: getSize() const
{
	return mSize;
}

void SP_CacheStatisticsImpl :: setSize( int size )
{
	mSize = size;
}

void SP_CacheStatisticsImpl :: markHit()
{
	mHits++;
	mAccesses++;
}

void SP_CacheStatisticsImpl :: markMiss()
{
	mAccesses++;
}

//===========================================================================

class SP_CacheEntry {
public:
	SP_CacheEntry( const void * item );
	~SP_CacheEntry();

	void setPrev( SP_CacheEntry * prev );
	SP_CacheEntry * getPrev();

	void setNext( SP_CacheEntry * next );
	SP_CacheEntry * getNext();

	void setItem( const void * item );
	const void * getItem();

	void setExpTime( time_t expTime );
	time_t getExpTime();

private:
	SP_CacheEntry * mPrev, * mNext;
	const void * mItem;
	time_t mExpTime;
};

SP_CacheEntry :: SP_CacheEntry( const void * item )
{
	mItem = item;
	mPrev = mNext = NULL;
	mExpTime = 0;
}

SP_CacheEntry :: ~SP_CacheEntry()
{
	mItem = NULL;
	mPrev = mNext = NULL;
}

void SP_CacheEntry :: setPrev( SP_CacheEntry * prev )
{
	mPrev = prev;
}

SP_CacheEntry * SP_CacheEntry :: getPrev()
{
	return mPrev;
}

void SP_CacheEntry :: setNext( SP_CacheEntry * next )
{
	mNext = next;
}

SP_CacheEntry * SP_CacheEntry :: getNext()
{
	return mNext;
}

void SP_CacheEntry :: setItem( const void * item )
{
	mItem = item;
}

const void * SP_CacheEntry :: getItem()
{
	return mItem;
}

void SP_CacheEntry :: setExpTime( time_t expTime )
{
	mExpTime = expTime;
}

time_t SP_CacheEntry :: getExpTime()
{
	return mExpTime;
}

class SP_CacheEntryList {
public:
	SP_CacheEntryList();
	~SP_CacheEntryList();

	SP_CacheEntry * getHead();

	void append( SP_CacheEntry * entry );

	void remove( SP_CacheEntry * entry );

private:
	SP_CacheEntry * mHead, * mTail;
};

SP_CacheEntryList :: SP_CacheEntryList()
{
	mHead = mTail = NULL;
}

SP_CacheEntryList :: ~SP_CacheEntryList()
{
}

SP_CacheEntry * SP_CacheEntryList :: getHead()
{
	return mHead;
}

void SP_CacheEntryList :: append( SP_CacheEntry * entry )
{
	if( NULL == mTail ) {
		mHead = mTail = entry;
	} else {
		mTail->setNext( entry );
		entry->setPrev( mTail );

		mTail = entry;
	}
}

void SP_CacheEntryList :: remove( SP_CacheEntry * entry )
{
	SP_CacheEntry * prev = entry->getPrev(), * next = entry->getNext();

	if( NULL == prev ) {
		mHead = next;
	} else {
		prev->setNext( next );
	}

	if( NULL == next ) {
		mTail = prev;
	} else {
		next->setPrev( prev );
	}
}

//===========================================================================

class SP_CacheHandlerAdapter : public SP_DictHandler {
public:
	SP_CacheHandlerAdapter( SP_CacheHandler * handler );
	~SP_CacheHandlerAdapter();

	virtual int compare( const void * item1, const void * item2 ) const;
	virtual void destroy( void * item ) const;

private:
	SP_CacheHandler * mHandler;
};

SP_CacheHandlerAdapter :: SP_CacheHandlerAdapter( SP_CacheHandler * handler )
{
	mHandler = handler;
}

SP_CacheHandlerAdapter :: ~SP_CacheHandlerAdapter()
{
}

int SP_CacheHandlerAdapter :: compare( const void * item1, const void * item2 ) const
{
	SP_CacheEntry * entry1 = ( SP_CacheEntry * ) item1;
	SP_CacheEntry * entry2 = ( SP_CacheEntry * ) item2;

	return mHandler->compare( entry1->getItem(), entry2->getItem() );
}

void SP_CacheHandlerAdapter :: destroy( void * item ) const
{
	SP_CacheEntry * entry = ( SP_CacheEntry * ) item;
	mHandler->destroy( (void*)entry->getItem() );
	delete entry;
}

//===========================================================================


class SP_CacheImpl : public SP_Cache {
public:
	SP_CacheImpl( int algo, int maxItems, SP_CacheHandler * handler );
	virtual ~SP_CacheImpl();

	virtual int put( void * item, time_t expTime = 0 );
	virtual int get( const void * key, void * resultHolder );
	virtual int erase( const void * key );
	virtual void * remove( const void * key, time_t * expTime = 0 );
	virtual SP_CacheStatistics * getStatistics();

private:
	SP_CacheHandler * mHandler;
	int mMaxItems;
	int mAlgo;

	SP_Dictionary * mDict;
	SP_CacheEntryList * mList;
	SP_CacheStatisticsImpl * mStatistics;
};

SP_CacheImpl :: SP_CacheImpl( int algo, int maxItems,
		SP_CacheHandler * handler )
{
	mAlgo = algo;
	mMaxItems = maxItems;
	mHandler = handler;

	mDict = SP_Dictionary::newInstance( SP_Dictionary::eBSTree,
			new SP_CacheHandlerAdapter( handler ) );
	mList = new SP_CacheEntryList();

	mStatistics = new SP_CacheStatisticsImpl();
}

SP_CacheImpl :: ~SP_CacheImpl()
{
	delete mStatistics;
	delete mList;
	delete mDict;
	delete mHandler;
}

int SP_CacheImpl :: put( void * item, time_t expTime )
{
	int result = 0;

	SP_CacheEntry * entry = new SP_CacheEntry( item );
	entry->setExpTime( expTime );

	SP_CacheEntry * oldEntry = (SP_CacheEntry*)mDict->search( entry );
	if( NULL != oldEntry ) {
		result = 1;

		mList->remove( oldEntry );
		mDict->remove( oldEntry );

		mHandler->destroy( (void*)oldEntry->getItem() );
		delete oldEntry;
	}

	mDict->insert( entry );
	mList->append( entry );

	for( ; mDict->getCount() > mMaxItems && mMaxItems > 0; ) {
		SP_CacheEntry * head = mList->getHead();

		mList->remove( head );
		mDict->remove( head );

		mHandler->destroy( (void*)head->getItem() );
		delete head;
	}

	return result;
}

int SP_CacheImpl :: get( const void * key, void * resultHolder )
{
	int result = 0;

	SP_CacheEntry keyEntry( key );
	SP_CacheEntry * entry = (SP_CacheEntry*)mDict->search( &keyEntry );

	if( NULL != entry ) {
		if( entry->getExpTime() > 0 && entry->getExpTime() < time( NULL ) ) {
			//erase( key );
		} else {
			result = 1;
			mHandler->onHit( entry->getItem(), resultHolder );

			if( eLRU == mAlgo ) {
				mList->remove( entry );
				mList->append( entry );
			}

			mStatistics->markHit();
		}
	} else {
		mStatistics->markMiss();
	}

	return result;
}

int SP_CacheImpl :: erase( const void * key )
{
	int result = 0;

	void * item = remove( key );

	if( NULL != item ) {
		result = 1;
		mHandler->destroy( item );
	}

	return result;
}

void * SP_CacheImpl :: remove( const void * key, time_t * expTime )
{
	void * result = NULL;

	SP_CacheEntry keyEntry( key );
	SP_CacheEntry * entry = (SP_CacheEntry*)mDict->remove( &keyEntry );

	if( NULL != entry ) {
		mList->remove( entry );

		if( NULL != expTime ) *expTime = entry->getExpTime();
		result = (void*)entry->getItem();
		delete entry;
	}

	return result;
}

SP_CacheStatistics * SP_CacheImpl :: getStatistics()
{
	SP_CacheStatisticsImpl * ret = new SP_CacheStatisticsImpl( *mStatistics );
	ret->setSize( mDict->getCount() );

	return ret;
}

//===========================================================================

class SP_ThreadSafeCacheWrapper : public SP_Cache {
public:
	SP_ThreadSafeCacheWrapper( SP_Cache * cache );
	virtual ~SP_ThreadSafeCacheWrapper();

	virtual int put( void * item, time_t expTime = 0 );
	virtual int get( const void * key, void * resultHolder );
	virtual int erase( const void * key );
	virtual void * remove( const void * key, time_t * expTime );
	virtual SP_CacheStatistics * getStatistics();

private:

	enum { eRead, eWrite };
	void lock( int lockType );
	void unlock();

	SP_Cache * mCache;

#ifndef WIN32
	pthread_rwlock_t mLock;
#else
	HANDLE mMutex;
#endif

};

SP_ThreadSafeCacheWrapper :: SP_ThreadSafeCacheWrapper( SP_Cache * cache )
{
	mCache = cache;

#ifndef WIN32
	pthread_rwlock_init( &mLock, NULL );
#else
	mMutex = CreateMutex(0, FALSE, 0);
#endif
}

SP_ThreadSafeCacheWrapper :: ~SP_ThreadSafeCacheWrapper()
{
	delete mCache;

#ifndef WIN32
	pthread_rwlock_destroy( &mLock );
#else
	CloseHandle( mMutex );
#endif
}

void SP_ThreadSafeCacheWrapper :: lock( int lockType )
{
#ifndef WIN32
	if( eRead == lockType ) {
		assert( 0 == pthread_rwlock_rdlock( &mLock ) );
	} else {
		pthread_rwlock_wrlock( &mLock );
	}
#else
	WaitForSingleObject( mMutex, INFINITE );
#endif
}

void SP_ThreadSafeCacheWrapper :: unlock()
{
#ifndef WIN32
	pthread_rwlock_unlock( &mLock );
#else
	ReleaseMutex( mMutex );
#endif
}

int SP_ThreadSafeCacheWrapper :: put( void * item, time_t expTime )
{
	lock( eWrite );

	int ret = mCache->put( item, expTime );

	unlock();

	return ret;
}

int SP_ThreadSafeCacheWrapper :: get( const void * key, void * resultHolder )
{
	lock( eRead );

	int ret = mCache->get( key, resultHolder );

	unlock();

	return ret;
}

int SP_ThreadSafeCacheWrapper :: erase( const void * key )
{
	lock( eWrite );

	int ret = mCache->erase( key );

	unlock();

	return ret;
}

void * SP_ThreadSafeCacheWrapper :: remove( const void * key, time_t * expTime )
{
	lock( eWrite );

	void * item = mCache->remove( key, expTime );

	unlock();

	return item;
}

SP_CacheStatistics * SP_ThreadSafeCacheWrapper :: getStatistics()
{
	lock( eWrite );

	SP_CacheStatistics * stat = mCache->getStatistics();

	unlock();

	return stat;
}

//===========================================================================

SP_Cache :: ~SP_Cache()
{
}

SP_Cache * SP_Cache :: newInstance( int algo,
		int maxItems, SP_CacheHandler * handler, int threadSafe )
{
	SP_Cache * cache = new SP_CacheImpl( algo, maxItems, handler );
	if( threadSafe ) cache = new SP_ThreadSafeCacheWrapper( cache );

	return cache;
}


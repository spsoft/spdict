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

#include "spdictcache.hpp"
#include "spdictionary.hpp"

//===========================================================================

SP_DictCacheHandler :: ~SP_DictCacheHandler()
{
}

//===========================================================================

SP_DictCacheStatistics :: ~SP_DictCacheStatistics()
{
}

class SP_DictCacheStatisticsImpl : public SP_DictCacheStatistics {
public:
	SP_DictCacheStatisticsImpl();
	SP_DictCacheStatisticsImpl( SP_DictCacheStatisticsImpl & other );
	virtual ~SP_DictCacheStatisticsImpl();

	virtual int getHits() const;
	virtual int getAccesses() const;
	virtual int getSize() const;

	void setSize( int size );
	void markHit();
	void markMiss();

private:
	int mHits, mAccesses, mSize;
};

SP_DictCacheStatisticsImpl :: SP_DictCacheStatisticsImpl()
{
	mHits = mAccesses = mSize = 0;
}

SP_DictCacheStatisticsImpl :: SP_DictCacheStatisticsImpl( SP_DictCacheStatisticsImpl & other )
{
	mHits = other.getHits();
	mAccesses = other.getAccesses();
	mSize = other.getSize();
}

SP_DictCacheStatisticsImpl :: ~SP_DictCacheStatisticsImpl()
{
}

int SP_DictCacheStatisticsImpl :: getHits() const
{
	return mHits;
}

int SP_DictCacheStatisticsImpl :: getAccesses() const
{
	return mAccesses;
}

int SP_DictCacheStatisticsImpl :: getSize() const
{
	return mSize;
}

void SP_DictCacheStatisticsImpl :: setSize( int size )
{
	mSize = size;
}

void SP_DictCacheStatisticsImpl :: markHit()
{
	mHits++;
	mAccesses++;
}

void SP_DictCacheStatisticsImpl :: markMiss()
{
	mAccesses++;
}

//===========================================================================

class SP_DictCacheEntry {
public:
	SP_DictCacheEntry( const void * item );
	~SP_DictCacheEntry();

	void setPrev( SP_DictCacheEntry * prev );
	SP_DictCacheEntry * getPrev();

	void setNext( SP_DictCacheEntry * next );
	SP_DictCacheEntry * getNext();

	void setItem( const void * item );
	const void * getItem();

	void setExpTime( time_t expTime );
	time_t getExpTime();

private:
	SP_DictCacheEntry * mPrev, * mNext;
	const void * mItem;
	time_t mExpTime;
};

SP_DictCacheEntry :: SP_DictCacheEntry( const void * item )
{
	mItem = item;
	mPrev = mNext = NULL;
	mExpTime = 0;
}

SP_DictCacheEntry :: ~SP_DictCacheEntry()
{
	mItem = NULL;
	mPrev = mNext = NULL;
}

void SP_DictCacheEntry :: setPrev( SP_DictCacheEntry * prev )
{
	mPrev = prev;
}

SP_DictCacheEntry * SP_DictCacheEntry :: getPrev()
{
	return mPrev;
}

void SP_DictCacheEntry :: setNext( SP_DictCacheEntry * next )
{
	mNext = next;
}

SP_DictCacheEntry * SP_DictCacheEntry :: getNext()
{
	return mNext;
}

void SP_DictCacheEntry :: setItem( const void * item )
{
	mItem = item;
}

const void * SP_DictCacheEntry :: getItem()
{
	return mItem;
}

void SP_DictCacheEntry :: setExpTime( time_t expTime )
{
	mExpTime = expTime;
}

time_t SP_DictCacheEntry :: getExpTime()
{
	return mExpTime;
}

class SP_DictCacheEntryList {
public:
	SP_DictCacheEntryList();
	~SP_DictCacheEntryList();

	SP_DictCacheEntry * getHead();

	void append( SP_DictCacheEntry * entry );

	void remove( SP_DictCacheEntry * entry );

private:
	SP_DictCacheEntry * mHead, * mTail;
};

SP_DictCacheEntryList :: SP_DictCacheEntryList()
{
	mHead = mTail = NULL;
}

SP_DictCacheEntryList :: ~SP_DictCacheEntryList()
{
}

SP_DictCacheEntry * SP_DictCacheEntryList :: getHead()
{
	return mHead;
}

void SP_DictCacheEntryList :: append( SP_DictCacheEntry * entry )
{
	entry->setPrev( NULL );
	entry->setNext( NULL );

	if( NULL == mTail ) {
		mHead = mTail = entry;
	} else {
		mTail->setNext( entry );
		entry->setPrev( mTail );

		mTail = entry;
	}
}

void SP_DictCacheEntryList :: remove( SP_DictCacheEntry * entry )
{
	SP_DictCacheEntry * prev = entry->getPrev(), * next = entry->getNext();

	if( mHead == entry ) assert( NULL == prev );
	if( mTail == entry ) assert( NULL == next );

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

	entry->setPrev( NULL );
	entry->setNext( NULL );
}

//===========================================================================

class SP_DictCacheHandlerAdapter : public SP_DictHandler {
public:
	SP_DictCacheHandlerAdapter( SP_DictCacheHandler * handler );
	~SP_DictCacheHandlerAdapter();

	virtual int compare( const void * item1, const void * item2 ) const;
	virtual void destroy( void * item ) const;

private:
	SP_DictCacheHandler * mHandler;
};

SP_DictCacheHandlerAdapter :: SP_DictCacheHandlerAdapter( SP_DictCacheHandler * handler )
{
	mHandler = handler;
}

SP_DictCacheHandlerAdapter :: ~SP_DictCacheHandlerAdapter()
{
}

int SP_DictCacheHandlerAdapter :: compare( const void * item1, const void * item2 ) const
{
	SP_DictCacheEntry * entry1 = ( SP_DictCacheEntry * ) item1;
	SP_DictCacheEntry * entry2 = ( SP_DictCacheEntry * ) item2;

	return mHandler->compare( entry1->getItem(), entry2->getItem() );
}

void SP_DictCacheHandlerAdapter :: destroy( void * item ) const
{
	SP_DictCacheEntry * entry = ( SP_DictCacheEntry * ) item;
	mHandler->destroy( (void*)entry->getItem() );
	delete entry;
}

//===========================================================================


class SP_DictCacheImpl : public SP_DictCache {
public:
	SP_DictCacheImpl( int algo, int maxItems, SP_DictCacheHandler * handler );
	virtual ~SP_DictCacheImpl();

	virtual int put( void * item, time_t expTime = 0 );
	virtual int get( const void * key, void * resultHolder );
	virtual int erase( const void * key );
	virtual void * remove( const void * key, time_t * expTime = 0 );
	virtual SP_DictCacheStatistics * getStatistics();

private:
	SP_DictCacheHandler * mHandler;
	int mMaxItems;
	int mAlgo;

	SP_Dictionary * mDict;
	SP_DictCacheEntryList * mList;
	SP_DictCacheStatisticsImpl * mStatistics;
};

SP_DictCacheImpl :: SP_DictCacheImpl( int algo, int maxItems,
		SP_DictCacheHandler * handler )
{
	mAlgo = algo;
	mMaxItems = maxItems;
	mHandler = handler;

	mDict = SP_Dictionary::newInstance( SP_Dictionary::eBTree,
			new SP_DictCacheHandlerAdapter( handler ) );
	mList = new SP_DictCacheEntryList();

	mStatistics = new SP_DictCacheStatisticsImpl();
}

SP_DictCacheImpl :: ~SP_DictCacheImpl()
{
	delete mStatistics;
	delete mList;
	delete mDict;
	delete mHandler;
}

int SP_DictCacheImpl :: put( void * item, time_t expTime )
{
	int result = 0;

	SP_DictCacheEntry * entry = new SP_DictCacheEntry( item );
	entry->setExpTime( expTime );

	SP_DictCacheEntry * oldEntry = (SP_DictCacheEntry*)mDict->search( entry );
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
		SP_DictCacheEntry * head = mList->getHead();

		mList->remove( head );
		mDict->remove( head );

		mHandler->destroy( (void*)head->getItem() );
		delete head;
	}

	return result;
}

int SP_DictCacheImpl :: get( const void * key, void * resultHolder )
{
	int result = 0;

	SP_DictCacheEntry keyEntry( key );
	SP_DictCacheEntry * entry = (SP_DictCacheEntry*)mDict->search( &keyEntry );

	if( NULL != entry ) {
		if( entry->getExpTime() > 0 && entry->getExpTime() < time( NULL ) ) {
			erase( key );
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

int SP_DictCacheImpl :: erase( const void * key )
{
	int result = 0;

	void * item = remove( key );

	if( NULL != item ) {
		result = 1;
		mHandler->destroy( item );
	}

	return result;
}

void * SP_DictCacheImpl :: remove( const void * key, time_t * expTime )
{
	void * result = NULL;

	SP_DictCacheEntry keyEntry( key );
	SP_DictCacheEntry * entry = (SP_DictCacheEntry*)mDict->remove( &keyEntry );

	if( NULL != entry ) {
		mList->remove( entry );

		if( NULL != expTime ) *expTime = entry->getExpTime();
		result = (void*)entry->getItem();
		delete entry;
	}

	return result;
}

SP_DictCacheStatistics * SP_DictCacheImpl :: getStatistics()
{
	SP_DictCacheStatisticsImpl * ret = new SP_DictCacheStatisticsImpl( *mStatistics );
	ret->setSize( mDict->getCount() );

	return ret;
}

//===========================================================================

class SP_ThreadSafeCacheWrapper : public SP_DictCache {
public:
	SP_ThreadSafeCacheWrapper( SP_DictCache * cache );
	virtual ~SP_ThreadSafeCacheWrapper();

	virtual int put( void * item, time_t expTime = 0 );
	virtual int get( const void * key, void * resultHolder );
	virtual int erase( const void * key );
	virtual void * remove( const void * key, time_t * expTime );
	virtual SP_DictCacheStatistics * getStatistics();

private:

	void lock();
	void unlock();

	SP_DictCache * mCache;

#ifndef WIN32
	pthread_mutex_t mMutex;
#else
	HANDLE mMutex;
#endif

};

SP_ThreadSafeCacheWrapper :: SP_ThreadSafeCacheWrapper( SP_DictCache * cache )
{
	mCache = cache;

#ifndef WIN32
	pthread_mutex_init( &mMutex, NULL );
#else
	mMutex = CreateMutex(0, FALSE, 0);
#endif
}

SP_ThreadSafeCacheWrapper :: ~SP_ThreadSafeCacheWrapper()
{
	delete mCache;

#ifndef WIN32
	pthread_mutex_destroy( &mMutex );
#else
	CloseHandle( mMutex );
#endif
}

void SP_ThreadSafeCacheWrapper :: lock()
{
#ifndef WIN32
	pthread_mutex_lock( &mMutex );
#else
	WaitForSingleObject( mMutex, INFINITE );
#endif
}

void SP_ThreadSafeCacheWrapper :: unlock()
{
#ifndef WIN32
	pthread_mutex_unlock( &mMutex );
#else
	ReleaseMutex( mMutex );
#endif
}

int SP_ThreadSafeCacheWrapper :: put( void * item, time_t expTime )
{
	lock();

	int ret = mCache->put( item, expTime );

	unlock();

	return ret;
}

int SP_ThreadSafeCacheWrapper :: get( const void * key, void * resultHolder )
{
	lock();

	int ret = mCache->get( key, resultHolder );

	unlock();

	return ret;
}

int SP_ThreadSafeCacheWrapper :: erase( const void * key )
{
	lock();

	int ret = mCache->erase( key );

	unlock();

	return ret;
}

void * SP_ThreadSafeCacheWrapper :: remove( const void * key, time_t * expTime )
{
	lock();

	void * item = mCache->remove( key, expTime );

	unlock();

	return item;
}

SP_DictCacheStatistics * SP_ThreadSafeCacheWrapper :: getStatistics()
{
	lock();

	SP_DictCacheStatistics * stat = mCache->getStatistics();

	unlock();

	return stat;
}

//===========================================================================

SP_DictCache :: ~SP_DictCache()
{
}

SP_DictCache * SP_DictCache :: newInstance( int algo,
		int maxItems, SP_DictCacheHandler * handler, int threadSafe )
{
	SP_DictCache * cache = new SP_DictCacheImpl( algo, maxItems, handler );
	if( threadSafe ) cache = new SP_ThreadSafeCacheWrapper( cache );

	return cache;
}


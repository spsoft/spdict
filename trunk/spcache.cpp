/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdlib.h>

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

private:
	SP_CacheEntry * mPrev, * mNext;
	const void * mItem;
};

SP_CacheEntry :: SP_CacheEntry( const void * item )
{
	mItem = item;
	mPrev = mNext = NULL;
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

	virtual int put( void * itemm );
	virtual int get( const void * key, void * resultHolder );
	virtual int remove( const void * key );
	virtual SP_CacheStatistics * getStatistics();

private:
	SP_CacheHandler * mHandler;
	int mMaxItems;
	int mAlgo;

	SP_Dictionary * mDict;
	SP_CacheEntryList * mList;
	SP_CacheStatisticsImpl * mStatistics;

#ifndef WIN32
	pthread_mutex_t mMutex;
#else
	HANDLE mMutex;
#endif
};

SP_CacheImpl :: SP_CacheImpl( int algo, int maxItems,
		SP_CacheHandler * handler )
{
	mAlgo = algo;
	mMaxItems = maxItems;
	mHandler = handler;

	mDict = SP_Dictionary::newInstance( SP_Dictionary::eBTree,
			new SP_CacheHandlerAdapter( handler ) );
	mList = new SP_CacheEntryList();

	mStatistics = new SP_CacheStatisticsImpl();

#ifndef WIN32
	pthread_mutex_init( &mMutex, NULL );
#else
	mMutex = CreateMutex(0, FALSE, 0);
#endif
}

SP_CacheImpl :: ~SP_CacheImpl()
{
	delete mStatistics;
	delete mList;
	delete mDict;
	delete mHandler;

#ifndef WIN32
	pthread_mutex_destroy( &mMutex );
#else
	CloseHandle( mMutex );
#endif
}

int SP_CacheImpl :: put( void * item )
{
	int result = 0;

#ifndef WIN32
	pthread_mutex_lock( &mMutex );
#else
	WaitForSingleObject( mMutex, INFINITE );
#endif

	SP_CacheEntry * entry = new SP_CacheEntry( item );

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

	for( ; mDict->getCount() > mMaxItems; ) {
		SP_CacheEntry * head = mList->getHead();

		mList->remove( head );
		mDict->remove( head );

		mHandler->destroy( (void*)head->getItem() );
		delete head;
	}

#ifndef WIN32
	pthread_mutex_unlock( &mMutex );
#else
	ReleaseMutex( mMutex );
#endif

	return result;
}

int SP_CacheImpl :: get( const void * key, void * resultHolder )
{
	int result = 0;

#ifndef WIN32
	pthread_mutex_lock( &mMutex );
#else
	WaitForSingleObject( mMutex, INFINITE );
#endif

	SP_CacheEntry keyEntry( key );
	SP_CacheEntry * entry = (SP_CacheEntry*)mDict->search( &keyEntry );

	if( NULL != entry ) {
		result = 1;
		mHandler->onHit( entry->getItem(), resultHolder );

		if( eLRU == mAlgo ) {
			mList->remove( entry );
			mList->append( entry );
		}

		mStatistics->markHit();
	} else {
		mStatistics->markMiss();
	}

#ifndef WIN32
	pthread_mutex_unlock( &mMutex );
#else
	ReleaseMutex( mMutex );
#endif

	return result;
}

int SP_CacheImpl :: remove( const void * key )
{
	int result = 0;

#ifndef WIN32
	pthread_mutex_lock( &mMutex );
#else
	WaitForSingleObject( mMutex, INFINITE );
#endif

	SP_CacheEntry keyEntry( key );
	SP_CacheEntry * entry = (SP_CacheEntry*)mDict->remove( &keyEntry );

	if( NULL != entry ) {
		result = 1;
		mList->remove( entry );

		mHandler->destroy( (void*)entry->getItem() );
		delete entry;
	}

#ifndef WIN32
	pthread_mutex_unlock( &mMutex );
#else
	ReleaseMutex( mMutex );
#endif

	return result;
}

SP_CacheStatistics * SP_CacheImpl :: getStatistics()
{
#ifndef WIN32
	pthread_mutex_lock( &mMutex );
#else
	WaitForSingleObject( mMutex, INFINITE );
#endif

	SP_CacheStatisticsImpl * ret = new SP_CacheStatisticsImpl( *mStatistics );
	ret->setSize( mDict->getCount() );

#ifndef WIN32
	pthread_mutex_unlock( &mMutex );
#else
	ReleaseMutex( mMutex );
#endif

	return ret;
}

//===========================================================================

SP_Cache :: ~SP_Cache()
{
}

SP_Cache * SP_Cache :: newInstance( int algo,
		int maxItems, SP_CacheHandler * handler )
{
	return new SP_CacheImpl( algo, maxItems, handler );
}


/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spdictshmqueue_hpp__
#define __spdictshmqueue_hpp__

#include <time.h>

class SP_DictCircleQueue
{
public:
	typedef struct tagHeader
	{
		char mType0;
		char mType1;

		int mLen, mItemSize;
		int mMaxCount, mCount;
		int mHead, mTail;

		char mData[1];
	} Header_t;

public:
	SP_DictCircleQueue( Header_t * header );
	~SP_DictCircleQueue();

	int push( void * item );

	int pop( void * holder );

	int getCount();

private:
	Header_t * mHeader;
};

class CSem;
class CFileLock;

class SP_DictShmQueue
{
public:
	SP_DictShmQueue();
	~SP_DictShmQueue();

	// @return 0 : OK, -1 : Fail
	int init( const char * path, int maxCount, int itemSize );

	int getCount();

	// @return 0 : OK, -1 : Fail
	int push( void * item );

	// @return 0 : OK, -1 : Fail
	int pop( void * item );

	static void * getMmapPtr( const char * path, size_t len );

	static void freeMmapPtr( void * ptr, size_t len );

private:
	SP_DictCircleQueue::Header_t * mHeader;
	int mLen;

	SP_DictCircleQueue * mQueue;
	int mSemID;
};

#endif


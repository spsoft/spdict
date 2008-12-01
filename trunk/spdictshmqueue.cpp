/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <sys/sem.h>
#include <fcntl.h>

#include "spdictshmqueue.hpp"
#include "spdictshmalloc.hpp"

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                    /* value for SETVAL */
	struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
	unsigned short int *array;  /* array for GETALL, SETALL */
	struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif

static struct sembuf op_lock   =     { 0, -1, 0 };
static struct sembuf op_unlock =     { 0,  1, 0 };
static struct sembuf op_pop_wait  =  { 1, -1, 0 };
static struct sembuf op_pop_post  =  { 1,  1, 0 };
static struct sembuf op_push_wait =  { 2, -1, 0 };
static struct sembuf op_push_post =  { 2,  1, 0 };

SP_DictCircleQueue :: SP_DictCircleQueue( Header_t * header )
{
	mHeader = header;

	assert( mHeader->mMaxCount == (int)( ( mHeader->mLen - sizeof( Header_t ) ) / mHeader->mItemSize ) );

	assert( mHeader->mHead >= 0 && mHeader->mHead < mHeader->mMaxCount );
	assert( mHeader->mTail >= 0 && mHeader->mTail < mHeader->mMaxCount );

	int count = 0;

	if( mHeader->mHead == mHeader->mTail ) {
		count = mHeader->mCount > 0 ? mHeader->mMaxCount : 0;
	} else if( mHeader->mHead > mHeader->mTail ) {
		count = mHeader->mHead - mHeader->mTail;
	} else {
		count = mHeader->mMaxCount + mHeader->mHead - mHeader->mTail;
	}

	if( mHeader->mCount != count ) {
		printf( "ERR: invalid header, count %d, %d, head %d, tail %d\n",
				mHeader->mCount, count, mHeader->mHead, mHeader->mTail );
		mHeader->mCount = count;
	}
}

SP_DictCircleQueue :: ~SP_DictCircleQueue()
{
	mHeader = NULL;
}

int SP_DictCircleQueue :: push( void * item )
{
	if( mHeader->mCount >= mHeader->mMaxCount ) return -1;

	char * dest = mHeader->mData + ( mHeader->mItemSize * mHeader->mHead );

	memcpy( dest, item, mHeader->mItemSize );

	++( mHeader->mCount );
	++( mHeader->mHead );

	if( mHeader->mHead >= mHeader->mMaxCount ) mHeader->mHead = 0;

	return 0;
}

int SP_DictCircleQueue :: pop( void * item )
{
	if( mHeader->mCount <= 0 ) return -1;

	char * dest = mHeader->mData + ( mHeader->mItemSize * mHeader->mTail );

	memcpy( item, dest, mHeader->mItemSize );

	--( mHeader->mCount );
	++( mHeader->mTail );

	if( mHeader->mTail >= mHeader->mMaxCount ) mHeader->mTail = 0;

	return 0;
}

int SP_DictCircleQueue :: getCount()
{
	return mHeader->mCount;
}

//===================================================================

SP_DictShmQueue :: SP_DictShmQueue()
{
	mHeader = NULL;
	mLen = 0;

	mQueue = NULL;
	mSemID = -1;
}

SP_DictShmQueue :: ~SP_DictShmQueue()
{
	if( NULL != mHeader ) SP_DictShmAllocator::freeMmapPtr( mHeader, mLen );
	mHeader = NULL;

	if( NULL != mQueue ) delete mQueue;
	mQueue = NULL;
}

int SP_DictShmQueue :: init( const char * path, int maxCount, int itemSize )
{
	mLen = sizeof(SP_DictCircleQueue::Header_t) + maxCount * itemSize;

	int isNew = 0;

	mHeader = (SP_DictCircleQueue::Header_t*)SP_DictShmAllocator::getMmapPtr( path, mLen, &isNew );

	if( NULL != mHeader ) {
		if( 0 == mHeader->mType0 && 0 == mHeader->mType1 ) {
			mHeader->mType0 = 'P';
			mHeader->mType1 = 'Q';

			mHeader->mMaxCount = maxCount;
			mHeader->mCount = 0;

			mHeader->mItemSize = itemSize;
			mHeader->mLen = mLen;
		} else {
			if( 'P' == mHeader->mType0 && 'Q' == mHeader->mType1
					&& maxCount == mHeader->mMaxCount
					&& itemSize == mHeader->mItemSize
					&& mLen == mHeader->mLen ) {
				printf( "INIT: %s verify ok, [%c%c] m %d i %d l %d\n",
						path, mHeader->mType0, mHeader->mType1, mHeader->mMaxCount,
						mHeader->mItemSize, mHeader->mLen );
			} else {
				printf( "INIT: %s verify fail, [%c%c] m %d:%d i %d:%d l %d:%d\n",
						path, mHeader->mType0, mHeader->mType1, mHeader->mMaxCount, maxCount,
						mHeader->mItemSize, itemSize, mHeader->mLen, mLen );

				SP_DictShmAllocator::freeMmapPtr( mHeader, mLen );
				mHeader = NULL;
			}
		}
	}

	if( NULL != mHeader ) {
		printf( "INIT: count %d, head %d, tail %d\n",
				mHeader->mCount, mHeader->mHead, mHeader->mTail );

		mQueue = new SP_DictCircleQueue( mHeader );

		key_t semKey = ftok( path, 0 );
		printf( "INIT: sem key %x\n", semKey );

		// sem0 -- for lock, sem1 -- for pop, sem2 -- for push
		mSemID = semget( semKey, 3, IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR );

		int val0 = semctl( mSemID, 0, GETVAL, 0 );
		int val1 = semctl( mSemID, 1, GETVAL, 0 );
		int val2 = semctl( mSemID, 2, GETVAL, 0 );

		printf( "INIT: before val0 %d, val1 %d val2 %d\n", val0, val1, val2 );

		union semun arg;

		arg.val = 1;
		semctl( mSemID, 0, SETVAL, arg );

		arg.val = mHeader->mCount;
		semctl( mSemID, 1, SETVAL, arg );

		arg.val = mHeader->mMaxCount - mHeader->mCount;
		semctl( mSemID, 2, SETVAL, arg );

		val0 = semctl( mSemID, 0, GETVAL, 0 );
		val1 = semctl( mSemID, 1, GETVAL, 0 );
		val2 = semctl( mSemID, 2, GETVAL, 0 );

		printf( "INIT: after val0 %d, val1 %d val2 %d\n", val0, val1, val2 );
	}

	return NULL != mHeader ? 0 : -1;
}

int SP_DictShmQueue :: getCount()
{
	int ret = 0;

	semop( mSemID, &op_lock, 1 );

	ret = mQueue->getCount();

	semop( mSemID, &op_unlock, 1 );

	return ret;
}

int SP_DictShmQueue :: push( void * item )
{
	int ret = 0;

	// wait for push space
	semop( mSemID, &op_push_wait, 1 );

	{
		semop( mSemID, &op_lock, 1 );
		ret = mQueue->push( item );
		semop( mSemID, &op_unlock, 1 );
	}

	if( 0 == ret )
	{
		// signal for available item
		semop( mSemID, &op_pop_post, 1 );
	}

	return ret;
}

int SP_DictShmQueue :: pop( void * item )
{
	int ret = 0;

	memset( item, 0, mHeader->mItemSize );

	// wait for available item
	semop( mSemID, &op_pop_wait, 1 );

	{
		semop( mSemID, &op_lock, 1 );
		ret = mQueue->pop( item );
		semop( mSemID, &op_unlock, 1 );
	}

	if( 0 == ret )
	{
		// signal for push space
		semop( mSemID, &op_push_post, 1 );
	}

	return ret;
}


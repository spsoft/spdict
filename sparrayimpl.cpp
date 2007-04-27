/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>

#include "sparrayimpl.hpp"

//===========================================================================

SP_SortedArrayNode :: SP_SortedArrayNode( void * item )
{
	mItem = item;
}

SP_SortedArrayNode :: ~SP_SortedArrayNode()
{
}

void SP_SortedArrayNode :: setItem( void * item )
{
	mItem = item;
}

void * SP_SortedArrayNode :: getItem() const
{
	return mItem;
}

void * SP_SortedArrayNode :: takeItem()
{
	void * ret = mItem;
	mItem = NULL;

	return ret;
}

//===========================================================================

SP_SortedArrayIterator :: SP_SortedArrayIterator( SP_SortedArrayNode ** list, int count )
{
	mList = list;
	mCount = count;
	mIndex = 0;
}

SP_SortedArrayIterator :: ~SP_SortedArrayIterator()
{
}

const void * SP_SortedArrayIterator :: getNext( int * level )
{
	if( mIndex < mCount ) return mList[ mIndex++ ]->getItem();

	return NULL;
}

//===========================================================================

SP_SortedArrayImpl :: SP_SortedArrayImpl( SP_DictHandler * handler )
{
	mHandler = handler;

	mMaxCount = 128;
	mCount = 0;

	mList = (SP_SortedArrayNode**)malloc( mMaxCount * sizeof( void * ) );
	memset( mList, 0, mMaxCount * sizeof( void * ) );
}

SP_SortedArrayImpl :: ~SP_SortedArrayImpl()
{
	for( int i = 0; i < mCount; i++ ) {
		mHandler->destroy( mList[i]->getItem() );
		delete mList[i];
	}

	free( mList );
	delete mHandler;
}

int SP_SortedArrayImpl :: binarySearch( const void * item, int * insertPoint,
		int firstIndex, int size ) const
{
	// if aiSize not specify, then search the hold list
	if( size == -1 )  size = mCount;
	// can't find the key
	if( size == 0 ) {
		// set the insert point
		if( insertPoint != NULL ) * insertPoint = firstIndex; 
		return -1;  // return "can't find"
	}

	int cmpRet = mHandler->compare( item,
			mList[ firstIndex + ( size - 1 ) / 2 ]->getItem() );
	if( cmpRet < 0 ) {
		return binarySearch( item, insertPoint, firstIndex, ( size - 1 ) / 2 );
	} else if( cmpRet > 0 ) {
		return binarySearch( item, insertPoint, firstIndex + ( ( size - 1 ) / 2 ) + 1,
				size - ( ( size - 1 ) / 2 ) - 1 );
	} else { // find it
		return( firstIndex + ( size - 1 ) / 2 );
	}
}

int SP_SortedArrayImpl :: insert( void * item )
{
	int insertPoint = -1;

	int index = binarySearch( item, &insertPoint );
	if( index >= 0 ) {
		mHandler->destroy( mList[ index ]->takeItem() );
		mList[ index ]->setItem( item );
	} else {
		if( mCount >= mMaxCount ) {
			mMaxCount = ( mMaxCount * 3 ) / 2 + 1;
			mList = (SP_SortedArrayNode**)realloc( mList, mMaxCount * sizeof( void * ) );
			memset( mList + mCount, 0, ( mMaxCount - mCount ) * sizeof( void * ) );
		}
		if( insertPoint < mCount ) {
			memmove( mList + insertPoint + 1, mList + insertPoint,
					( mCount - insertPoint ) * sizeof( void * ) );
		}

		mList[ insertPoint ] = new SP_SortedArrayNode( item );
		mCount++;
	}

	return index >= 0 ? 1 : 0;
}

const void * SP_SortedArrayImpl :: search( const void * key ) const
{
	const void * ret = NULL;

	int index = binarySearch( key );
	if( index >= 0 ) ret = mList[ index ]->getItem();

	return ret;
}

void * SP_SortedArrayImpl :: remove( const void * key )
{
	void * ret = NULL;

	int index = binarySearch( key );
	if( index >= 0 ) {
		SP_SortedArrayNode * node = mList[ index ];
		memmove( mList + index, mList + index + 1, ( mCount - index - 1 ) * sizeof( void * ) );

		ret = node->takeItem();
		delete node;

		mCount--;
	}

	return ret;
}

int SP_SortedArrayImpl :: getCount() const
{
	return mCount;
}

SP_DictIterator * SP_SortedArrayImpl :: getIterator() const
{
	return new SP_SortedArrayIterator( mList, mCount );
}


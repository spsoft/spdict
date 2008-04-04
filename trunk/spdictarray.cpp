/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>

#include "spdictarray.hpp"

//===========================================================================

SP_DictSortedArrayNode :: SP_DictSortedArrayNode( void * item )
{
	mItem = item;
}

SP_DictSortedArrayNode :: ~SP_DictSortedArrayNode()
{
}

void SP_DictSortedArrayNode :: setItem( void * item )
{
	mItem = item;
}

void * SP_DictSortedArrayNode :: getItem() const
{
	return mItem;
}

void * SP_DictSortedArrayNode :: takeItem()
{
	void * ret = mItem;
	mItem = NULL;

	return ret;
}

//===========================================================================

SP_DictSortedArrayIterator :: SP_DictSortedArrayIterator( SP_DictSortedArrayNode ** list, int count )
{
	mList = list;
	mCount = count;
	mIndex = 0;
}

SP_DictSortedArrayIterator :: ~SP_DictSortedArrayIterator()
{
}

const void * SP_DictSortedArrayIterator :: getNext( int * level )
{
	if( mIndex < mCount ) return mList[ mIndex++ ]->getItem();

	return NULL;
}

//===========================================================================

SP_DictSortedArray :: SP_DictSortedArray( SP_DictHandler * handler )
{
	mHandler = handler;

	mMaxCount = 128;
	mCount = 0;

	mList = (SP_DictSortedArrayNode**)malloc( mMaxCount * sizeof( void * ) );
	memset( mList, 0, mMaxCount * sizeof( void * ) );
}

SP_DictSortedArray :: ~SP_DictSortedArray()
{
	for( int i = 0; i < mCount; i++ ) {
		mHandler->destroy( mList[i]->getItem() );
		delete mList[i];
	}

	free( mList );
	delete mHandler;
}

int SP_DictSortedArray :: binarySearch( const void * item, int * insertPoint,
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

int SP_DictSortedArray :: insert( void * item )
{
	int insertPoint = -1;

	int index = binarySearch( item, &insertPoint );
	if( index >= 0 ) {
		mHandler->destroy( mList[ index ]->takeItem() );
		mList[ index ]->setItem( item );
	} else {
		if( mCount >= mMaxCount ) {
			mMaxCount = ( mMaxCount * 3 ) / 2 + 1;
			mList = (SP_DictSortedArrayNode**)realloc( mList, mMaxCount * sizeof( void * ) );
			memset( mList + mCount, 0, ( mMaxCount - mCount ) * sizeof( void * ) );
		}
		if( insertPoint < mCount ) {
			memmove( mList + insertPoint + 1, mList + insertPoint,
					( mCount - insertPoint ) * sizeof( void * ) );
		}

		mList[ insertPoint ] = new SP_DictSortedArrayNode( item );
		mCount++;
	}

	return index >= 0 ? 1 : 0;
}

const void * SP_DictSortedArray :: search( const void * key ) const
{
	const void * ret = NULL;

	int index = binarySearch( key );
	if( index >= 0 ) ret = mList[ index ]->getItem();

	return ret;
}

void * SP_DictSortedArray :: remove( const void * key )
{
	void * ret = NULL;

	int index = binarySearch( key );
	if( index >= 0 ) {
		SP_DictSortedArrayNode * node = mList[ index ];
		memmove( mList + index, mList + index + 1, ( mCount - index - 1 ) * sizeof( void * ) );

		ret = node->takeItem();
		delete node;

		mCount--;
	}

	return ret;
}

int SP_DictSortedArray :: getCount() const
{
	return mCount;
}

SP_DictIterator * SP_DictSortedArray :: getIterator() const
{
	return new SP_DictSortedArrayIterator( mList, mCount );
}


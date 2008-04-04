/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "spdictbtree.hpp"

//===========================================================================

SP_DictBTreeNode :: SP_DictBTreeNode( int maxCount, SP_DictHandler * handler )
		: mMaxCount( maxCount )
{
	mNodeCount = mItemCount = 0;
	mHandler = handler;
	mNodeList = (SP_DictBTreeNode**)malloc(
			sizeof( void * ) * ( mMaxCount + 1 ) );
	memset( mNodeList, 0, sizeof( void * ) * ( mMaxCount + 1 ) );
	mItemList = (void**)malloc( sizeof( void * ) * mMaxCount );
	memset( mItemList, 0, sizeof( void * ) * mMaxCount );

	mParent = NULL;
}

SP_DictBTreeNode :: ~SP_DictBTreeNode()
{
	for( int i = 0; i < mNodeCount; i++ ) {
		delete mNodeList[i];
	}

	for( int j = 0; j < mItemCount; j++ ) {
		mHandler->destroy( mItemList[j] );
	}

	free( mNodeList );
	free( mItemList );
}

int SP_DictBTreeNode :: getItemCount() const
{
	return mItemCount;
}

int SP_DictBTreeNode :: getNodeCount() const
{
	return mNodeCount;
}

void SP_DictBTreeNode :: insertItem( int index, void * item )
{
	assert( NULL != item );
	if( index >= 0 && mItemCount < mMaxCount ) {
		if( index >= mItemCount ) {
			mItemList[ mItemCount ] = item;
		} else {
			for( int i = mItemCount; i > index; i-- ) {
				mItemList[ i ] = mItemList[ i - 1 ];
			}
			mItemList[ index ] = item;
		}
		mItemCount++;
	} else {
		printf( "fatal error, out of buffer for item\n" );
		mHandler->destroy( item );
	}
}

void SP_DictBTreeNode :: appendItem( void * item )
{
	insertItem( mItemCount, item );
}

void * SP_DictBTreeNode :: takeItem( int index )
{
	void * item = NULL;
	if( index >= 0 && index < mItemCount ) {
		item = mItemList[ index ];
		mItemCount--;
		for( int i = index; i < mItemCount; i++ ) {
			mItemList[ i ] = mItemList[ i + 1 ];
		}
		mItemList[ mItemCount ] = 0;
	}
	return item;
}

void * SP_DictBTreeNode :: getItem( int index ) const
{
	if( index >= 0 && index < mItemCount ) {
		return mItemList[ index ];
	}

	return NULL;
}

void SP_DictBTreeNode :: updateItem( int index, void * item )
{
	if( index >= 0 && index < mItemCount ) {
		mHandler->destroy( mItemList[ index ] );
		mItemList[ index ] = item;
	} else {
		printf( "fatal error, out of buffer for item\n" );
		mHandler->destroy( item );
	}
}

void SP_DictBTreeNode :: insertNode( int index, SP_DictBTreeNode * node )
{
	if( NULL == node ) return;
	if( index >= 0 && mNodeCount <= mMaxCount ) {
		if( index >= mNodeCount ) {
			mNodeList[ mNodeCount ] = node;
		} else {
			for( int i = mNodeCount; i > index; i-- ) {
				mNodeList[ i ] = mNodeList[ i - 1 ];
			}
			mNodeList[ index ] = node;
		}
		node->setParent( this );
		mNodeCount++;
	} else {
		printf( "fatal error, out of buffer for node\n" );
		delete node;
	}
}

void SP_DictBTreeNode :: appendNode( SP_DictBTreeNode * node )
{
	insertNode( mNodeCount, node );
}

SP_DictBTreeNode * SP_DictBTreeNode :: takeNode( int index )
{
	SP_DictBTreeNode * node = NULL;
	if( index >= 0 && index < mNodeCount ) {
		node = mNodeList[ index ];
		mNodeCount--;
		for( int i = index; i < mNodeCount; i++ ) {
			mNodeList[ i ] = mNodeList[ i + 1 ];
		}
		mNodeList[ mNodeCount ] = NULL;
	}
	return node;
}

SP_DictBTreeNode * SP_DictBTreeNode :: getNode( int index ) const
{
	if( index >= 0 && index < mNodeCount ) {
		return mNodeList[ index ];
	}

	return NULL;
}

void SP_DictBTreeNode :: setParent( SP_DictBTreeNode * parent )
{
	mParent = parent;
}

SP_DictBTreeNode * SP_DictBTreeNode :: getParent() const
{
	return mParent;
}

int SP_DictBTreeNode :: needMerge() const
{
	return mItemCount < ( ( mMaxCount + 1 ) / 2 - 1 );
}

int SP_DictBTreeNode :: needSplit() const
{
	return mItemCount >= mMaxCount ? 1 : 0;
}

int SP_DictBTreeNode :: canSplit() const
{
	return mItemCount > ( ( mMaxCount + 1 ) / 2 - 1 );
}

int SP_DictBTreeNode :: nodeIndex( const SP_DictBTreeNode * node ) const
{
	for( int i = 0; i < mNodeCount; i++ ) {
		if( mNodeList[ i ] == node ) return i;
	}
	return -1;
}

// @return >= 0 : found, -1 : not found
int SP_DictBTreeNode :: search( const void * item, int * insertPoint,
		int firstIndex, int size ) const
{
	// if size not specify, then search the hold list
	if( size == -1 ) size = mItemCount;
	// can't find the key
	if( size == 0 ) {
		// set the insert point
		if( insertPoint != NULL ) * insertPoint = firstIndex; 
		return -1;  // return "can't find"
	}

	int cmpRet = mHandler->compare( item,
			mItemList[ firstIndex + ( size - 1 ) / 2 ] );
	if( cmpRet < 0 ) {
		return search( item, insertPoint, firstIndex, ( size - 1 ) / 2 );
	} else if( cmpRet > 0 ) {
		return search( item, insertPoint, firstIndex + ( ( size - 1 ) / 2 ) + 1,
				size - ( ( size - 1 ) / 2 ) - 1 );
	} else { // find it
		return ( firstIndex + ( size - 1 ) / 2 );
	}
}

//===========================================================================

SP_DictBTreeSearchResult :: SP_DictBTreeSearchResult()
{
	mNode = NULL;
	mIndex = -1;
	mTag = 0;
}

SP_DictBTreeSearchResult :: ~SP_DictBTreeSearchResult()
{
}

void SP_DictBTreeSearchResult :: setNode( SP_DictBTreeNode * node )
{
	mNode = node;
}

SP_DictBTreeNode * SP_DictBTreeSearchResult :: getNode()
{
	return mNode;
}

void SP_DictBTreeSearchResult :: setIndex( int index )
{
	mIndex = index;
}

int SP_DictBTreeSearchResult :: getIndex()
{
	return mIndex;
}

void SP_DictBTreeSearchResult :: setTag( int tag )
{
	mTag = tag;
}

int SP_DictBTreeSearchResult :: getTag()
{
	return mTag;
}

//===========================================================================

SP_DictBTreeIterator :: SP_DictBTreeIterator( const SP_DictBTreeNode * root, int count )
{
	mCurrent = root;
	mCurrIndex = 0;
	mLevel = 0;
	mRemainCount = count;
}

SP_DictBTreeIterator :: ~SP_DictBTreeIterator()
{
}

const void * SP_DictBTreeIterator :: getNext( int * level )
{
	if( NULL != mCurrent->getNode( mCurrIndex ) ) {
		// move to leaf
		for( ; NULL != mCurrent->getNode( mCurrIndex ); ) {
			mCurrent = mCurrent->getNode( mCurrIndex );
			mCurrIndex = 0;
			mLevel++;
		}
	}

	const void * ret = NULL;
	const SP_DictBTreeNode * parent = mCurrent;
	for( ; NULL == ret && NULL != parent; ) {
		if( mCurrIndex < mCurrent->getItemCount() ) {
			ret = mCurrent->getItem( mCurrIndex++ );
		} else {
			// move to parent
			parent = mCurrent->getParent();
			if( NULL != parent ) {
				mCurrIndex = parent->nodeIndex( mCurrent );
				mCurrent = parent;
				mLevel--;
			}
		}
	}

	if( NULL != ret ) assert( mRemainCount-- >= 0 );

	if( NULL != level ) *level = mLevel;

	return ret;
}

//===========================================================================

SP_DictBTree :: SP_DictBTree( int rank, SP_DictHandler * handler )
		: mRank( rank )
{
	mRoot = new SP_DictBTreeNode( rank, handler );
	mHandler = handler;
	mCount = 0;
}

SP_DictBTree :: ~SP_DictBTree()
{
	if( NULL != mRoot ) delete mRoot;
	delete mHandler;
}

int SP_DictBTree :: getCount() const
{
	return mCount;
}

SP_DictBTreeNode * SP_DictBTree :: split( int rank,
		SP_DictHandler * handler, SP_DictBTreeNode * node )
{
	SP_DictBTreeNode * sibling = new SP_DictBTreeNode( rank, handler );
	int index = ( rank + 1 ) / 2;
	for( int i = index; i < rank; i++ ) {
		sibling->appendItem( node->takeItem( index ) );
		sibling->appendNode( node->takeNode( index ) );
	}
	sibling->appendNode( node->takeNode( index ) );

	assert( sibling->getItemCount() == ( rank - index ) );
	assert( node->getItemCount() == index );

	return sibling;
}

int SP_DictBTree :: insert( void * item )
{
	SP_DictBTreeSearchResult result;
	search( mRoot, item, &result );

	if( 0 == result.getTag() ) {
		mCount++;

		SP_DictBTreeNode * curr = result.getNode();
		SP_DictBTreeNode * child = NULL;
		int index = result.getIndex();

		for( ; ; ) {
			curr->insertItem( index, item );
			curr->insertNode( index + 1, child );

			if( curr->needSplit() ) {
				child = split( mRank, mHandler, curr );
				item = curr->takeItem( ( mRank + 1 ) / 2 - 1 );
				assert( NULL != item );
				if( NULL == curr->getParent() ) {
					mRoot = new SP_DictBTreeNode( mRank, mHandler );
					mRoot->insertNode( 0, curr );
				}
				curr = curr->getParent();
				if( curr->search( item, &index ) >= 0 ) {
					printf( "fatal error, overwrite item\n" );
				}
			} else {
				break;
			}
		}
	} else {
		result.getNode()->updateItem( result.getIndex(), item );
		printf( "overwrite\n" );
	}

	return result.getTag();
}

const void * SP_DictBTree :: search( const void * key ) const
{
	SP_DictBTreeSearchResult result;
	search( mRoot, key, &result );

	if( 0 != result.getTag() ) {
		return result.getNode()->getItem( result.getIndex() );
	}

	return NULL;
}

void SP_DictBTree :: search( SP_DictBTreeNode * node, const void * key,
			SP_DictBTreeSearchResult * result )
{
	int stop = 0;
	for( SP_DictBTreeNode * curr = node; 0 == stop; ) {
		int insertPoint = -1;
		int index = curr->search( key, &insertPoint );
		if( index >= 0 ) {
			stop = 1;
			result->setNode( curr );
			result->setIndex( index );
			result->setTag( 1 );
		} else {
			if( NULL == curr->getNode( insertPoint ) ) {
				stop = 1;
				result->setNode( curr );
				result->setIndex( insertPoint );
				result->setTag( 0 );
			} else {
				curr = curr->getNode( insertPoint );
			}
		}
	}
}

SP_DictBTreeNode * SP_DictBTree :: findLeaf( SP_DictBTreeNode * node )
{
	if( NULL != node ) {
		for( ; NULL != node->getNode( 0 ); ) {
			node = node->getNode( 0 );
		}
	}

	return node;
}

SP_DictBTreeNode * SP_DictBTree :: merge( int rank, SP_DictBTreeNode * node )
{
	SP_DictBTreeNode * parent = node->getParent();
	if( NULL != parent ) {
		int index = parent->nodeIndex( node );
		if( index < 0 ) {
			printf( "fatal error, invalid child\n" );
		}

		SP_DictBTreeNode * left = parent->getNode( index - 1 );
		SP_DictBTreeNode * right = parent->getNode( index + 1 );

		if( NULL != right ) {
			if( right->canSplit() ) {
				void * item = parent->takeItem( index );
				node->appendItem( item );
				node->appendNode( right->takeNode( 0 ) );
				item = right->takeItem( 0 );
				parent->insertItem( index, item );
				assert( node->getItemCount() == ( ( rank + 1 ) / 2 - 1 ) );
				assert( right->getItemCount() >= ( ( rank + 1 ) / 2 - 1 ) );
			} else {
				void * item = parent->takeItem( index );
				parent->takeNode( index + 1 );
				node->appendItem( item );
				for( ; right->getItemCount() > 0; ) {
					node->appendItem( right->takeItem( 0 ) );
					node->appendNode( right->takeNode( 0 ) );
				}
				node->appendNode( right->takeNode( 0 ) );
				assert( node->getItemCount() >= ( rank + 1 ) / 2 );
				assert( right->getItemCount() == 0 );
				delete right;
			}
		} else if( NULL != left ) {
			if( left->canSplit() ) {
				void * item = parent->takeItem( index - 1 );
				node->insertItem( 0, item );
				node->insertNode( 0,
					left->takeNode( left->getNodeCount() - 1 ) );
				item = left->takeItem( left->getItemCount() - 1 );
				parent->insertItem( index - 1, item );
				assert( node->getItemCount() == ( ( rank + 1 ) / 2 - 1 ) );
				assert( left->getItemCount() >= ( ( rank + 1 ) / 2 - 1 ) );
			} else {
				void * item = parent->takeItem( index - 1 );
				parent->takeNode( index );
				left->appendItem( item );
				for( ; node->getItemCount() > 0; ) {
					left->appendItem( node->takeItem( 0 ) );
					left->appendNode( node->takeNode( 0 ) );
				}
				left->appendNode( node->takeNode( 0 ) );
				assert( left->getItemCount() >= ( rank + 1 ) / 2 );
				assert( node->getItemCount() == 0 );
				delete node;
			}
		}
	}

	return parent;
}

void * SP_DictBTree :: remove( const void * key )
{
	void * ret = NULL;

	SP_DictBTreeSearchResult result;
	search( mRoot, key, &result );

	if( 0 != result.getTag() ) {
		mCount--;

		SP_DictBTreeNode * curr = result.getNode();
		int index = result.getIndex();

		SP_DictBTreeNode * leaf = findLeaf( curr->getNode( index + 1 ) );
		if( NULL != leaf ) {
			void * item = leaf->takeItem( 0 );
			ret = curr->takeItem( index );
			curr->insertItem( index, item );
			curr = leaf;
		} else {
			ret = curr->takeItem( index );
		}

		for( ; NULL != curr && curr->needMerge(); ) {
			curr = merge( mRank, curr );
		}
		if( 0 == mRoot->getItemCount() && NULL != mRoot->getNode( 0 ) ) {
			curr = mRoot;
			mRoot = mRoot->takeNode( 0 );
			mRoot->setParent( NULL );

			delete curr;
		}
	}

	return ret;
}

SP_DictIterator * SP_DictBTree :: getIterator() const
{
	return new SP_DictBTreeIterator( mRoot, mCount );
}


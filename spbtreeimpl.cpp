/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "spbtreeimpl.hpp"

//===========================================================================

SP_BTreeNode :: SP_BTreeNode( int maxCount, SP_DictHandler * handler )
		: mMaxCount( maxCount )
{
	mNodeCount = mItemCount = 0;
	mHandler = handler;
	mNodeList = (SP_BTreeNode**)malloc(
			sizeof( void * ) * ( mMaxCount + 1 ) );
	memset( mNodeList, 0, sizeof( void * ) * ( mMaxCount + 1 ) );
	mItemList = (void**)malloc( sizeof( void * ) * mMaxCount );
	memset( mItemList, 0, sizeof( void * ) * mMaxCount );

	mParent = NULL;
}

SP_BTreeNode :: ~SP_BTreeNode()
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

int SP_BTreeNode :: getItemCount() const
{
	return mItemCount;
}

int SP_BTreeNode :: getNodeCount() const
{
	return mNodeCount;
}

void SP_BTreeNode :: insertItem( int index, void * item )
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

void SP_BTreeNode :: appendItem( void * item )
{
	insertItem( mItemCount, item );
}

void * SP_BTreeNode :: takeItem( int index )
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

void * SP_BTreeNode :: getItem( int index ) const
{
	if( index >= 0 && index < mItemCount ) {
		return mItemList[ index ];
	}

	return NULL;
}

void SP_BTreeNode :: updateItem( int index, void * item )
{
	if( index >= 0 && index < mItemCount ) {
		mHandler->destroy( mItemList[ index ] );
		mItemList[ index ] = item;
	} else {
		printf( "fatal error, out of buffer for item\n" );
		mHandler->destroy( item );
	}
}

void SP_BTreeNode :: insertNode( int index, SP_BTreeNode * node )
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

void SP_BTreeNode :: appendNode( SP_BTreeNode * node )
{
	insertNode( mNodeCount, node );
}

SP_BTreeNode * SP_BTreeNode :: takeNode( int index )
{
	SP_BTreeNode * node = NULL;
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

SP_BTreeNode * SP_BTreeNode :: getNode( int index ) const
{
	if( index >= 0 && index < mNodeCount ) {
		return mNodeList[ index ];
	}

	return NULL;
}

void SP_BTreeNode :: setParent( SP_BTreeNode * parent )
{
	mParent = parent;
}

SP_BTreeNode * SP_BTreeNode :: getParent() const
{
	return mParent;
}

int SP_BTreeNode :: needMerge() const
{
	return mItemCount < ( ( mMaxCount + 1 ) / 2 - 1 );
}

int SP_BTreeNode :: needSplit() const
{
	return mItemCount >= mMaxCount ? 1 : 0;
}

int SP_BTreeNode :: canSplit() const
{
	return mItemCount > ( ( mMaxCount + 1 ) / 2 - 1 );
}

int SP_BTreeNode :: nodeIndex( const SP_BTreeNode * node ) const
{
	for( int i = 0; i < mNodeCount; i++ ) {
		if( mNodeList[ i ] == node ) return i;
	}
	return -1;
}

// @return >= 0 : found, -1 : not found
int SP_BTreeNode :: search( const void * item, int * insertPoint,
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

SP_BTreeSearchResult :: SP_BTreeSearchResult()
{
	mNode = NULL;
	mIndex = -1;
	mTag = 0;
}

SP_BTreeSearchResult :: ~SP_BTreeSearchResult()
{
}

void SP_BTreeSearchResult :: setNode( SP_BTreeNode * node )
{
	mNode = node;
}

SP_BTreeNode * SP_BTreeSearchResult :: getNode()
{
	return mNode;
}

void SP_BTreeSearchResult :: setIndex( int index )
{
	mIndex = index;
}

int SP_BTreeSearchResult :: getIndex()
{
	return mIndex;
}

void SP_BTreeSearchResult :: setTag( int tag )
{
	mTag = tag;
}

int SP_BTreeSearchResult :: getTag()
{
	return mTag;
}

//===========================================================================

SP_BTreeIterator :: SP_BTreeIterator( const SP_BTreeNode * root, int count )
{
	mCurrent = root;
	mCurrIndex = 0;
	mLevel = 0;
	mRemainCount = count;
}

SP_BTreeIterator :: ~SP_BTreeIterator()
{
}

const void * SP_BTreeIterator :: getNext( int * level )
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
	const SP_BTreeNode * parent = mCurrent;
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

SP_BTreeImpl :: SP_BTreeImpl( int rank, SP_DictHandler * handler )
		: mRank( rank )
{
	mRoot = new SP_BTreeNode( rank, handler );
	mHandler = handler;
	mCount = 0;
}

SP_BTreeImpl :: ~SP_BTreeImpl()
{
	if( NULL != mRoot ) delete mRoot;
	delete mHandler;
}

int SP_BTreeImpl :: getCount() const
{
	return mCount;
}

SP_BTreeNode * SP_BTreeImpl :: split( int rank,
		SP_DictHandler * handler, SP_BTreeNode * node )
{
	SP_BTreeNode * sibling = new SP_BTreeNode( rank, handler );
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

int SP_BTreeImpl :: insert( void * item )
{
	SP_BTreeSearchResult result;
	search( mRoot, item, &result );

	if( 0 == result.getTag() ) {
		mCount++;

		SP_BTreeNode * curr = result.getNode();
		SP_BTreeNode * child = NULL;
		int index = result.getIndex();

		for( ; ; ) {
			curr->insertItem( index, item );
			curr->insertNode( index + 1, child );

			if( curr->needSplit() ) {
				child = split( mRank, mHandler, curr );
				item = curr->takeItem( ( mRank + 1 ) / 2 - 1 );
				assert( NULL != item );
				if( NULL == curr->getParent() ) {
					mRoot = new SP_BTreeNode( mRank, mHandler );
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

const void * SP_BTreeImpl :: search( const void * key ) const
{
	SP_BTreeSearchResult result;
	search( mRoot, key, &result );

	if( 0 != result.getTag() ) {
		return result.getNode()->getItem( result.getIndex() );
	}

	return NULL;
}

void SP_BTreeImpl :: search( SP_BTreeNode * node, const void * key,
			SP_BTreeSearchResult * result )
{
	int stop = 0;
	for( SP_BTreeNode * curr = node; 0 == stop; ) {
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

SP_BTreeNode * SP_BTreeImpl :: findLeaf( SP_BTreeNode * node )
{
	if( NULL != node ) {
		for( ; NULL != node->getNode( 0 ); ) {
			node = node->getNode( 0 );
		}
	}

	return node;
}

SP_BTreeNode * SP_BTreeImpl :: merge( int rank, SP_BTreeNode * node )
{
	SP_BTreeNode * parent = node->getParent();
	if( NULL != parent ) {
		int index = parent->nodeIndex( node );
		if( index < 0 ) {
			printf( "fatal error, invalid child\n" );
		}

		SP_BTreeNode * left = parent->getNode( index - 1 );
		SP_BTreeNode * right = parent->getNode( index + 1 );

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

void * SP_BTreeImpl :: remove( const void * key )
{
	void * ret = NULL;

	SP_BTreeSearchResult result;
	search( mRoot, key, &result );

	if( 0 != result.getTag() ) {
		mCount--;

		SP_BTreeNode * curr = result.getNode();
		int index = result.getIndex();

		SP_BTreeNode * leaf = findLeaf( curr->getNode( index + 1 ) );
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

SP_DictIterator * SP_BTreeImpl :: getIterator() const
{
	return new SP_BTreeIterator( mRoot, mCount );
}


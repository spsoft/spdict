/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "spdictbstree.hpp"

//===========================================================================

SP_DictBSTreeNode :: SP_DictBSTreeNode( void * item )
{
	mLeft = mRight = NULL;
	mItem = item;
}

SP_DictBSTreeNode :: ~SP_DictBSTreeNode()
{
	if( NULL != mLeft ) delete mLeft;
	if( NULL != mRight ) delete mRight;
}

SP_DictBSTreeNode * SP_DictBSTreeNode :: getLeft() const
{
	return mLeft;
}

void SP_DictBSTreeNode :: setLeft( SP_DictBSTreeNode * left )
{
	mLeft = left;
}

SP_DictBSTreeNode * SP_DictBSTreeNode :: getRight() const
{
	return mRight;
}

void SP_DictBSTreeNode :: setRight( SP_DictBSTreeNode * right )
{
	mRight = right;
}

const void * SP_DictBSTreeNode :: getItem() const
{
	return mItem;
}

void * SP_DictBSTreeNode :: takeItem()
{
	void * item = mItem;
	mItem = NULL;

	return item;
}

void SP_DictBSTreeNode :: setItem( void * item )
{
	mItem = item;
}

//===========================================================================

SP_MyMiniStack :: SP_MyMiniStack()
{
	mCount = 0;
	mMaxCount = 128;
	mItemList = (void**)malloc( mMaxCount * sizeof( void * ) );
	memset( mItemList, 0, mMaxCount * sizeof( void * ) );
}

SP_MyMiniStack :: ~SP_MyMiniStack()
{
	free( mItemList );
}

void SP_MyMiniStack :: push( void * item )
{
	if( mCount >= mMaxCount ) {
		mMaxCount = ( mMaxCount * 3 ) / 2 + 1;
		mItemList = (void**)realloc( mItemList, mMaxCount * sizeof( void * ) );
		memset( mItemList + mCount, 0, ( mMaxCount - mCount ) * sizeof( void * ) );
	}

	mItemList[ mCount++ ] = item;
}

void * SP_MyMiniStack :: pop()
{
	if( mCount > 0 ) return mItemList[ --mCount ];
	return NULL;
}

int SP_MyMiniStack :: isEmpty()
{
	return 0 == mCount;
}

//===========================================================================

SP_DictBSTreeIterator :: SP_DictBSTreeIterator( const SP_DictBSTreeNode * root, int count )
{
	mLevel = 0;
	mRemainCount = count;

	mStack = new SP_MyMiniStack();
	pushLeft( mStack, root );
}

SP_DictBSTreeIterator :: ~SP_DictBSTreeIterator()
{
	delete mStack;
}

void SP_DictBSTreeIterator :: pushLeft( SP_MyMiniStack * stack, const SP_DictBSTreeNode * node )
{
	for( ; NULL != node; ) {
		stack->push( (void*)node );
		node = node->getLeft();
	}
}

const void * SP_DictBSTreeIterator :: getNext( int * level )
{
	if( mStack->isEmpty() ) return NULL;

	SP_DictBSTreeNode * node = (SP_DictBSTreeNode*)mStack->pop();
	pushLeft( mStack, node->getRight() );

	assert( mRemainCount-- >= 0 );

	return node->getItem();
}

//===========================================================================

SP_DictBSTree :: SP_DictBSTree( SP_DictHandler * handler )
{
	mRoot = NULL;
	mHandler = handler;
	mCount = 0;
}

SP_DictBSTree :: ~SP_DictBSTree()
{
	freeItem( mRoot, mHandler );
	if( NULL != mRoot ) delete mRoot;
	delete mHandler;
}

void SP_DictBSTree :: freeItem( SP_DictBSTreeNode * node,
		SP_DictHandler * handler )
{
	if( NULL != node ) {
		freeItem( node->getLeft(), handler );
		handler->destroy( node->takeItem() );
		freeItem( node->getRight(), handler );
	}
}

int SP_DictBSTree :: getCount() const
{
	return mCount;
}

int SP_DictBSTree :: insert( void * item )
{
	int ret = 0;
	if( NULL == mRoot ) {
		mCount++;
		mRoot = new SP_DictBSTreeNode( item );
	} else {
		for( SP_DictBSTreeNode * curr = mRoot; NULL != curr; ) {
			int cmpRet = mHandler->compare( item, curr->getItem() );
			if( 0 == cmpRet ) {
				ret = 1;
				mHandler->destroy( curr->takeItem() );
				curr->setItem( item );
				curr = NULL;
			} else if( cmpRet > 0 ) {
				if( NULL == curr->getRight() ) {
					mCount++;
					curr->setRight( new SP_DictBSTreeNode( item ) );
					curr = NULL;
				} else {
					curr = curr->getRight();
				}
			} else {
				if( NULL == curr->getLeft() ) {
					mCount++;
					curr->setLeft( new SP_DictBSTreeNode( item ) );
					curr = NULL;
				} else {
					curr = curr->getLeft();
				}
			}
		}
	}

	return ret;
}

const void * SP_DictBSTree :: search( const void * key ) const
{
	const SP_DictBSTreeNode * node = NULL, * curr = mRoot;

	for( ; NULL == node && NULL != curr; ) {
		int ret = mHandler->compare( key, curr->getItem() );
		if( 0 == ret ) {
			node = curr;
		} else if( ret > 0 ) {
			curr = curr->getRight();
		} else {
			curr = curr->getLeft();
		}
	}

	return NULL != node ? node->getItem() : NULL;
}

void * SP_DictBSTree :: remove( const void * key )
{
	void * ret = NULL;

	if( NULL != mRoot ) {
		SP_DictBSTreeNode * parent = mRoot, * curr = mRoot;

		int cmpRet = 0;
		do {
			cmpRet = mHandler->compare( key, curr->getItem() );
			if( cmpRet > 0 ) {
				parent = curr;
				curr = curr->getRight();
			} else if( cmpRet < 0 ) {
				parent = curr;
				curr = curr->getLeft();
			}
		} while( 0 != cmpRet && NULL != curr );

		if( 0 == cmpRet ) {
			if( curr == mRoot ) {
				mRoot = removeTop( mRoot );
			} else {
				if( parent->getRight() == curr ) {
					parent->setRight( removeTop( curr ) );
				} else {
					parent->setLeft( removeTop( curr ) );
				}
			}
			curr->setLeft( NULL );
			curr->setRight( NULL );
			ret = curr->takeItem();
			delete curr;
			mCount--;
		}
	}

	return ret;
}

SP_DictBSTreeNode * SP_DictBSTree :: removeTop( SP_DictBSTreeNode * node )
{
	SP_DictBSTreeNode * left = node->getLeft();
	SP_DictBSTreeNode * right = node->getRight();

	//1. not left child
	if( NULL == left ) return right;

	//2. not right child
	if( NULL == right ) return left;

	//3. right child has not left child
	SP_DictBSTreeNode * curr = right->getLeft();
	if( NULL == curr ) {
		right->setLeft( left );
		return right;
	}

	//4. find right child's left child
	SP_DictBSTreeNode * parent = right;
	for( ; NULL != curr->getLeft(); ) {
		parent = curr;
		curr = curr->getLeft();
	}

	parent->setLeft( curr->getRight() );
	curr->setLeft( left );
	curr->setRight( right );

	return curr;
}

SP_DictIterator * SP_DictBSTree :: getIterator() const
{
	return new SP_DictBSTreeIterator( mRoot, mCount );
}


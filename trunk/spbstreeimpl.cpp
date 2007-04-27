/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "spbstreeimpl.hpp"

//===========================================================================

SP_BSTreeNode :: SP_BSTreeNode( void * item )
{
	mLeft = mRight = NULL;
	mItem = item;
}

SP_BSTreeNode :: ~SP_BSTreeNode()
{
	if( NULL != mLeft ) delete mLeft;
	if( NULL != mRight ) delete mRight;
}

SP_BSTreeNode * SP_BSTreeNode :: getLeft() const
{
	return mLeft;
}

void SP_BSTreeNode :: setLeft( SP_BSTreeNode * left )
{
	mLeft = left;
}

SP_BSTreeNode * SP_BSTreeNode :: getRight() const
{
	return mRight;
}

void SP_BSTreeNode :: setRight( SP_BSTreeNode * right )
{
	mRight = right;
}

const void * SP_BSTreeNode :: getItem() const
{
	return mItem;
}

void * SP_BSTreeNode :: takeItem()
{
	void * item = mItem;
	mItem = NULL;

	return item;
}

void SP_BSTreeNode :: setItem( void * item )
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

SP_BSTreeIterator :: SP_BSTreeIterator( const SP_BSTreeNode * root, int count )
{
	mLevel = 0;
	mRemainCount = count;

	mStack = new SP_MyMiniStack();
	pushLeft( mStack, root );
}

SP_BSTreeIterator :: ~SP_BSTreeIterator()
{
	delete mStack;
}

void SP_BSTreeIterator :: pushLeft( SP_MyMiniStack * stack, const SP_BSTreeNode * node )
{
	for( ; NULL != node; ) {
		stack->push( (void*)node );
		node = node->getLeft();
	}
}

const void * SP_BSTreeIterator :: getNext( int * level )
{
	if( mStack->isEmpty() ) return NULL;

	SP_BSTreeNode * node = (SP_BSTreeNode*)mStack->pop();
	pushLeft( mStack, node->getRight() );

	assert( mRemainCount-- >= 0 );

	return node->getItem();
}

//===========================================================================

SP_BSTreeImpl :: SP_BSTreeImpl( SP_DictHandler * handler )
{
	mRoot = NULL;
	mHandler = handler;
	mCount = 0;
}

SP_BSTreeImpl :: ~SP_BSTreeImpl()
{
	freeItem( mRoot, mHandler );
	if( NULL != mRoot ) delete mRoot;
	delete mHandler;
}

void SP_BSTreeImpl :: freeItem( SP_BSTreeNode * node,
		SP_DictHandler * handler )
{
	if( NULL != node ) {
		freeItem( node->getLeft(), handler );
		handler->destroy( node->takeItem() );
		freeItem( node->getRight(), handler );
	}
}

int SP_BSTreeImpl :: getCount() const
{
	return mCount;
}

int SP_BSTreeImpl :: insert( void * item )
{
	int ret = 0;
	if( NULL == mRoot ) {
		mCount++;
		mRoot = new SP_BSTreeNode( item );
	} else {
		for( SP_BSTreeNode * curr = mRoot; NULL != curr; ) {
			int cmpRet = mHandler->compare( item, curr->getItem() );
			if( 0 == cmpRet ) {
				ret = 1;
				mHandler->destroy( curr->takeItem() );
				curr->setItem( item );
				curr = NULL;
			} else if( cmpRet > 0 ) {
				if( NULL == curr->getRight() ) {
					mCount++;
					curr->setRight( new SP_BSTreeNode( item ) );
					curr = NULL;
				} else {
					curr = curr->getRight();
				}
			} else {
				if( NULL == curr->getLeft() ) {
					mCount++;
					curr->setLeft( new SP_BSTreeNode( item ) );
					curr = NULL;
				} else {
					curr = curr->getLeft();
				}
			}
		}
	}

	return ret;
}

const void * SP_BSTreeImpl :: search( const void * key ) const
{
	const SP_BSTreeNode * node = NULL, * curr = mRoot;

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

void * SP_BSTreeImpl :: remove( const void * key )
{
	void * ret = NULL;

	if( NULL != mRoot ) {
		SP_BSTreeNode * parent = mRoot, * curr = mRoot;

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

SP_BSTreeNode * SP_BSTreeImpl :: removeTop( SP_BSTreeNode * node )
{
	SP_BSTreeNode * left = node->getLeft();
	SP_BSTreeNode * right = node->getRight();

	//1. not left child
	if( NULL == left ) return right;

	//2. not right child
	if( NULL == right ) return left;

	//3. right child has not left child
	SP_BSTreeNode * curr = right->getLeft();
	if( NULL == curr ) {
		right->setLeft( left );
		return right;
	}

	//4. find right child's left child
	SP_BSTreeNode * parent = right;
	for( ; NULL != curr->getLeft(); ) {
		parent = curr;
		curr = curr->getLeft();
	}

	parent->setLeft( curr->getRight() );
	curr->setLeft( left );
	curr->setRight( right );

	return curr;
}

SP_DictIterator * SP_BSTreeImpl :: getIterator() const
{
	return new SP_BSTreeIterator( mRoot, mCount );
}


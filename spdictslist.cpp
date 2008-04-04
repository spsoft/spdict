/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "spdictslist.hpp"

//===========================================================================

SP_DictSkipListNode :: SP_DictSkipListNode( int maxLevel, void * item )
		: mMaxLevel( maxLevel )
{
	mItem = item;
	mForward = (SP_DictSkipListNode**)malloc( sizeof( void * ) * mMaxLevel );
	memset( mForward, 0, sizeof( void * ) * mMaxLevel );
}

SP_DictSkipListNode :: ~SP_DictSkipListNode()
{
	free( mForward );
}

void SP_DictSkipListNode :: setForward( int level, SP_DictSkipListNode * node )
{
	if( level >= 0 && level < mMaxLevel ) {
		mForward[ level ] = node;
	} else {
		printf( "fatal error, out of forward\n" );
	}
}

SP_DictSkipListNode * SP_DictSkipListNode :: getForward( int level ) const
{
	if( level >= 0 && level < mMaxLevel ) {
		return mForward[ level ];
	}

	return NULL;
}

void SP_DictSkipListNode :: setMaxLevel( int maxLevel )
{
	if( maxLevel > mMaxLevel ) {
		mForward = (SP_DictSkipListNode**)realloc(
				mForward, sizeof( void * ) * maxLevel );
		memset( mForward + mMaxLevel, 0,
				( maxLevel - mMaxLevel ) * sizeof( void * ) );
		mMaxLevel = maxLevel;
	}
	mMaxLevel = maxLevel;
}

int SP_DictSkipListNode :: getMaxLevel() const
{
	return mMaxLevel;
}

void SP_DictSkipListNode :: setItem( void * item )
{
	mItem = item;
}

void * SP_DictSkipListNode :: getItem() const
{
	return mItem;
}

void * SP_DictSkipListNode :: takeItem()
{
	void * item = mItem;
	mItem = NULL;
	return item;
}

//===========================================================================
SP_DictSkipListIterator :: SP_DictSkipListIterator( const SP_DictSkipListNode * root, int count )
{
	mCurrent = root->getForward(0);
	mRemainCount = count;
}

SP_DictSkipListIterator :: ~SP_DictSkipListIterator()
{
}

const void * SP_DictSkipListIterator :: getNext( int * level )
{
	const void * ret = NULL;

	if( NULL != mCurrent ) {
		if( NULL != level ) * level = mCurrent->getMaxLevel();
		ret = mCurrent->getItem();
		mCurrent = mCurrent->getForward(0);
		assert( mRemainCount -- >= 0 );
	}

	return ret;
}

//===========================================================================

SP_DictSkipList :: SP_DictSkipList( int maxLevel, SP_DictHandler * handler )
		: mMaxLevel( maxLevel )
{
	mHandler = handler;
	mCount = 0;
	mRoot = new SP_DictSkipListNode( 0 );
}

SP_DictSkipList :: ~SP_DictSkipList()
{
	if( NULL != mRoot ) {
		for( SP_DictSkipListNode * curr = mRoot; NULL != curr; ) {
			mHandler->destroy( curr->takeItem() );
			SP_DictSkipListNode * next = curr->getForward( 0 );
			delete curr;
			curr = next;
		}
	}
	delete mHandler;
}

int SP_DictSkipList :: randomLevel( int maxLevel )
{
	return ( rand() % maxLevel ) + 1;
}

int SP_DictSkipList :: insert( void * item )
{
	SP_DictSkipListNode path( mMaxLevel );

	SP_DictSkipListNode * node = mRoot;
	int cmpRet = 1;
	for( int i = mRoot->getMaxLevel() - 1; i >= 0; i-- ) {
		SP_DictSkipListNode * next = node->getForward( i );
		for( ; NULL != next; ) {
			cmpRet = mHandler->compare( item, next->getItem() );
			if( cmpRet > 0 ) {
				node = next;
				next = node->getForward( i );
			} else {
				break;
			}
		}
		path.setForward( i, node );
	}

	int ret = 0;
	if( NULL != node && 0 == cmpRet ) {
		mHandler->destroy( node->takeItem() );
		node->setItem( item );
		ret = 1;
	} else {
		int level = randomLevel( mMaxLevel );
		if( level > mRoot->getMaxLevel() ) {
			for( int i = mRoot->getMaxLevel(); i < level; i++ ) {
				path.setForward( i, mRoot );
			}
			mRoot->setMaxLevel( level );
		}

		node = new SP_DictSkipListNode( level, item );
		for( int i = 0; i < level; i++ ) {
			node->setForward( i, path.getForward(i)->getForward( i ) );
			path.getForward(i)->setForward( i, node );
		}
		mCount++;
	}

	return ret;
}

const void * SP_DictSkipList :: search( const void * key ) const
{
	SP_DictSkipListNode * node = mRoot;
	int ret = 1;
	for( int i = mRoot->getMaxLevel() - 1; i >= 0 && 0 != ret; i-- ) {
		SP_DictSkipListNode * next = node->getForward( i );
		for( ; NULL != next; ) {
			ret = mHandler->compare( key, next->getItem() );
			if( ret > 0 ) {
				node = next;
				next = node->getForward( i );
			} else {
				break;
			}
		}
	}

	node = node->getForward( 0 );

	if( NULL != node && 0 == ret ) return node->getItem();

	return NULL;
}

void * SP_DictSkipList :: remove( const void * key )
{
	void * ret = NULL;

	SP_DictSkipListNode path( mMaxLevel );

	SP_DictSkipListNode * node = mRoot;
	int cmpRet = 1;
	for( int i = mRoot->getMaxLevel() - 1; i >= 0; i-- ) {
		SP_DictSkipListNode * next = node->getForward( i );
		for( ; NULL != next; ) {
			cmpRet = mHandler->compare( key, next->getItem() );
			if( cmpRet > 0 ) {
				node = next;
				next = node->getForward( i );
			} else {
				break;
			}
		}
		path.setForward( i, node );
	}

	node = node->getForward( 0 );

	if( NULL != node && 0 == cmpRet ) {
		for( int i = 0; i < mRoot->getMaxLevel(); i++ ) {
			SP_DictSkipListNode * curr = path.getForward( i );
			if( NULL != curr ) {
				if( curr->getForward(i) == node ) {
					curr->setForward( i, node->getForward( i ) );
				}
			}
		}
		ret = node->takeItem();
		delete node;
		mCount--;
	}

	for( int j = mRoot->getMaxLevel(); j > 0; j-- ) {
		if( NULL != mRoot->getForward( j - 1 ) ) {
			mRoot->setMaxLevel( j );
			break;
		}
	}

	return ret;
}

SP_DictIterator * SP_DictSkipList :: getIterator() const
{
	return new SP_DictSkipListIterator( mRoot, mCount );
}

int SP_DictSkipList :: getCount() const
{
	return mCount;
}


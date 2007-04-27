/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "spslistimpl.hpp"

//===========================================================================

SP_SkipListNode :: SP_SkipListNode( int maxLevel, void * item )
		: mMaxLevel( maxLevel )
{
	mItem = item;
	mForward = (SP_SkipListNode**)malloc( sizeof( void * ) * mMaxLevel );
	memset( mForward, 0, sizeof( void * ) * mMaxLevel );
}

SP_SkipListNode :: ~SP_SkipListNode()
{
	free( mForward );
}

void SP_SkipListNode :: setForward( int level, SP_SkipListNode * node )
{
	if( level >= 0 && level < mMaxLevel ) {
		mForward[ level ] = node;
	} else {
		printf( "fatal error, out of forward\n" );
	}
}

SP_SkipListNode * SP_SkipListNode :: getForward( int level ) const
{
	if( level >= 0 && level < mMaxLevel ) {
		return mForward[ level ];
	}

	return NULL;
}

void SP_SkipListNode :: setMaxLevel( int maxLevel )
{
	if( maxLevel > mMaxLevel ) {
		mForward = (SP_SkipListNode**)realloc(
				mForward, sizeof( void * ) * maxLevel );
		memset( mForward + mMaxLevel, 0,
				( maxLevel - mMaxLevel ) * sizeof( void * ) );
		mMaxLevel = maxLevel;
	}
	mMaxLevel = maxLevel;
}

int SP_SkipListNode :: getMaxLevel() const
{
	return mMaxLevel;
}

void SP_SkipListNode :: setItem( void * item )
{
	mItem = item;
}

void * SP_SkipListNode :: getItem() const
{
	return mItem;
}

void * SP_SkipListNode :: takeItem()
{
	void * item = mItem;
	mItem = NULL;
	return item;
}

//===========================================================================
SP_SkipListIterator :: SP_SkipListIterator( const SP_SkipListNode * root, int count )
{
	mCurrent = root->getForward(0);
	mRemainCount = count;
}

SP_SkipListIterator :: ~SP_SkipListIterator()
{
}

const void * SP_SkipListIterator :: getNext( int * level )
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

SP_SkipListImpl :: SP_SkipListImpl( int maxLevel, SP_DictHandler * handler )
		: mMaxLevel( maxLevel )
{
	mHandler = handler;
	mCount = 0;
	mRoot = new SP_SkipListNode( 0 );
}

SP_SkipListImpl :: ~SP_SkipListImpl()
{
	if( NULL != mRoot ) {
		for( SP_SkipListNode * curr = mRoot; NULL != curr; ) {
			mHandler->destroy( curr->takeItem() );
			SP_SkipListNode * next = curr->getForward( 0 );
			delete curr;
			curr = next;
		}
	}
	delete mHandler;
}

int SP_SkipListImpl :: randomLevel( int maxLevel )
{
	return ( rand() % maxLevel ) + 1;
}

int SP_SkipListImpl :: insert( void * item )
{
	SP_SkipListNode path( mMaxLevel );

	SP_SkipListNode * node = mRoot;
	int cmpRet = 1;
	for( int i = mRoot->getMaxLevel() - 1; i >= 0; i-- ) {
		SP_SkipListNode * next = node->getForward( i );
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

		node = new SP_SkipListNode( level, item );
		for( int i = 0; i < level; i++ ) {
			node->setForward( i, path.getForward(i)->getForward( i ) );
			path.getForward(i)->setForward( i, node );
		}
		mCount++;
	}

	return ret;
}

const void * SP_SkipListImpl :: search( const void * key ) const
{
	SP_SkipListNode * node = mRoot;
	int ret = 1;
	for( int i = mRoot->getMaxLevel() - 1; i >= 0 && 0 != ret; i-- ) {
		SP_SkipListNode * next = node->getForward( i );
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

void * SP_SkipListImpl :: remove( const void * key )
{
	void * ret = NULL;

	SP_SkipListNode path( mMaxLevel );

	SP_SkipListNode * node = mRoot;
	int cmpRet = 1;
	for( int i = mRoot->getMaxLevel() - 1; i >= 0; i-- ) {
		SP_SkipListNode * next = node->getForward( i );
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
			SP_SkipListNode * curr = path.getForward( i );
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

SP_DictIterator * SP_SkipListImpl :: getIterator() const
{
	return new SP_SkipListIterator( mRoot, mCount );
}

int SP_SkipListImpl :: getCount() const
{
	return mCount;
}


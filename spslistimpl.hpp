/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef ___slistimpl_hpp__
#define ___slistimpl_hpp__

#include <stdio.h>

#include "spdictionary.hpp"

// skip list
class SP_SkipListNode {
public:
	SP_SkipListNode( int maxLevel, void * item = 0 );
	~SP_SkipListNode();

	void setForward( int level, SP_SkipListNode * node );
	SP_SkipListNode * getForward( int level ) const;

	void setMaxLevel( int maxLevel );
	int getMaxLevel() const;

	void setItem( void * item );
	void * getItem() const;
	void * takeItem();

private:
	int mMaxLevel;
	void * mItem;
	SP_SkipListNode ** mForward;
};

class SP_SkipListIterator : public SP_DictIterator {
public:
	SP_SkipListIterator( const SP_SkipListNode * root, int count );
	virtual ~SP_SkipListIterator();

	virtual const void * getNext( int * level = 0 );

private:
	const SP_SkipListNode * mCurrent;
	int mRemainCount;
};

class SP_SkipListImpl : public SP_Dictionary {
public:
	SP_SkipListImpl( int maxLevel, SP_DictHandler * handler );
	virtual ~SP_SkipListImpl();

	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

private:
	static int randomLevel( int maxLevel );

	const int mMaxLevel;
	int mCount;
	SP_SkipListNode * mRoot;

	SP_DictHandler * mHandler;
};

#endif


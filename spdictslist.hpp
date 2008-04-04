/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef ___spdictslist_hpp__
#define ___spdictslist_hpp__

#include <stdio.h>

#include "spdictionary.hpp"

// skip list
class SP_DictSkipListNode {
public:
	SP_DictSkipListNode( int maxLevel, void * item = 0 );
	~SP_DictSkipListNode();

	void setForward( int level, SP_DictSkipListNode * node );
	SP_DictSkipListNode * getForward( int level ) const;

	void setMaxLevel( int maxLevel );
	int getMaxLevel() const;

	void setItem( void * item );
	void * getItem() const;
	void * takeItem();

private:
	int mMaxLevel;
	void * mItem;
	SP_DictSkipListNode ** mForward;
};

class SP_DictSkipListIterator : public SP_DictIterator {
public:
	SP_DictSkipListIterator( const SP_DictSkipListNode * root, int count );
	virtual ~SP_DictSkipListIterator();

	virtual const void * getNext( int * level = 0 );

private:
	const SP_DictSkipListNode * mCurrent;
	int mRemainCount;
};

class SP_DictSkipList : public SP_Dictionary {
public:
	SP_DictSkipList( int maxLevel, SP_DictHandler * handler );
	virtual ~SP_DictSkipList();

	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

private:
	static int randomLevel( int maxLevel );

	const int mMaxLevel;
	int mCount;
	SP_DictSkipListNode * mRoot;

	SP_DictHandler * mHandler;
};

#endif


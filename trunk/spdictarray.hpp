/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spdictarray_hpp__
#define __spdictarray_hpp__

#include <stdio.h>

#include "spdictionary.hpp"

// sorted array
class SP_DictSortedArrayNode {
public:
	SP_DictSortedArrayNode( void * item = 0 );
	~SP_DictSortedArrayNode();

	void setItem( void * item );
	void * getItem() const;
	void * takeItem();

private:
	void * mItem;
};

class SP_DictSortedArrayIterator : public SP_DictIterator {
public:
	SP_DictSortedArrayIterator( SP_DictSortedArrayNode ** list, int count );
	virtual ~SP_DictSortedArrayIterator();

	virtual const void * getNext( int * level = 0 );

private:
	SP_DictSortedArrayNode ** mList;
	int mCount;
	int mIndex;
};

class SP_DictSortedArray : public SP_Dictionary {
public:
	SP_DictSortedArray( SP_DictHandler * handler );
	virtual ~SP_DictSortedArray();	

	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

private:

	// @return >= 0 : found, -1 : not found
	int binarySearch( const void * item, int * insertPoint = 0,
			int firstIndex = 0, int size = -1 ) const;

	SP_DictSortedArrayNode ** mList;
	int mMaxCount;
	int mCount;

	SP_DictHandler * mHandler;
};

#endif


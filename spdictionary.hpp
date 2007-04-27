/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spdict_hpp__
#define __spdict_hpp__

class SP_DictHandler {
public:
	virtual ~SP_DictHandler();

	// @return 1 : item1 > item2, 0 : item1 == item2, -1 : item1 < item2
	virtual int compare( const void * item1,
			const void * item2 ) const = 0;

	virtual void destroy( void * item ) const = 0;
};

class SP_DictIterator {
public:
	virtual ~SP_DictIterator();

	// @return NOT NULL : OK, NULL : reach the end
	virtual const void * getNext( int * level = 0 ) = 0;
};

/**
 * Dictionary data structure
 */
class SP_Dictionary {
public:
	virtual ~SP_Dictionary();

	// @return 0 : insert ok, 1 : update ok
	virtual int insert( void * item ) = 0;

	// @return NOT NULL : OK, NULL : FAIL
	virtual const void * search( const void * key ) const = 0;

	/**
	 * @return NOT NULL : found the key, and remove it from the dictionary,
	 *           caller must destroy the return value
	 * @return NULL : FAIL
	 */
	virtual void * remove( const void * key ) = 0;

	// get item count of the dictionary
	virtual int getCount() const = 0;

	// get the iterator of the dictionary
	virtual SP_DictIterator * getIterator() const = 0;

	//============================================================

	static SP_Dictionary * newBTree( int rank, SP_DictHandler * handler );

	static SP_Dictionary * newSkipList( int maxLevel, SP_DictHandler * handler );

	enum { eBSTree, eRBTree, eBTree, eSkipList, eSortedArray };
	static SP_Dictionary * newInstance( int type, SP_DictHandler * handler );
};

#endif


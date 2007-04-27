/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef ___bstreeimpl_hpp__
#define ___bstreeimpl_hpp__

#include "spdictionary.hpp"

// binray search tree
class SP_BSTreeNode {
public:
	SP_BSTreeNode( void * item = 0 );
	~SP_BSTreeNode();

	SP_BSTreeNode * getLeft() const;
	void setLeft( SP_BSTreeNode * left );

	SP_BSTreeNode * getRight() const;
	void setRight( SP_BSTreeNode * right );

	const void * getItem() const;
	void * takeItem();
	void setItem( void * item );

private:
	SP_BSTreeNode * mLeft, * mRight;
	void * mItem;
};

class SP_MyMiniStack {
public:
	SP_MyMiniStack();
	~SP_MyMiniStack();

	void push( void * item );
	void * pop();
	int isEmpty();

private:
	void ** mItemList;
	int mMaxCount;
	int mCount;
};

class SP_BSTreeIterator : public SP_DictIterator {
public:
	SP_BSTreeIterator( const SP_BSTreeNode * root, int count );
	virtual ~SP_BSTreeIterator();

	virtual const void * getNext( int * level = 0 );

private:
	static void pushLeft( SP_MyMiniStack * stack, const SP_BSTreeNode * node );

private:
	int mLevel;
	int mRemainCount;
	SP_MyMiniStack * mStack;
};

class SP_BSTreeImpl : public SP_Dictionary {
public:
	SP_BSTreeImpl( SP_DictHandler * handler );
	virtual ~SP_BSTreeImpl();

	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

protected:

	static SP_BSTreeNode * removeTop( SP_BSTreeNode * apoNode );

	static void freeItem( SP_BSTreeNode * node, SP_DictHandler * handler );

	SP_BSTreeNode * mRoot;
	SP_DictHandler * mHandler;
	int mCount;
};

#endif


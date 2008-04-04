/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef ___spdictbstree_hpp__
#define ___spdictbstree_hpp__

#include "spdictionary.hpp"

// binray search tree
class SP_DictBSTreeNode {
public:
	SP_DictBSTreeNode( void * item = 0 );
	~SP_DictBSTreeNode();

	SP_DictBSTreeNode * getLeft() const;
	void setLeft( SP_DictBSTreeNode * left );

	SP_DictBSTreeNode * getRight() const;
	void setRight( SP_DictBSTreeNode * right );

	const void * getItem() const;
	void * takeItem();
	void setItem( void * item );

private:
	SP_DictBSTreeNode * mLeft, * mRight;
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

class SP_DictBSTreeIterator : public SP_DictIterator {
public:
	SP_DictBSTreeIterator( const SP_DictBSTreeNode * root, int count );
	virtual ~SP_DictBSTreeIterator();

	virtual const void * getNext( int * level = 0 );

private:
	static void pushLeft( SP_MyMiniStack * stack, const SP_DictBSTreeNode * node );

private:
	int mLevel;
	int mRemainCount;
	SP_MyMiniStack * mStack;
};

class SP_DictBSTree : public SP_Dictionary {
public:
	SP_DictBSTree( SP_DictHandler * handler );
	virtual ~SP_DictBSTree();

	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

protected:

	static SP_DictBSTreeNode * removeTop( SP_DictBSTreeNode * apoNode );

	static void freeItem( SP_DictBSTreeNode * node, SP_DictHandler * handler );

	SP_DictBSTreeNode * mRoot;
	SP_DictHandler * mHandler;
	int mCount;
};

#endif


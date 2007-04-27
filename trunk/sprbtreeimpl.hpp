/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __sprbtreeimpl_hpp__
#define __sprbtreeimpl_hpp__

#include "spdictionary.hpp"

// red-black tree
class SP_RBTreeNode {
public:
	SP_RBTreeNode( void * item );
	~SP_RBTreeNode();

	void setLeft( SP_RBTreeNode * left );
	SP_RBTreeNode * getLeft() const;

	void setRight( SP_RBTreeNode * right );
	SP_RBTreeNode * getRight() const;

	void setParent( SP_RBTreeNode * parent );
	SP_RBTreeNode * getParent() const;

	void setItem( void * item );
	const void * getItem() const;
	void * takeItem();

	enum { eRed, eBlack };
	void setColor( int color );
	int getColor() const;

private:
	void * mItem;	
	SP_RBTreeNode * mLeft, * mRight, * mParent;
	int mColor;
};

class SP_RBTreeIterator : public SP_DictIterator {
public:
	SP_RBTreeIterator( SP_RBTreeNode * node, SP_RBTreeNode * nil, int count );
	virtual ~SP_RBTreeIterator();

	// @return value that is stored in the RBTree, or null if reach the end
	virtual const void * getNext( int * level = 0 );

private:
	SP_RBTreeNode * mCurrent, * mNil;
	int mRemainCount;
	int mLevel;
};

class SP_RBTreeImpl : public SP_Dictionary {
public:
	SP_RBTreeImpl( SP_DictHandler * handler );
	virtual ~SP_RBTreeImpl();

	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

private:
	SP_RBTreeNode * searchNode( const void * key ) const;

	void reset();

	void insertFixup( SP_RBTreeNode * node );
	void removeFixup( SP_RBTreeNode * node );

	void leftRotate( SP_RBTreeNode * root );
	void rightRotate( SP_RBTreeNode * root );

	SP_DictHandler * mHandler;
	SP_RBTreeNode * mNil;
	int mCount;
};

class SP_RBTreeVerifier {
public:
	static void verify( const SP_RBTreeNode * root, const SP_RBTreeNode * nil );

private:
	static void verifyParent( const SP_RBTreeNode * node, const SP_RBTreeNode * nil );
	static void verifyNodeColor( const SP_RBTreeNode * node, const SP_RBTreeNode * nil );
	static void verifyRootColor( const SP_RBTreeNode * node );
	static void verifyRedNode( const SP_RBTreeNode * node, const SP_RBTreeNode * nil );
	static void verifyPathBlackCount( const SP_RBTreeNode * node, const SP_RBTreeNode * nil );
	static void verifyPathBlackCountHelper( const SP_RBTreeNode * node,
		int blackCount, int * pathBlackCount, const SP_RBTreeNode * nil );

	SP_RBTreeVerifier();
	~SP_RBTreeVerifier();
};

#endif


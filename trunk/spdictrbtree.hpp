/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spdictrbtree_hpp__
#define __spdictrbtree_hpp__

#include "spdictionary.hpp"

// red-black tree
class SP_DictRBTreeNode {
public:
	SP_DictRBTreeNode( void * item );
	~SP_DictRBTreeNode();

	void setLeft( SP_DictRBTreeNode * left );
	SP_DictRBTreeNode * getLeft() const;

	void setRight( SP_DictRBTreeNode * right );
	SP_DictRBTreeNode * getRight() const;

	void setParent( SP_DictRBTreeNode * parent );
	SP_DictRBTreeNode * getParent() const;

	void setItem( void * item );
	const void * getItem() const;
	void * takeItem();

	enum { eRed, eBlack };
	void setColor( int color );
	int getColor() const;

private:
	void * mItem;	
	SP_DictRBTreeNode * mLeft, * mRight, * mParent;
	int mColor;
};

class SP_DictRBTreeIterator : public SP_DictIterator {
public:
	SP_DictRBTreeIterator( SP_DictRBTreeNode * node, SP_DictRBTreeNode * nil, int count );
	virtual ~SP_DictRBTreeIterator();

	// @return value that is stored in the RBTree, or null if reach the end
	virtual const void * getNext( int * level = 0 );

private:
	SP_DictRBTreeNode * mCurrent, * mNil;
	int mRemainCount;
	int mLevel;
};

class SP_DictRBTree : public SP_Dictionary {
public:
	SP_DictRBTree( SP_DictHandler * handler );
	virtual ~SP_DictRBTree();

	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

private:
	SP_DictRBTreeNode * searchNode( const void * key ) const;

	void reset();

	void insertFixup( SP_DictRBTreeNode * node );
	void removeFixup( SP_DictRBTreeNode * node );

	void leftRotate( SP_DictRBTreeNode * root );
	void rightRotate( SP_DictRBTreeNode * root );

	SP_DictHandler * mHandler;
	SP_DictRBTreeNode * mNil;
	int mCount;
};

class SP_DictRBTreeVerifier {
public:
	static void verify( const SP_DictRBTreeNode * root, const SP_DictRBTreeNode * nil );

private:
	static void verifyParent( const SP_DictRBTreeNode * node, const SP_DictRBTreeNode * nil );
	static void verifyNodeColor( const SP_DictRBTreeNode * node, const SP_DictRBTreeNode * nil );
	static void verifyRootColor( const SP_DictRBTreeNode * node );
	static void verifyRedNode( const SP_DictRBTreeNode * node, const SP_DictRBTreeNode * nil );
	static void verifyPathBlackCount( const SP_DictRBTreeNode * node, const SP_DictRBTreeNode * nil );
	static void verifyPathBlackCountHelper( const SP_DictRBTreeNode * node,
		int blackCount, int * pathBlackCount, const SP_DictRBTreeNode * nil );

	SP_DictRBTreeVerifier();
	~SP_DictRBTreeVerifier();
};

#endif


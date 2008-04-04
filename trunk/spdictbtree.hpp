/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef ___spdictbtree_hpp__
#define ___spdictbtree_hpp__

#include "spdictionary.hpp"

//Balanced Trees
class SP_DictBTreeNode {
public:
	SP_DictBTreeNode( int maxCount, SP_DictHandler * handler );
	~SP_DictBTreeNode();

	int getItemCount() const;
	void insertItem( int index, void * item );
	void appendItem( void * item );
	void * takeItem( int index );
	void * getItem( int index ) const;
	void updateItem( int index, void * item );

	int getNodeCount() const;
	void insertNode( int index, SP_DictBTreeNode * node );
	void appendNode( SP_DictBTreeNode * node );
	SP_DictBTreeNode * takeNode( int index );
	SP_DictBTreeNode * getNode( int index ) const;

	void setParent( SP_DictBTreeNode * parent );
	SP_DictBTreeNode * getParent() const;

	int needSplit() const;
	int needMerge() const;
	int canSplit() const;

	// @return >= 0 : found, -1 : not found
	int search( const void * item, int * insertPoint,
			int firstIndex = 0, int size = -1 ) const;

	int nodeIndex( const SP_DictBTreeNode * node ) const;

private:
	const int mMaxCount;
	SP_DictHandler * mHandler;

	SP_DictBTreeNode * mParent;

	int mNodeCount;
	SP_DictBTreeNode ** mNodeList;
	int mItemCount;
	void ** mItemList;
};

class SP_DictBTreeSearchResult {
public:
	SP_DictBTreeSearchResult();
	~SP_DictBTreeSearchResult();

	void setNode( SP_DictBTreeNode * node );
	SP_DictBTreeNode * getNode();

	void setIndex( int index );
	int getIndex();

	void setTag( int tag );
	int getTag();

private:
	SP_DictBTreeNode * mNode;
	int mIndex;
	int mTag;
};

class SP_DictBTreeIterator : public SP_DictIterator {
public:
	SP_DictBTreeIterator( const SP_DictBTreeNode * root, int count );
	virtual ~SP_DictBTreeIterator();

	// @return value that is stored in the BTree, or null if reach the end
	virtual const void * getNext( int * level = 0 );

private:
	const SP_DictBTreeNode * mCurrent;
	int mCurrIndex;
	int mLevel;

	int mRemainCount;
};

class SP_DictBTree : public SP_Dictionary {
public:
	SP_DictBTree( int rank, SP_DictHandler * handler );
	virtual ~SP_DictBTree();
	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

private:

	static void search( SP_DictBTreeNode * node, const void * key,
			SP_DictBTreeSearchResult * result );

	static SP_DictBTreeNode * split( int rank, SP_DictHandler * handler,
			SP_DictBTreeNode * node );

	static SP_DictBTreeNode * findLeaf( SP_DictBTreeNode * node );

	static SP_DictBTreeNode * merge( int rank, SP_DictBTreeNode * node );

	SP_DictBTreeNode * mRoot;
	SP_DictHandler * mHandler;

	const int mRank;
	int mCount;
};

#endif


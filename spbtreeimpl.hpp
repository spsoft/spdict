/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef ___btreeimpl_hpp__
#define ___btreeimpl_hpp__

#include "spdictionary.hpp"

//Balanced Trees
class SP_BTreeNode {
public:
	SP_BTreeNode( int maxCount, SP_DictHandler * handler );
	~SP_BTreeNode();

	int getItemCount() const;
	void insertItem( int index, void * item );
	void appendItem( void * item );
	void * takeItem( int index );
	void * getItem( int index ) const;
	void updateItem( int index, void * item );

	int getNodeCount() const;
	void insertNode( int index, SP_BTreeNode * node );
	void appendNode( SP_BTreeNode * node );
	SP_BTreeNode * takeNode( int index );
	SP_BTreeNode * getNode( int index ) const;

	void setParent( SP_BTreeNode * parent );
	SP_BTreeNode * getParent() const;

	int needSplit() const;
	int needMerge() const;
	int canSplit() const;

	// @return >= 0 : found, -1 : not found
	int search( const void * item, int * insertPoint,
			int firstIndex = 0, int size = -1 ) const;

	int nodeIndex( const SP_BTreeNode * node ) const;

private:
	const int mMaxCount;
	SP_DictHandler * mHandler;

	SP_BTreeNode * mParent;

	int mNodeCount;
	SP_BTreeNode ** mNodeList;
	int mItemCount;
	void ** mItemList;
};

class SP_BTreeSearchResult {
public:
	SP_BTreeSearchResult();
	~SP_BTreeSearchResult();

	void setNode( SP_BTreeNode * node );
	SP_BTreeNode * getNode();

	void setIndex( int index );
	int getIndex();

	void setTag( int tag );
	int getTag();

private:
	SP_BTreeNode * mNode;
	int mIndex;
	int mTag;
};

class SP_BTreeIterator : public SP_DictIterator {
public:
	SP_BTreeIterator( const SP_BTreeNode * root, int count );
	virtual ~SP_BTreeIterator();

	// @return value that is stored in the BTree, or null if reach the end
	virtual const void * getNext( int * level = 0 );

private:
	const SP_BTreeNode * mCurrent;
	int mCurrIndex;
	int mLevel;

	int mRemainCount;
};

class SP_BTreeImpl : public SP_Dictionary {
public:
	SP_BTreeImpl( int rank, SP_DictHandler * handler );
	virtual ~SP_BTreeImpl();
	virtual int insert( void * item );
	virtual const void * search( const void * key ) const;
	virtual void * remove( const void * key );
	virtual int getCount() const;
	virtual SP_DictIterator * getIterator() const;

private:

	static void search( SP_BTreeNode * node, const void * key,
			SP_BTreeSearchResult * result );

	static SP_BTreeNode * split( int rank, SP_DictHandler * handler,
			SP_BTreeNode * node );

	static SP_BTreeNode * findLeaf( SP_BTreeNode * node );

	static SP_BTreeNode * merge( int rank, SP_BTreeNode * node );

	SP_BTreeNode * mRoot;
	SP_DictHandler * mHandler;

	const int mRank;
	int mCount;
};

#endif


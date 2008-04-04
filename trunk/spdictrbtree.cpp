/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <assert.h>

#include "spdictrbtree.hpp"

//===========================================================================

SP_DictRBTreeNode :: SP_DictRBTreeNode( void * item )
{
	mItem = item;
	mLeft = mRight = mParent = NULL;
	mColor = eRed;
}

SP_DictRBTreeNode :: ~SP_DictRBTreeNode()
{
}

void SP_DictRBTreeNode :: setLeft( SP_DictRBTreeNode * left )
{
	mLeft = left;
	if( NULL != mLeft ) mLeft->setParent( this );
}

SP_DictRBTreeNode * SP_DictRBTreeNode :: getLeft() const
{
	return mLeft;
}

void SP_DictRBTreeNode :: setRight( SP_DictRBTreeNode * right )
{
	mRight = right;
	if( NULL != mRight ) mRight->setParent( this );
}

SP_DictRBTreeNode * SP_DictRBTreeNode :: getRight() const
{
	return mRight;
}

void SP_DictRBTreeNode :: setParent( SP_DictRBTreeNode * parent )
{
	mParent = parent;
}

SP_DictRBTreeNode * SP_DictRBTreeNode :: getParent() const
{
	return mParent;
}

void SP_DictRBTreeNode :: setItem( void * item )
{
	mItem = item;
}

const void * SP_DictRBTreeNode :: getItem() const
{
	return mItem;
}

void * SP_DictRBTreeNode :: takeItem()
{
	void * item = mItem;
	mItem = NULL;

	return item;
}

void SP_DictRBTreeNode :: setColor( int color )
{
	mColor = color;
}

int SP_DictRBTreeNode :: getColor() const
{
	return mColor;
}

//===========================================================================

SP_DictRBTreeIterator :: SP_DictRBTreeIterator( SP_DictRBTreeNode * node, SP_DictRBTreeNode * nil, int count )
{
	mRemainCount = count;

	mLevel = 0;

	mNil = nil;
	mCurrent = node;
	for( ; mNil != mCurrent->getLeft(); ) {
		mCurrent = mCurrent->getLeft();
		mLevel++;
	}
}

SP_DictRBTreeIterator :: ~SP_DictRBTreeIterator()
{
}

const void * SP_DictRBTreeIterator :: getNext( int * level )
{
	if( mNil == mCurrent ) return NULL;

	const void * item = mCurrent->getItem();
	if( NULL != level ) {
		*level = mLevel;
		//if( SP_DictRBTreeNode::eBlack == mCurrent->getColor() ) *level += 256;
	}

	// get next node
	if( mNil != mCurrent->getRight() ) {
		mCurrent = mCurrent->getRight();
		mLevel++;

		for( ; mNil != mCurrent->getLeft(); ) {
			mCurrent = mCurrent->getLeft();
			mLevel++;
		}
	} else {
		SP_DictRBTreeNode * iter = mCurrent;

		mCurrent = mCurrent->getParent();	
		mLevel--;

		for( ; mNil != mCurrent && iter == mCurrent->getRight(); ) {
			iter = mCurrent;

			mCurrent = mCurrent->getParent();
			mLevel--;
		}
	}

	assert( mRemainCount-- >= 0 );

	return item;
}

//===========================================================================

SP_DictRBTree :: SP_DictRBTree( SP_DictHandler * handler )
{
	mHandler = handler;
	mCount = 0;

	mNil = new SP_DictRBTreeNode( NULL );
	mNil->setLeft( mNil );
	mNil->setRight( mNil );
	mNil->setColor( SP_DictRBTreeNode::eBlack );
}

SP_DictRBTree :: ~SP_DictRBTree()
{
	reset();
	delete mNil;

	delete mHandler;
}

void SP_DictRBTree :: reset()
{
	SP_DictRBTreeNode * iter = mNil->getRight();
	for( ; mNil != iter; ) {
		if( mNil != iter->getLeft() ) {
			iter = iter->getLeft();
		} else if( mNil != iter->getRight() ) {
			iter = iter->getRight();
		} else {
			SP_DictRBTreeNode * toDel = iter;
			iter = iter->getParent();
			if( toDel == iter->getLeft() ) {
				iter->setLeft( mNil );
			} else {
				iter->setRight( mNil );
			}
			mHandler->destroy( toDel->takeItem() );
			delete toDel;
		}
	}
}

SP_DictRBTreeNode * SP_DictRBTree :: searchNode( const void * key ) const
{
	SP_DictRBTreeNode * ret = mNil;

	SP_DictRBTreeNode * curr = mNil->getRight();
	for( ; mNil != curr && mNil == ret; ) {
		int cmpRet = mHandler->compare( key, curr->getItem() );
		if( cmpRet < 0 ) {
			curr = curr->getLeft();
		} else if( cmpRet > 0 ) {
			curr = curr->getRight();
		} else {
			ret = curr;
		}
	}

	return ret;
}

const void * SP_DictRBTree :: search( const void * key ) const
{
	SP_DictRBTreeNode * node = searchNode( key );

	return mNil != node ? node->getItem() : NULL;
}

void SP_DictRBTree :: leftRotate( SP_DictRBTreeNode * root )
{
	SP_DictRBTreeNode * newRoot = root->getRight(), * parent = root->getParent();

	root->setRight( newRoot->getLeft() );
	newRoot->setLeft( root );

	if( root == parent->getLeft() ) {
		parent->setLeft( newRoot );
	} else {
		assert( root == parent->getRight() );
		parent->setRight( newRoot );
	}
}

void SP_DictRBTree :: rightRotate( SP_DictRBTreeNode * root )
{
	SP_DictRBTreeNode * newRoot = root->getLeft(), * parent = root->getParent();

	root->setLeft( newRoot->getRight() );
	newRoot->setRight( root );

	if( root == parent->getLeft() ) {
		parent->setLeft( newRoot );
	} else {
		assert( root == parent->getRight() );
		parent->setRight( newRoot );
	}
}

int SP_DictRBTree :: insert( void * item )
{
	int ret = 0;

	SP_DictRBTreeNode * parent = mNil;
	SP_DictRBTreeNode * curr = mNil->getRight();

	int cmpRet = 0;

	for( ; mNil != curr; ) {
		parent = curr;

		cmpRet = mHandler->compare( item, curr->getItem() );
		if( cmpRet < 0 ) {
			curr = curr->getLeft();
		} else if( cmpRet > 0 ) {
			curr = curr->getRight();
		} else {
			ret = 1;
			mHandler->destroy( curr->takeItem() );
			curr->setItem( item );
		}
	}

	if( 0 == ret ) {
		mCount++;

		SP_DictRBTreeNode * newNode = new SP_DictRBTreeNode( item );
		newNode->setLeft( mNil );
		newNode->setRight( mNil );

		if( mNil == parent ) {
			mNil->setRight( newNode );
		} else if( cmpRet < 0 ) {
			parent->setLeft( newNode );	
		} else {
			parent->setRight( newNode );
		}

		insertFixup( newNode );
	}

	//if( 0 == mCount % 10000 ) SP_DictRBTreeVerifier::verify( mNil->getRight(), mNil );

	return ret;
}

void SP_DictRBTree :: insertFixup( SP_DictRBTreeNode * node )
{
	for( ; SP_DictRBTreeNode::eRed == node->getParent()->getColor(); ) {
		SP_DictRBTreeNode * parent = node->getParent();
		SP_DictRBTreeNode * grandpa = parent->getParent();
		if( parent == grandpa->getLeft() ) {
			SP_DictRBTreeNode * uncle = grandpa->getRight();

			if( SP_DictRBTreeNode::eRed == uncle->getColor() ) {
				parent->setColor( SP_DictRBTreeNode::eBlack );
				uncle->setColor( SP_DictRBTreeNode::eBlack );
				grandpa->setColor( SP_DictRBTreeNode::eRed );
				node = grandpa;
			} else {
				if( node == parent->getRight() ) {
					node = parent;
					leftRotate( node );

					parent = node->getParent();
					grandpa = parent->getParent();
				}
				parent->setColor( SP_DictRBTreeNode::eBlack );
				grandpa->setColor( SP_DictRBTreeNode::eRed );
				rightRotate( grandpa );
			}
		} else {
			SP_DictRBTreeNode * uncle = grandpa->getLeft();

			if( SP_DictRBTreeNode::eRed == uncle->getColor() ) {
				parent->setColor( SP_DictRBTreeNode::eBlack );
				uncle->setColor( SP_DictRBTreeNode::eBlack );
				grandpa->setColor( SP_DictRBTreeNode::eRed );
				node = grandpa;
			} else {
				if( node == parent->getLeft() ) {
					node = parent;
					rightRotate( node );

					parent = node->getParent();
					grandpa = parent->getParent();
				}
				parent->setColor( SP_DictRBTreeNode::eBlack );
				grandpa->setColor( SP_DictRBTreeNode::eRed );
				leftRotate( grandpa );
			}
		}
	}

	mNil->setColor( SP_DictRBTreeNode::eBlack );
	mNil->getRight()->setColor( SP_DictRBTreeNode::eBlack );
}

void * SP_DictRBTree :: remove( const void * key )
{
	void * item = NULL;

	SP_DictRBTreeNode * node = searchNode( key );

	if( mNil != node ) {
		item = node->takeItem();

		SP_DictRBTreeNode * toDel = mNil;
		if( mNil == node->getLeft() || mNil == node->getRight() ) {
			toDel = node;
		} else {
			toDel = node->getRight();
			for( ; mNil != toDel->getLeft(); ) {
				toDel = toDel->getLeft();
			}
		}

		SP_DictRBTreeNode * child = mNil;
		if( mNil != toDel->getLeft() ) {
			child = toDel->getLeft();
		} else {
			child = toDel->getRight();
		}

		if( mNil == toDel->getParent() ) {
			mNil->setRight( child );
		} else {
			if( toDel == toDel->getParent()->getLeft() ) {
				toDel->getParent()->setLeft( child );
			} else {
				toDel->getParent()->setRight( child );
			}
		}

		if( toDel != node ) {
			node->setItem( toDel->takeItem() );
		}

		if( SP_DictRBTreeNode::eBlack == toDel->getColor() ) {
			removeFixup( child );
		}

		delete toDel;
		mCount--;
	}

	//if( 0 == mCount % 10000 ) SP_DictRBTreeVerifier::verify( mNil->getRight(), mNil );

	return item;
}

void SP_DictRBTree :: removeFixup( SP_DictRBTreeNode * node )
{
	for( ; node != mNil->getRight() && SP_DictRBTreeNode::eBlack == node->getColor(); ) {
		SP_DictRBTreeNode * parent = node->getParent();

		if( parent->getLeft() == node ) {
			SP_DictRBTreeNode * sister = parent->getRight();

			if( SP_DictRBTreeNode::eRed == sister->getColor() ) {
				sister->setColor( SP_DictRBTreeNode::eBlack );
				parent->setColor( SP_DictRBTreeNode::eRed );
				leftRotate( parent );
				sister = parent->getRight();
			}

			if( SP_DictRBTreeNode::eBlack == sister->getLeft()->getColor()
					&& SP_DictRBTreeNode::eBlack == sister->getRight()->getColor() ) {
				sister->setColor( SP_DictRBTreeNode::eRed );
				node = parent;
			} else {
				if( SP_DictRBTreeNode::eBlack == sister->getRight()->getColor() ) {
					sister->getLeft()->setColor( SP_DictRBTreeNode::eBlack );	
					sister->setColor( SP_DictRBTreeNode::eRed );
					rightRotate( sister );
					sister = parent->getRight();
				}
				sister->setColor( parent->getColor() );
				parent->setColor( SP_DictRBTreeNode::eBlack );
				sister->getRight()->setColor( SP_DictRBTreeNode::eBlack );
				leftRotate( parent );

				node = mNil->getRight();
			}
		} else {
			SP_DictRBTreeNode * sister = parent->getLeft();

			if( SP_DictRBTreeNode::eRed == sister->getColor() ) {
				sister->setColor( SP_DictRBTreeNode::eBlack );
				parent->setColor( SP_DictRBTreeNode::eRed );
				rightRotate( parent );
				sister = parent->getLeft();
			}

			if( SP_DictRBTreeNode::eBlack == sister->getLeft()->getColor()
					&& SP_DictRBTreeNode::eBlack == sister->getRight()->getColor() ) {
				sister->setColor( SP_DictRBTreeNode::eRed );
				node = parent;
			} else {
				if( SP_DictRBTreeNode::eBlack == sister->getLeft()->getColor() ) {
					sister->getRight()->setColor( SP_DictRBTreeNode::eBlack );	
					sister->setColor( SP_DictRBTreeNode::eRed );
					leftRotate( sister );
					sister = parent->getLeft();
				}
				sister->setColor( parent->getColor() );
				parent->setColor( SP_DictRBTreeNode::eBlack );
				sister->getLeft()->setColor( SP_DictRBTreeNode::eBlack );
				rightRotate( parent );

				node = mNil->getRight();
			}
		}
	}

	node->setColor( SP_DictRBTreeNode::eBlack );

	mNil->setColor( SP_DictRBTreeNode::eBlack );
	mNil->getRight()->setColor( SP_DictRBTreeNode::eBlack );
}

int SP_DictRBTree :: getCount() const
{
	return mCount;
}

SP_DictIterator * SP_DictRBTree :: getIterator() const
{
	return new SP_DictRBTreeIterator( mNil->getRight(), mNil, getCount() );
}

//===========================================================================

void SP_DictRBTreeVerifier :: verify( const SP_DictRBTreeNode * root, const SP_DictRBTreeNode * nil )
{
	verifyParent( root, nil );
	verifyNodeColor( root, nil );
	verifyRootColor( root );
	verifyRedNode( root, nil );
	verifyPathBlackCount( root, nil );
}

void SP_DictRBTreeVerifier :: verifyParent( const SP_DictRBTreeNode * node, const SP_DictRBTreeNode * nil )
{
	if( nil != node ) {
		if( nil != node->getLeft() ) {
			assert( node->getLeft()->getParent() == node );
		}
		if( nil != node->getRight() ) {
			assert( node->getRight()->getParent() == node );
		}

		verifyParent( node->getLeft(), nil );
		verifyParent( node->getRight(), nil );
	}
}

void SP_DictRBTreeVerifier :: verifyNodeColor( const SP_DictRBTreeNode * node, const SP_DictRBTreeNode * nil )
{
	assert( SP_DictRBTreeNode::eRed == node->getColor()
		||  SP_DictRBTreeNode::eBlack == node->getColor() );

	if( nil != node ) {
		verifyNodeColor( node->getLeft(), nil );
		verifyNodeColor( node->getRight(), nil );
	}
}

void SP_DictRBTreeVerifier :: verifyRootColor( const SP_DictRBTreeNode * node )
{
	assert( SP_DictRBTreeNode::eBlack == node->getColor() );
}

void SP_DictRBTreeVerifier :: verifyRedNode( const SP_DictRBTreeNode * node, const SP_DictRBTreeNode * nil )
{
	if( SP_DictRBTreeNode::eRed == node->getColor() ) {
		assert( SP_DictRBTreeNode::eBlack == node->getLeft()->getColor() );
		assert( SP_DictRBTreeNode::eBlack == node->getRight()->getColor() );
		assert( SP_DictRBTreeNode::eBlack == node->getParent()->getColor() );
	}

	if( nil != node ) {
		verifyRedNode( node->getLeft(), nil );
		verifyRedNode( node->getRight(), nil );
	}
}

void SP_DictRBTreeVerifier :: verifyPathBlackCount( const SP_DictRBTreeNode * node,
		const SP_DictRBTreeNode * nil )
{
	int pathBlackCount = -1;
	verifyPathBlackCountHelper( node, 0, &pathBlackCount, nil );
}

void SP_DictRBTreeVerifier :: verifyPathBlackCountHelper( const SP_DictRBTreeNode * node,
		int blackCount, int * pathBlackCount, const SP_DictRBTreeNode * nil )
{
	if( SP_DictRBTreeNode::eBlack == node->getColor() ) {
		blackCount++;
	}

	if( nil == node ) {
		if( * pathBlackCount == -1 ) {
			* pathBlackCount = blackCount;
		} else {
			assert( * pathBlackCount == blackCount );
		}
		return;
	}

	verifyPathBlackCountHelper( node->getLeft(), blackCount, pathBlackCount, nil );
	verifyPathBlackCountHelper( node->getRight(), blackCount, pathBlackCount, nil );
}


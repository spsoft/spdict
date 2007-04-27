/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <assert.h>

#include "sprbtreeimpl.hpp"

//===========================================================================

SP_RBTreeNode :: SP_RBTreeNode( void * item )
{
	mItem = item;
	mLeft = mRight = mParent = NULL;
	mColor = eRed;
}

SP_RBTreeNode :: ~SP_RBTreeNode()
{
}

void SP_RBTreeNode :: setLeft( SP_RBTreeNode * left )
{
	mLeft = left;
	if( NULL != mLeft ) mLeft->setParent( this );
}

SP_RBTreeNode * SP_RBTreeNode :: getLeft() const
{
	return mLeft;
}

void SP_RBTreeNode :: setRight( SP_RBTreeNode * right )
{
	mRight = right;
	if( NULL != mRight ) mRight->setParent( this );
}

SP_RBTreeNode * SP_RBTreeNode :: getRight() const
{
	return mRight;
}

void SP_RBTreeNode :: setParent( SP_RBTreeNode * parent )
{
	mParent = parent;
}

SP_RBTreeNode * SP_RBTreeNode :: getParent() const
{
	return mParent;
}

void SP_RBTreeNode :: setItem( void * item )
{
	mItem = item;
}

const void * SP_RBTreeNode :: getItem() const
{
	return mItem;
}

void * SP_RBTreeNode :: takeItem()
{
	void * item = mItem;
	mItem = NULL;

	return item;
}

void SP_RBTreeNode :: setColor( int color )
{
	mColor = color;
}

int SP_RBTreeNode :: getColor() const
{
	return mColor;
}

//===========================================================================

SP_RBTreeIterator :: SP_RBTreeIterator( SP_RBTreeNode * node, SP_RBTreeNode * nil, int count )
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

SP_RBTreeIterator :: ~SP_RBTreeIterator()
{
}

const void * SP_RBTreeIterator :: getNext( int * level )
{
	if( mNil == mCurrent ) return NULL;

	const void * item = mCurrent->getItem();
	if( NULL != level ) {
		*level = mLevel;
		//if( SP_RBTreeNode::eBlack == mCurrent->getColor() ) *level += 256;
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
		SP_RBTreeNode * iter = mCurrent;

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

SP_RBTreeImpl :: SP_RBTreeImpl( SP_DictHandler * handler )
{
	mHandler = handler;
	mCount = 0;

	mNil = new SP_RBTreeNode( NULL );
	mNil->setLeft( mNil );
	mNil->setRight( mNil );
	mNil->setColor( SP_RBTreeNode::eBlack );
}

SP_RBTreeImpl :: ~SP_RBTreeImpl()
{
	reset();
	delete mNil;

	delete mHandler;
}

void SP_RBTreeImpl :: reset()
{
	SP_RBTreeNode * iter = mNil->getRight();
	for( ; mNil != iter; ) {
		if( mNil != iter->getLeft() ) {
			iter = iter->getLeft();
		} else if( mNil != iter->getRight() ) {
			iter = iter->getRight();
		} else {
			SP_RBTreeNode * toDel = iter;
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

SP_RBTreeNode * SP_RBTreeImpl :: searchNode( const void * key ) const
{
	SP_RBTreeNode * ret = mNil;

	SP_RBTreeNode * curr = mNil->getRight();
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

const void * SP_RBTreeImpl :: search( const void * key ) const
{
	SP_RBTreeNode * node = searchNode( key );

	return mNil != node ? node->getItem() : NULL;
}

void SP_RBTreeImpl :: leftRotate( SP_RBTreeNode * root )
{
	SP_RBTreeNode * newRoot = root->getRight(), * parent = root->getParent();

	root->setRight( newRoot->getLeft() );
	newRoot->setLeft( root );

	if( root == parent->getLeft() ) {
		parent->setLeft( newRoot );
	} else {
		assert( root == parent->getRight() );
		parent->setRight( newRoot );
	}
}

void SP_RBTreeImpl :: rightRotate( SP_RBTreeNode * root )
{
	SP_RBTreeNode * newRoot = root->getLeft(), * parent = root->getParent();

	root->setLeft( newRoot->getRight() );
	newRoot->setRight( root );

	if( root == parent->getLeft() ) {
		parent->setLeft( newRoot );
	} else {
		assert( root == parent->getRight() );
		parent->setRight( newRoot );
	}
}

int SP_RBTreeImpl :: insert( void * item )
{
	int ret = 0;

	SP_RBTreeNode * parent = mNil;
	SP_RBTreeNode * curr = mNil->getRight();

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

		SP_RBTreeNode * newNode = new SP_RBTreeNode( item );
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

	//if( 0 == mCount % 10000 ) SP_RBTreeVerifier::verify( mNil->getRight(), mNil );

	return ret;
}

void SP_RBTreeImpl :: insertFixup( SP_RBTreeNode * node )
{
	for( ; SP_RBTreeNode::eRed == node->getParent()->getColor(); ) {
		SP_RBTreeNode * parent = node->getParent();
		SP_RBTreeNode * grandpa = parent->getParent();
		if( parent == grandpa->getLeft() ) {
			SP_RBTreeNode * uncle = grandpa->getRight();

			if( SP_RBTreeNode::eRed == uncle->getColor() ) {
				parent->setColor( SP_RBTreeNode::eBlack );
				uncle->setColor( SP_RBTreeNode::eBlack );
				grandpa->setColor( SP_RBTreeNode::eRed );
				node = grandpa;
			} else {
				if( node == parent->getRight() ) {
					node = parent;
					leftRotate( node );

					parent = node->getParent();
					grandpa = parent->getParent();
				}
				parent->setColor( SP_RBTreeNode::eBlack );
				grandpa->setColor( SP_RBTreeNode::eRed );
				rightRotate( grandpa );
			}
		} else {
			SP_RBTreeNode * uncle = grandpa->getLeft();

			if( SP_RBTreeNode::eRed == uncle->getColor() ) {
				parent->setColor( SP_RBTreeNode::eBlack );
				uncle->setColor( SP_RBTreeNode::eBlack );
				grandpa->setColor( SP_RBTreeNode::eRed );
				node = grandpa;
			} else {
				if( node == parent->getLeft() ) {
					node = parent;
					rightRotate( node );

					parent = node->getParent();
					grandpa = parent->getParent();
				}
				parent->setColor( SP_RBTreeNode::eBlack );
				grandpa->setColor( SP_RBTreeNode::eRed );
				leftRotate( grandpa );
			}
		}
	}

	mNil->setColor( SP_RBTreeNode::eBlack );
	mNil->getRight()->setColor( SP_RBTreeNode::eBlack );
}

void * SP_RBTreeImpl :: remove( const void * key )
{
	void * item = NULL;

	SP_RBTreeNode * node = searchNode( key );

	if( mNil != node ) {
		item = node->takeItem();

		SP_RBTreeNode * toDel = mNil;
		if( mNil == node->getLeft() || mNil == node->getRight() ) {
			toDel = node;
		} else {
			toDel = node->getRight();
			for( ; mNil != toDel->getLeft(); ) {
				toDel = toDel->getLeft();
			}
		}

		SP_RBTreeNode * child = mNil;
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

		if( SP_RBTreeNode::eBlack == toDel->getColor() ) {
			removeFixup( child );
		}

		delete toDel;
		mCount--;
	}

	//if( 0 == mCount % 10000 ) SP_RBTreeVerifier::verify( mNil->getRight(), mNil );

	return item;
}

void SP_RBTreeImpl :: removeFixup( SP_RBTreeNode * node )
{
	for( ; node != mNil->getRight() && SP_RBTreeNode::eBlack == node->getColor(); ) {
		SP_RBTreeNode * parent = node->getParent();

		if( parent->getLeft() == node ) {
			SP_RBTreeNode * sister = parent->getRight();

			if( SP_RBTreeNode::eRed == sister->getColor() ) {
				sister->setColor( SP_RBTreeNode::eBlack );
				parent->setColor( SP_RBTreeNode::eRed );
				leftRotate( parent );
				sister = parent->getRight();
			}

			if( SP_RBTreeNode::eBlack == sister->getLeft()->getColor()
					&& SP_RBTreeNode::eBlack == sister->getRight()->getColor() ) {
				sister->setColor( SP_RBTreeNode::eRed );
				node = parent;
			} else {
				if( SP_RBTreeNode::eBlack == sister->getRight()->getColor() ) {
					sister->getLeft()->setColor( SP_RBTreeNode::eBlack );	
					sister->setColor( SP_RBTreeNode::eRed );
					rightRotate( sister );
					sister = parent->getRight();
				}
				sister->setColor( parent->getColor() );
				parent->setColor( SP_RBTreeNode::eBlack );
				sister->getRight()->setColor( SP_RBTreeNode::eBlack );
				leftRotate( parent );

				node = mNil->getRight();
			}
		} else {
			SP_RBTreeNode * sister = parent->getLeft();

			if( SP_RBTreeNode::eRed == sister->getColor() ) {
				sister->setColor( SP_RBTreeNode::eBlack );
				parent->setColor( SP_RBTreeNode::eRed );
				rightRotate( parent );
				sister = parent->getLeft();
			}

			if( SP_RBTreeNode::eBlack == sister->getLeft()->getColor()
					&& SP_RBTreeNode::eBlack == sister->getRight()->getColor() ) {
				sister->setColor( SP_RBTreeNode::eRed );
				node = parent;
			} else {
				if( SP_RBTreeNode::eBlack == sister->getLeft()->getColor() ) {
					sister->getRight()->setColor( SP_RBTreeNode::eBlack );	
					sister->setColor( SP_RBTreeNode::eRed );
					leftRotate( sister );
					sister = parent->getLeft();
				}
				sister->setColor( parent->getColor() );
				parent->setColor( SP_RBTreeNode::eBlack );
				sister->getLeft()->setColor( SP_RBTreeNode::eBlack );
				rightRotate( parent );

				node = mNil->getRight();
			}
		}
	}

	node->setColor( SP_RBTreeNode::eBlack );

	mNil->setColor( SP_RBTreeNode::eBlack );
	mNil->getRight()->setColor( SP_RBTreeNode::eBlack );
}

int SP_RBTreeImpl :: getCount() const
{
	return mCount;
}

SP_DictIterator * SP_RBTreeImpl :: getIterator() const
{
	return new SP_RBTreeIterator( mNil->getRight(), mNil, getCount() );
}

//===========================================================================

void SP_RBTreeVerifier :: verify( const SP_RBTreeNode * root, const SP_RBTreeNode * nil )
{
	verifyParent( root, nil );
	verifyNodeColor( root, nil );
	verifyRootColor( root );
	verifyRedNode( root, nil );
	verifyPathBlackCount( root, nil );
}

void SP_RBTreeVerifier :: verifyParent( const SP_RBTreeNode * node, const SP_RBTreeNode * nil )
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

void SP_RBTreeVerifier :: verifyNodeColor( const SP_RBTreeNode * node, const SP_RBTreeNode * nil )
{
	assert( SP_RBTreeNode::eRed == node->getColor()
		||  SP_RBTreeNode::eBlack == node->getColor() );

	if( nil != node ) {
		verifyNodeColor( node->getLeft(), nil );
		verifyNodeColor( node->getRight(), nil );
	}
}

void SP_RBTreeVerifier :: verifyRootColor( const SP_RBTreeNode * node )
{
	assert( SP_RBTreeNode::eBlack == node->getColor() );
}

void SP_RBTreeVerifier :: verifyRedNode( const SP_RBTreeNode * node, const SP_RBTreeNode * nil )
{
	if( SP_RBTreeNode::eRed == node->getColor() ) {
		assert( SP_RBTreeNode::eBlack == node->getLeft()->getColor() );
		assert( SP_RBTreeNode::eBlack == node->getRight()->getColor() );
		assert( SP_RBTreeNode::eBlack == node->getParent()->getColor() );
	}

	if( nil != node ) {
		verifyRedNode( node->getLeft(), nil );
		verifyRedNode( node->getRight(), nil );
	}
}

void SP_RBTreeVerifier :: verifyPathBlackCount( const SP_RBTreeNode * node,
		const SP_RBTreeNode * nil )
{
	int pathBlackCount = -1;
	verifyPathBlackCountHelper( node, 0, &pathBlackCount, nil );
}

void SP_RBTreeVerifier :: verifyPathBlackCountHelper( const SP_RBTreeNode * node,
		int blackCount, int * pathBlackCount, const SP_RBTreeNode * nil )
{
	if( SP_RBTreeNode::eBlack == node->getColor() ) {
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


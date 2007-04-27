/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>

#include "spdictionary.hpp"

#include "spbtreeimpl.hpp"
#include "spslistimpl.hpp"
#include "sparrayimpl.hpp"
#include "spbstreeimpl.hpp"
#include "sprbtreeimpl.hpp"

//===========================================================================

SP_DictHandler :: ~SP_DictHandler()
{
}

//===========================================================================

SP_DictIterator :: ~SP_DictIterator()
{
}

//===========================================================================

SP_Dictionary :: ~SP_Dictionary()
{
}

SP_Dictionary * SP_Dictionary :: newBTree( int rank, SP_DictHandler * handler )
{
	return new SP_BTreeImpl( rank, handler );
}

SP_Dictionary * SP_Dictionary :: newSkipList( int maxLevel, SP_DictHandler * handler )
{
	return new SP_SkipListImpl( maxLevel, handler );
}

SP_Dictionary * SP_Dictionary :: newInstance( int type, SP_DictHandler * handler )
{
	if( eSkipList == type ) {
		return new SP_SkipListImpl( 128, handler );
	} else if( eBSTree == type ) {
		return new SP_BSTreeImpl( handler );
	} else if( eRBTree == type ) {
		return new SP_RBTreeImpl( handler );
	} else if( eSortedArray == type ) {
		return new SP_SortedArrayImpl( handler );
	} else {
		return new SP_BTreeImpl( 64, handler );
	}
}


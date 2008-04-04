/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>

#include "spdictionary.hpp"

#include "spdictbtree.hpp"
#include "spdictslist.hpp"
#include "spdictarray.hpp"
#include "spdictbstree.hpp"
#include "spdictrbtree.hpp"

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
	return new SP_DictBTree( rank, handler );
}

SP_Dictionary * SP_Dictionary :: newSkipList( int maxLevel, SP_DictHandler * handler )
{
	return new SP_DictSkipList( maxLevel, handler );
}

SP_Dictionary * SP_Dictionary :: newInstance( int type, SP_DictHandler * handler )
{
	if( eSkipList == type ) {
		return new SP_DictSkipList( 128, handler );
	} else if( eBSTree == type ) {
		return new SP_DictBSTree( handler );
	} else if( eRBTree == type ) {
		return new SP_DictRBTree( handler );
	} else if( eSortedArray == type ) {
		return new SP_DictSortedArray( handler );
	} else {
		return new SP_DictBTree( 64, handler );
	}
}


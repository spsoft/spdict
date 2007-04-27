/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spcache_hpp__
#define __spcache_hpp__

class SP_CacheHandler {
public:
	virtual ~SP_CacheHandler();

	virtual int compare( const void * item1, const void * item2 ) = 0;
	virtual void destroy( void * item ) = 0;
	virtual void onHit( const void * item, void * resultHolder ) = 0;
};

class SP_CacheStatistics {
public:
	virtual ~SP_CacheStatistics();

	virtual int getHits() const = 0;
	virtual int getAccesses() const = 0;
	virtual int getSize() const = 0;
};

class SP_Cache {
public:
	virtual ~SP_Cache();

	// @return 0 : insert ok, 1 : update ok
	virtual int put( void * itemm ) = 0;

	// @return 0 : no such key, 1 : found it
	virtual int get( const void * key, void * resultHolder ) = 0;

	// @return 0 : no such key, 1 : remove it
	virtual int remove( const void * key ) = 0;

	// caller need to delete the return object
	virtual SP_CacheStatistics * getStatistics() = 0;

	//===========================================================

	enum { eFIFO, eLRU };

	static SP_Cache * newInstance( int algo, int maxItems,
			SP_CacheHandler * handler );
};

#endif


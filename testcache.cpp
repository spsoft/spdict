/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "spdictcache.hpp"

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

#ifdef WIN32
#include <time.h>
#include <windows.h>

#define strcasecmp stricmp

int gettimeofday(struct timeval* tv, void * ) 
{
	union {
		long ns100;
		FILETIME ft;
	} now;

	GetSystemTimeAsFileTime (&now.ft);
	tv->tv_usec = (long) ((now.ns100 / 10L ) % 1000000L);
	tv->tv_sec = (long) ((now.ns100 - 116444736000000000L) / 10000000L);
	return (0);
}
#endif

class SP_Clock {
private:
	struct timeval mBornTime;
public:
	SP_Clock() {
		gettimeofday ( &mBornTime, NULL ); 
	}

	long getAge() {
		struct timeval now;
		gettimeofday ( &now, NULL ); 

		return (long)( 1000000.0 * ( now.tv_sec - mBornTime.tv_sec )
				+ ( now.tv_usec - mBornTime.tv_usec ) );
	}

	void print( const char * label ) {
		printf( "%s :\t\t%.6f (seconds)\n", label, getAge() / 1000000.0 );
	}
};

class SP_User {
public:
	SP_User( const char * name ) {
		mName = strdup( name );
	}

	~SP_User() {
		free( mName );
	}

	const char * getName() {
		return mName;
	}

private:
	char * mName;
};

class SP_UserCacheHandler : public SP_DictCacheHandler {
public:
	SP_UserCacheHandler() {}

	~SP_UserCacheHandler() {}

	int compare( const void * item1, const void * item2 ) {
		SP_User * user1 = (SP_User*)item1, * user2 = (SP_User*)item2;

		return strcmp( user1->getName(), user2->getName() );
	}

	void destroy( void * item ) {
		SP_User * user = (SP_User*)item;
		delete user;
	}

	void onHit( const void * item, void * resultHolder ) {
		SP_User * user = (SP_User*)item;
		strcpy( (char*)resultHolder, user->getName() );
	}
};

static char * randStr( char * buffer, int size )
{
	for( int i = 0; i < size - 1; i++ ) {
		int index = rand() % 26;
		buffer[ i ] = 'a' + index;
	}
	buffer[ size - 1 ] = '\0';

	return buffer;
}

int main( int argc, char * argv[] )
{
	int size = 256, count = 1000, algo = SP_DictCache::eFIFO;

#ifndef WIN32
	extern char *optarg;
	int c;
	while ( ( c = getopt ( argc, argv, "a:s:c:v" ) ) != EOF ) {
		switch ( c ) {
			case 'a':
				if( 0 == strcasecmp( "LRU", optarg ) ) algo = SP_DictCache::eLRU;
				break;
			case 's':
				size = atoi( optarg );
				break;
			case 'c':
				count = atoi( optarg );
				break;
			case 'v':
			case '?':
			default:
				printf( "%s -a <FIFO|LRU> -s <cache size> -c <count> [-v]\n", argv[0] );
				exit ( 0 );
		}
	}
#endif

	SP_Clock clock;

	SP_UserCacheHandler * handler = new SP_UserCacheHandler();
	SP_DictCache * cache = SP_DictCache::newInstance( algo, size, handler );

	char name[ 9 ] = { 0 };
	for( int i = 0; i < count; i++ ) {
		SP_User * user = new SP_User( randStr( name, sizeof( name ) ) );
		cache->put( user );
		assert( 0 != cache->get( user, name ) );

		if( 1 == ( i % 10 ) ) {
			SP_User temp( name );
			cache->erase( &temp );
			assert( 0 == cache->get( &temp, name ) );
		}
	}

	SP_DictCacheStatistics * stat = cache->getStatistics();

	printf( "Stat : accesses( %d ), hits( %d ), size( %d )\n",
			stat->getAccesses(), stat->getHits(), stat->getSize() );

	delete stat;

	delete cache;

	clock.print( "TotalTime" );

#ifdef WIN32
	printf( "\npress any key to exit ...\n" );
	getchar();
#endif

	return 0;
}


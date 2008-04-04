/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

#ifdef WIN32
#include <time.h>
#include <windows.h>
#endif

#include "spdictshmcache.hpp"

typedef struct tagUser {
	unsigned int mID;
	char mName[ 16 ];
} User_t;

class UserHandler : public SP_DictShmCacheHandler {
public:
	UserHandler() {
		mCurrBucket = -1;
	}

	~UserHandler() {}

	unsigned int hash( const void * item ) {
		User_t * user = (User_t*) item;

		return SP_DictShmCache::fnvHash( user->mName, strlen( user->mName ) );
	}

	int compare( const void * item1, const void * item2 ) {
		User_t * user1 = (User_t*) item1;
		User_t * user2 = (User_t*) item2;

		return strcmp( user1->mName, user2->mName );
	}

	void onHit( const void * item, void * resultHolder ) {
		memcpy( resultHolder, item, sizeof( User_t ) );
	}

	void onDumpHash( int bucket, const void * item ) {
		User_t * user = (User_t*)item;

		if( mCurrBucket != bucket ) printf( "\n%d : ", bucket );
		mCurrBucket = bucket;

		printf( "%s ", user->mName );
	}

	void onDumpEvict( time_t expTime, const void * item ) {
		User_t * user = (User_t*)item;

		printf( " %s[%li]\n", user->mName, expTime );
	}

private:
	int mCurrBucket;
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
	int size = 256, count = 10000, algo = SP_DictShmCache::eFIFO;

#ifndef WIN32
	extern char *optarg;
	int c;
	while ( ( c = getopt ( argc, argv, "a:s:c:v" ) ) != EOF ) {
		switch ( c ) {
			case 'a':
				if( 0 == strcasecmp( "LRU", optarg ) ) algo = SP_DictShmCache::eLRU;
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

	SP_DictShmCache cache( new UserHandler(), 1024, sizeof( User_t ) );

	const char * mapFile  = "testshmcache.map";

	int ret = cache.init( mapFile, 102400 );

	if( 0 == ret ) {
		printf( "Create file %s\n", mapFile );
	} else if ( 1 == ret ) {
		printf( "Reuse file %s\n", mapFile );
	} else {
		printf( "Fail to init file %s\n", mapFile );
		exit( 0 );
	}

	srand( time( NULL ) );

	for( int i = 0; i < count; i++ ) {
		User_t user;

		user.mID = rand();
		randStr( user.mName, sizeof( user.mName ) );

		if( -1 == cache.put( &user, time( NULL ) + 10 ) ) {
			printf( "\nout of memory on index #%d\n", i );
			break;
		}

		User_t result;

		assert( 0 != cache.get( &user, &result ) );

		if( 1 == ( i % 10 ) ) {
			printf( "#" );
			cache.erase( &user );
			assert( 0 == cache.get( &user, &result  ) );
		}
	}

	printf( "\n" );

	const SP_DictShmCacheStatistics * stat = cache.getStatistics();

	printf( "Stat : accesses( %d ), hits( %d ), size( %d )\n",
			stat->getAccesses(), stat->getHits(), stat->getSize() );

	delete stat;

#ifdef WIN32
	printf( "\npress any key to exit ...\n" );
	getchar();
#endif

	return 0;
}


/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

#include "spdictionary.hpp"

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
	SP_User( int number, char * name ) {
		strncpy( mName, name, sizeof( mName ) );
		mNumber = number;
	}

	~SP_User(){}

	const char * getName() {
		return mName;
	}

	int getNumber() {
		return mNumber;
	}

private:
	char mName[ 32 ];
	int mNumber;
};

class SP_UserHandler : public SP_DictHandler {
private:
	mutable int mCmpCount;

	int mShowCmpRet;

public:
	SP_UserHandler() {
		mCmpCount = 0;
		mShowCmpRet = 0;
	}

	virtual ~SP_UserHandler() {
		printf( "CmpCount=%d\n", mCmpCount );
	}

	void setShowCmpRet( int showCmpRet ) {
		mShowCmpRet = showCmpRet;
	}

	virtual int compare( const void * item1, const void * item2 ) const {
		mCmpCount++;
		SP_User * user1 = (SP_User*)item1, * user2 = (SP_User*)item2;
		int ret = strcmp( user1->getName(), user2->getName() );

		if( 0 != mShowCmpRet ) {
			printf( "%s, %s, %d\n", user1->getName(), user2->getName(), ret );
		}
		return ret;
	}

	virtual void destroy( void * item ) const {
		delete (SP_User*)item;
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

static void randTest( int type, int count )
{
	SP_Clock totalClock;

	SP_UserHandler * handler = new SP_UserHandler();
	SP_Dictionary * dictionary = SP_Dictionary::newInstance( type, handler );

	SP_User ** userList = (SP_User**)malloc( sizeof( void * ) * count );

	{
		SP_Clock clock;

		char name[ 9 ] = { 0 };
		for( int i = 0; i < count; i++ ) {
			if( 0 == ( i % 1000 ) ) printf( "#" );
			userList[i] = new SP_User( i, randStr( name, sizeof( name ) ) );
			if( NULL == dictionary->search( userList[i] ) ) {
				assert( 0 == dictionary->insert( userList[i] ) );
			} else {
				delete userList[i];
				userList[i] = NULL;
			}
		}

		printf( "\ninsert count = %d\n", dictionary->getCount() );
		clock.print( "InsertTime" );
	}

	{
		SP_Clock clock;

		for( int i = 0; i < count; i++ ) {
			if( NULL == userList[i] ) continue;
			SP_User * ret = (SP_User*)dictionary->search( userList[i] );
			if( NULL == ret ) {
				printf( "cannot found %i, %s\n", i, userList[i]->getName() );
			}
		}

		clock.print( "SearchTime" );
	}

	int iterCount = 0;

	{
		SP_Clock clock;

		SP_DictIterator * iter = dictionary->getIterator();
		const void * val = iter->getNext();
		const void * prev = NULL;

		for( ; NULL != val; iterCount++ ) {
			if( NULL != prev ) {
				assert( handler->compare( val, prev ) > 0 );
			}
			prev = val;
			val = iter->getNext();
		}

		delete iter;

		assert( iterCount == dictionary->getCount() );

		printf( "iterate count = %d\n", iterCount );
		clock.print( "IterateTime" );
	}

	{
		SP_Clock clock;

		for( int i = count - 1; i >= 0; i-- ) {
			if( NULL == userList[i] ) continue;
			SP_User * ret = (SP_User*)dictionary->search( userList[i] );
			if( NULL != ret ) {
				assert( NULL != ( ret = (SP_User*)dictionary->remove( userList[i] ) ) );
				delete ret;
			} else {
				printf( ">>>>>>>>>>>>>>>>remove %s>>>>>>>>>>>>>>>\n", userList[i]->getName() );
				handler->setShowCmpRet( 1 );
				ret = (SP_User*)dictionary->search( userList[i] );
				handler->setShowCmpRet( 0 );
				printf( "cannot found %s\n", userList[i]->getName() );
			}
			assert( -- iterCount == dictionary->getCount() );
		}

		assert( 0 == dictionary->getCount() );

		clock.print( "DeleteTime" );
	}

	free( userList );
	delete dictionary;

	totalClock.print( "TotalTime" );
}

static void usage( const char * program )
{
	printf( "%s [-t type] [-c count]\n", program );
	printf( "\t-t type :\n" );
	printf( "\t\t bst ( brinary search tree )\n" );
	printf( "\t\t rb ( red-black tree )\n" );
	printf( "\t\t bt ( balanced tree )\n" );
	printf( "\t\t sl ( skip list )\n" );
	printf( "\t\t sa ( sorted array )\n" );
	printf( "\t-c count, test how many items\n" );
	printf( "\n" );
}

int main( int argc, char * argv[] )
{
	const char * strType = "bt";
	int count = 100000;

#ifndef WIN32
	extern char *optarg ;
	int c ;
	while( ( c = getopt( argc, argv, "t:c:v" ) ) != EOF ) {
		switch ( c ) {
			case 't' :
				strType = optarg;
				break;
			case 'c' :
				count = atoi( optarg );
				break;
			case 'v' :
			default: usage( argv[0] ); exit( 0 ); break;
		}
	}
#endif

	int type = SP_Dictionary::eBTree;
	if( 0 == strcasecmp( strType, "bst" ) ) type = SP_Dictionary::eBSTree;
	if( 0 == strcasecmp( strType, "rb" ) ) type = SP_Dictionary::eRBTree;
	if( 0 == strcasecmp( strType, "bt" ) ) type = SP_Dictionary::eBTree;
	if( 0 == strcasecmp( strType, "sl" ) ) type = SP_Dictionary::eSkipList;
	if( 0 == strcasecmp( strType, "sa" ) ) type = SP_Dictionary::eSortedArray;
	if( SP_Dictionary::eBTree == type ) strType = "bt";

	printf( "type = %s, count = %d\n", strType, count );

	srand( time( NULL ) );

	randTest( type, count );

#ifdef WIN32
	printf( "\npress any key to exit ...\n" );
	getchar();
#endif

	return 0;
}


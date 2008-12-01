/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "spdictshmqueue.hpp"

typedef struct tagUser {
	unsigned int mID;
	char mName[ 16 ];
} User_t;

void pushProc( SP_DictShmQueue * queue )
{
	for( int i = 0; i < 100; i++ ) {
		User_t user;
		user.mID = i;

		queue->push( &user );

		if( 0 == ( i % 10 ) ) {
			printf( "push %d\n", i );
		}
	}
}

void popProc( SP_DictShmQueue * queue )
{
	for( int i = 0; i < 100; i++ ) {
		User_t user;

		queue->pop( &user );

		if( 0 == ( i % 10 ) ) {
			printf( "pop %d\n", i );
		}
	}
}

int main( int argc, char * argv[] )
{
	SP_DictShmQueue queue;

	queue.init( "shmqueue.map", 10, sizeof( User_t ) );

	int maxProc = 1;

	for( int i = 0; i < maxProc; i++ ) {
		if( 0 == fork() ) {
			popProc( &queue );
			exit( 0 );
		}
	}

	for( int i = 0; i < maxProc; i++ ) {
		if( 0 == fork() ) {
			pushProc( &queue );
			exit( 0 );
		}
	}

	for( ; wait( NULL ) > 0; ) {
		printf( "wait next child\n" );
	}

	return 0;
}


/*
 * Maroš Polák
 *
 * Some classical synchronizations problems from book Little Book of Semaphores by Allen B. Downey
 * compile:
 	gcc -Wall -pthread -o classical classical-problems.c
 	./classical [number-of-problems-example]
 	
	case 1:
		producer_consumer();
	case 2:
		readers_writers();
	case 3:
		
	case 4:
		
	case 5:
 */

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
/*
	int sem_init(sem_t *sem, int pshared, unsigned int value);
	The pshared argument indicates whether this semaphore is to be shared between the threads of a process, or between processes.
*/
#include <stdio.h>
#include <stdlib.h> // rand
#include <unistd.h> // sleep


#define POOL_SIZE 10
pthread_t t_pool[POOL_SIZE];
int counter = 0;

/* 4.1. Producer-consumer problem START */
/*
 *
 */
int 	items = 0;
sem_t 	s_mutex,
		s_producer,
		s_consumer;

void *_1worker_produce(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_consumer) == 0 ); // cakaj kym nepride nejaky konzumator

		sleep(1);
		items = (rand() % 5) + 1;
		printf("_1worker_consume: Thread %lu \E[32mincrement\E[0m items=%d\n", pthread_self(), items);

		assert( sem_post(&s_producer) == 0 );
	}

	pthread_exit((void*) 0);
}

void *_1worker_consume(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_mutex) == 0 );
		if( items == 0 )
		{
			assert( sem_post(&s_consumer) == 0 );
			assert( sem_wait(&s_producer) == 0 );
		}
		--items;
		printf("_1worker_consume: Thread %lu \E[31mdecrement\E[0m items=%d\n", pthread_self(), items);
		assert( sem_post(&s_mutex) == 0 );
	}

	pthread_exit((void*) 0);
}

void producer_consumer(void)
{
	int i,
		threads_count = 5;

	printf("%s Executing %s\n", __FILE__, __FUNCTION__);
	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_producer, 1, 0) == 0 );
	assert( sem_init(&s_consumer, 1, 0) == 0 );

	for( i = 0; i < threads_count; ++i )
	{
		if( i > 1 )
			assert( pthread_create(&t_pool[i], NULL, _1worker_consume, NULL) == 0 );
		else
			assert( pthread_create(&t_pool[i], NULL, _1worker_produce, NULL) == 0 );
	}

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_producer) == 0 );
	assert( sem_destroy(&s_consumer) == 0 );
}
/* 4.1. Producer-consumer problem END */



/* 4.2. Readers-writers problem START */
/*
 *
 */
sem_t 	s_multiplex,
		s_turnstile,
		s_room_empty;
int 	data = 0;

void *_2worker_writer(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_turnstile) == 0 );

		assert( sem_wait(&s_room_empty) == 0 );

		// CS update
		data = rand() % 100;
		printf("%s: Trhread %lu update data=%d\n", __FUNCTION__, pthread_self(), data);

		assert( sem_post(&s_turnstile) == 0 );
		assert( sem_post(&s_room_empty) == 0 );

		sleep((rand() % 1));
	}

	pthread_exit((void*) 0);
}

void *_2worker_reader(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_multiplex) == 0 );

		assert( sem_wait(&s_turnstile) == 0 );
		assert( sem_post(&s_turnstile) == 0 );

		// LightSwitch lock
		assert( sem_wait(&s_mutex) == 0 );
		++counter; // pocet citatelov
		if( counter == 1 )
			assert( sem_wait(&s_room_empty) == 0 );
		assert( sem_post(&s_mutex) == 0 );

		// CS read
		printf("%s: Trhread %lu read data=%d\n", __FUNCTION__, pthread_self(), data);		

		// LightSwitch unlock
		assert( sem_wait(&s_mutex) == 0 );
		--counter; // pocet citatelov
		if( counter == 0 )
			assert( sem_post(&s_room_empty) == 0 );
		assert( sem_post(&s_mutex) == 0 );

		assert( sem_post(&s_multiplex) == 0 );
	}

	pthread_exit((void*) 0);
}

void readers_writers(void)
{
	int i,
		threads_count = 5,
		writers_count = 1,
		n = threads_count - writers_count;

	threads_count += 3; // x vlakien musi cakat pred multiplexom

	printf("%s Executing %s\n", __FILE__, __FUNCTION__);
	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_multiplex, 1, n) == 0 );
	assert( sem_init(&s_turnstile, 1, 1) == 0 );
	assert( sem_init(&s_room_empty, 1, 1) == 0 );

	for( i = 0; i < threads_count; ++i )
	{
		if( i > writers_count )
			assert( pthread_create(&t_pool[i], NULL, _2worker_reader, NULL) == 0 );
		else
			assert( pthread_create(&t_pool[i], NULL, _2worker_writer, NULL) == 0 );
	}

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_producer) == 0 );
	assert( sem_destroy(&s_consumer) == 0 );

}
/* 4.2. Readers-writers problem END */

int main(int argc, char **argv)
{
	int u_choice = 1;

	if( argc > 1 )
		sscanf(argv[1], "%d", &u_choice);
	
	switch( u_choice )
	{
		case 1:
			producer_consumer();
			break;
		case 2:
			readers_writers();
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		default:
			printf("Peu si?\n");
	}
	return 0;
}
/*
 * Maroš Polák
 *
 * Some basic synchronizations patterns from book Little Book of Semaphores by Allen B. Downey
 * compile:
 	gcc -Wall -pthread -o basic basic-patterns.c
 	./basic [number-of-pattern-example]

 	case 1:
		signaling();
	case 2:
		rendezvous();
	case 3:
		mutex();
	case 4:
		multiplex();
	case 5:
		barrier();
	case 6:
		queue();
 */

#include <assert.h>
#include <pthread.h>
/*
 *	int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
 */
#include <semaphore.h>
/*
 *	int sem_init(sem_t *sem, int pshared, unsigned int value);
 *	The pshared argument indicates whether this semaphore is to be shared between the threads of a process, or between processes.
 *
 *  int sem_destroy(sem_t *sem);
 */
#include <stdio.h>
#include <stdlib.h> // rand
#include <unistd.h> // sleep


#define POOL_SIZE 10
pthread_t t_pool[POOL_SIZE];
int counter = 0;

/* 3.1 Signaling START */
/*
 * Vlakno sa nezacne vykonavat kym ho ine vlakno k tomu nevyzve
 */

sem_t s_semaphore;

void *_1worker_a(void *arg)
{
	printf("%s: Thread %lu say Hello\n", __FUNCTION__, pthread_self());
	assert( sem_post(&s_semaphore) == 0 );

	pthread_exit((void*) 0);
}

void *_1worker_b(void *arg)
{
	assert( sem_wait(&s_semaphore) == 0 );
	printf("%s: Thread %lu say Hello\n", __FUNCTION__, pthread_self());

	pthread_exit((void*) 0);
}

void signaling(void)
{
	int i,
		threads_count = 2;
	assert( sem_init(&s_semaphore, 1, 0) == 0 );

	for( i = 0; i < threads_count; ++i )
	{
		if( i % 2 )
			assert( pthread_create(&t_pool[i], NULL, _1worker_a, NULL) == 0 );
		else
			assert( pthread_create(&t_pool[i], NULL, _1worker_b, NULL) == 0 );
	}

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	assert( sem_destroy(&s_semaphore) == 0 );
}
/* 3.1 Signaling END */



/* 3.3 Rendezvous START */
/*
 * Vlakna sa navzajom cakaju a az ked su obe dosutpne tak sa pokracuje vo vykonavani kodu
 */
sem_t s_a_arrived;
sem_t s_b_arrived;

void *_3worker_a(void *arg)
{
	printf("%s: Thread %lu say Hello 1\n", __FUNCTION__, pthread_self());
	assert( sem_post(&s_a_arrived) == 0 );
	assert( sem_wait(&s_b_arrived) == 0 );
	printf("%s: Thread %lu say Hello 2\n", __FUNCTION__, pthread_self());

	pthread_exit((void*) 0);
}

void *_3worker_b(void *arg)
{
	printf("%s: Thread %lu say Hello 1\n", __FUNCTION__, pthread_self());
	assert( sem_post(&s_b_arrived) == 0 );
	assert( sem_wait(&s_a_arrived) == 0 );
	printf("%s: Thread %lu say Hello 2\n", __FUNCTION__, pthread_self());

	pthread_exit((void*) 0);
}

void rendezvous(void)
{
	int i,
		threads_count = 2;
	assert( sem_init(&s_a_arrived, 1, 0) == 0 );
	assert( sem_init(&s_b_arrived, 1, 0) == 0 );

	for( i = 0; i < threads_count; ++i )
	{
		if( i )			
			assert( pthread_create(&t_pool[i], NULL, _3worker_a, NULL) == 0 );
		else
			assert( pthread_create(&t_pool[i], NULL, _3worker_b, NULL) == 0 );
	}

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	assert( sem_destroy(&s_a_arrived) == 0 );
	assert( sem_destroy(&s_b_arrived) == 0 );
}
/* 3.3 Rendezvous END */



/* 3.4 Mutex START */
/*
 * Iba jedno vlakno moze byt v kriticke oblasti - vzajomne vylucenie(MUTual EXclusion)
 */
sem_t s_mutex;

void *_4worker(void *arg)
{
	assert( sem_wait(&s_mutex) == 0 );
	//CS critical section
	counter++;
	printf("%s: Thread %lu increments counter\n", __FUNCTION__, pthread_self());
	assert( sem_post(&s_mutex) == 0 );

	pthread_exit((void*) 0);
}

void mutex(void)
{
	int i,
		threads_count = POOL_SIZE;
	assert( sem_init(&s_mutex, 1, 1) == 0 );

	for( i = 0; i < threads_count; ++i )
		assert( pthread_create(&t_pool[i], NULL, _4worker, NULL) == 0 );

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	printf("Value of counter=%d\n", counter);
	assert( sem_destroy(&s_mutex) == 0 );
}
/* 3.4 Mutex END */



/* 3.5 Multiplex START */
/*
 * V kritickej oblasti moze byt maximalne N vlakien. Sauna, vyrivka, vytah, dopravny prostriedok, ...
 */
sem_t s_multiplex;

void _5worker_cs(void)
{
	assert( sem_wait(&s_mutex) == 0 );
	++counter;
	printf("%s: Thread %lu \e[32mJOINed\E[0m thread party, #thread=%d\n", __FUNCTION__, pthread_self(), counter);
	assert( sem_post(&s_mutex) == 0 );

	sleep(rand() % 2 + 1);

	assert( sem_wait(&s_mutex) == 0 );
	--counter;
	printf("%s: Thread %lu \E[31mLEAVEs\E[0m thread party, #thread=%d\n", __FUNCTION__, pthread_self(), counter);
	assert( sem_post(&s_mutex) == 0 );
}

void *_5worker(void *arg)
{
	assert( sem_wait(&s_multiplex) == 0 );
	//CS critical section
	_5worker_cs();
	assert( sem_post(&s_multiplex) == 0 );

	pthread_exit((void*) 0);
}

void multiplex(void)
{
	int i,
		threads_count = POOL_SIZE,
		multiplex_size = 5;

	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_multiplex, 1, multiplex_size) == 0 );

	for( i = 0; i < threads_count; ++i )
		assert( pthread_create(&t_pool[i], NULL, _5worker, NULL) == 0 );

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_multiplex) == 0 );
}
/* 3.5 Multiplex END */



/* 3.6 Barrier START */
/*
 * rozsirene rande(cakaju na seba viac ako 2 vlakna): Vlakna sa cakaju az kym nevykonaju danu ulohu vsetky
 */
sem_t s_barrier;
sem_t s_barrier_2;

void *_6worker(void *arg)
{
	assert( sem_wait(&s_mutex) == 0 );
	{
		++counter;
		if( counter == *(int*)arg )
		{
			printf("\n");
			assert( sem_post(&s_barrier) == 0 );
		}
	}
	assert( sem_post(&s_mutex) == 0 );

	// turnsitle - turniket moze prejst len jeden
	assert( sem_wait(&s_barrier) == 0 );
	printf("%s: Thread %lu statement 1\n", __FUNCTION__, pthread_self());
	assert( sem_post(&s_barrier) == 0 );
	/*
	 * problem by bol ak by sme chceli kod pouzit v cykle, vlakna by sa uz necakali.
	 * Majme 2 vlakna: s_barier hodnota nazaciatku 0 -1 0 1 0 1 nakonci
	 */

	// CS critical section

	printf("%s: Thread %lu statement 2\n", __FUNCTION__, pthread_self());

	pthread_exit((void*) 0);
}

void *_6worker_reusable(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_mutex) == 0 );
		{
			++counter;
			if( counter == *(int*)arg )
			{
				printf("\n");
				assert( sem_wait(&s_barrier_2) == 0 );
				assert( sem_post(&s_barrier) == 0 );
			}
		}
		assert( sem_post(&s_mutex) == 0 );

		// turnsitle - turniket moze prejst len jeden
		assert( sem_wait(&s_barrier) == 0 );
		printf("%s: Thread %lu statement 1\n", __FUNCTION__, pthread_self());
		assert( sem_post(&s_barrier) == 0 );

		// CS critical section

		assert( sem_wait(&s_mutex) == 0 );
		{
			--counter;
			if( counter == 0 )
			{
				printf("\n");
				assert( sem_wait(&s_barrier) == 0 );
				assert( sem_post(&s_barrier_2) == 0 );
			}
		}
		assert( sem_post(&s_mutex) == 0 );

		// turnsitle - turniket moze prejst len jeden
		assert( sem_wait(&s_barrier_2) == 0 );
		assert( sem_post(&s_barrier_2) == 0 );

		printf("%s: Thread %lu statement 2\n", __FUNCTION__, pthread_self());
	}

	pthread_exit((void*) 0);
}

void barrier(void)
{
	int i,
		threads_count = POOL_SIZE;

	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_barrier, 1, 0) == 0 );

	if( 1 )
	{
		assert( sem_init(&s_barrier_2, 1, 1) == 0 );

		for( i = 0; i < threads_count; ++i )
			assert( pthread_create(&t_pool[i], NULL, _6worker_reusable, &threads_count) == 0 );
	}
	else
		for( i = 0; i < threads_count; ++i )
			assert( pthread_create(&t_pool[i], NULL, _6worker, &threads_count) == 0 );

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_barrier) == 0 );
}
/* 3.6 Barrier END */



/* 3.8 Queue START */
/*
 * ? co k tomu povedat
 */
int count_of_a = 0, // leader
	count_of_b = 0; // follower

void dance(void)
{
	sleep((rand() % 3) + 1);
	printf("Abba - Dancing Queen: You can dance, you can jive, having the time of your life...\n");
}

void *_8worker_a(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_mutex) == 0 );
		{
			if( count_of_b > 0 )
			{
				--count_of_b;
				assert( sem_post(&s_a_arrived) == 0 );
			}
			else
			{
				++count_of_a;
				assert( sem_post(&s_mutex) == 0 );
				assert( sem_wait(&s_b_arrived) == 0 );	
			}

			printf("%s: Thread %lu is going to dance\n", __FUNCTION__, pthread_self());
			dance();
			
			assert( sem_wait(&s_semaphore) == 0 );
		}
		assert( sem_post(&s_mutex) == 0 );
	}

	pthread_exit((void*) 0);
}

void *_8worker_b(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_mutex) == 0 );
		{
			if( count_of_a > 0 )
			{
				--count_of_a;
				assert( sem_post(&s_b_arrived) == 0 );
			}
			else
			{
				++count_of_b;
				assert( sem_post(&s_mutex) == 0 );
				assert( sem_wait(&s_a_arrived) == 0 );	
			}

			printf("%s: Thread %lu is going to dance\n", __FUNCTION__, pthread_self());
			dance();
			
			assert( sem_post(&s_semaphore) == 0 );
		}
	}

	pthread_exit((void*) 0);
}

void queue(void)
{
	int i,
		threads_count = POOL_SIZE;

	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_semaphore, 1, 1) == 0 );
	assert( sem_init(&s_a_arrived, 1, 0) == 0 );
	assert( sem_init(&s_b_arrived, 1, 0) == 0 );

	for( i = 0; i < threads_count; ++i )
	{
		if( i % 2 )
			assert( pthread_create(&t_pool[i], NULL, _8worker_a, NULL) == 0 );
		else
			assert( pthread_create(&t_pool[i], NULL, _8worker_b, NULL) == 0 );
	}

	for( i = 0; i < threads_count; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_semaphore) == 0 );
	assert( sem_destroy(&s_a_arrived) == 0 );
	assert( sem_destroy(&s_b_arrived) == 0 );
}
/* 3.8 Queue END */


int main(int argc, char **argv)
{
	int u_choice = 4;

	if( argc > 1 )
		sscanf(argv[1], "%d", &u_choice); // or strtol

	switch( u_choice )
	{
		case 1:
			signaling();
			break;
		case 2:
			rendezvous();
			break;
		case 3:
			mutex();
			break;
		case 4:
			multiplex();
			break;
		case 5:
			barrier();
			break;
		case 6:
			queue();
			break;
		default:
			printf("Peu si?\n");
	}

	return 0;
}
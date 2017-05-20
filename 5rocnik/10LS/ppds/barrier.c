#include "barrier.h"

BARRIER *barrier_init(int N)
{
	printf("%s: of size=%d\n", __FUNCTION__, N);
	BARRIER *b = (BARRIER*) malloc(1 * sizeof(BARRIER));
	assert( b != NULL );
	assert( sem_init(&b->s_mutex, 1, 1) == 0 );
	assert( sem_init(&b->s_turnstile1, 1, 0) == 0 );
	assert( sem_init(&b->s_turnstile2, 1, 1) == 0 );
	b->cnt = 0;
	b->N = N;

	return b;
}

void barrier_phase1(BARRIER *b)
{
	printf("%s\n", __FUNCTION__);
	assert( sem_wait(&b->s_mutex) == 0 );
	++b->cnt;
	if( b->cnt == b->N )
	{
		assert( sem_wait(&b->s_turnstile2) == 0 );
		assert( sem_post(&b->s_turnstile1) == 0 );
	}
	assert( sem_post(&b->s_mutex) == 0 );

	assert( sem_wait(&b->s_turnstile1) == 0 );
	assert( sem_post(&b->s_turnstile1) == 0 );
}

void barrier_phase2(BARRIER *b)
{
	printf("%s\n", __FUNCTION__);
	assert( sem_wait(&b->s_mutex) == 0 );
	--b->cnt;
	if( b->cnt == 0 )
	{
		assert( sem_wait(&b->s_turnstile1) == 0 );
		assert( sem_post(&b->s_turnstile2) == 0 );
	}
	assert( sem_post(&b->s_mutex) == 0 );

	assert( sem_wait(&b->s_turnstile2) == 0 );
	assert( sem_post(&b->s_turnstile2) == 0 );
}

void *worker(void *arg)
{
	while( 1 )
	{
		barrier_phase1((BARRIER*)arg);
		// CS
		barrier_phase2((BARRIER*)arg);
	}

	pthread_exit((void*) 0);
}

void barrier_destroy(BARRIER *b)
{
	printf("%s\n", __FUNCTION__);

	if( b != NULL )
	{
		assert( sem_destroy(&b->s_mutex) == 0 );
		assert( sem_destroy(&b->s_turnstile1) == 0 );
		assert( sem_destroy(&b->s_turnstile2) == 0 );
		free((void*) b);
		b = NULL;
	}
}

// int main(void)
// {
// 	BARRIER *b;
// 	int i,
// 		size = 5;
// 	pthread_t t_pool[size];
// 	b = barrier_init(size);

// 	for( i = 0; i < size; ++i )
// 	{
// 		pthread_create(&t_pool[i], NULL, worker, b);
// 	}

// 	for( i = 0; i < size; ++i )
// 	{
// 		pthread_join(t_pool[i], NULL);
// 	}
	
// 	barrier_destroy(b);
// }
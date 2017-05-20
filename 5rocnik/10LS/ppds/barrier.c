#include "barrier.h"

BARRIER *barrier_init(int N, int version)
{
	printf("%s: of size=%d\n", __FUNCTION__, N);

	if( !(version > 0 && version < 3) )
	{
		return NULL;
	}

	BARRIER *b = (BARRIER*) malloc(1 * sizeof(BARRIER));
	assert( b != NULL );
	
	assert( sem_init(&b->s_mutex, 1, 1) == 0 );
	assert( sem_init(&b->s_turnstile1, 1, 0) == 0 );
	
	if( version == 1 )
	{
		assert( sem_init(&b->s_turnstile2, 1, 1) == 0 );
	}
	else if( version == 2 )
	{
		assert( sem_init(&b->s_turnstile2, 1, 0) == 0 );
	}

	b->cnt = 0;
	b->N = N;
	b->version = version;

	return b;
}

void barrier_phase1(BARRIER *b)
{
	printf("%s\n", __FUNCTION__);

	assert( sem_wait(&b->s_mutex) == 0 );
	++b->cnt;
	assert( b->cnt <= b->N );
	if( b->cnt == b->N )
	{
		assert( sem_wait(&b->s_turnstile2) == 0 );
		assert( sem_post(&b->s_turnstile1) == 0 );
	}
	assert( sem_post(&b->s_mutex) == 0 );

	assert( sem_wait(&b->s_turnstile1) == 0 );
	assert( sem_post(&b->s_turnstile1) == 0 );
}

void barrier_phase1_2(BARRIER *b)
{
	printf("%s\n", __FUNCTION__);

	assert( sem_wait(&b->s_mutex) == 0 );
	++b->cnt;
	assert( b->cnt <= b->N );
	if( b->cnt == b->N )
	{
		int i;
		for( i = 0; i < b->N; ++i )
		{
			assert( sem_post(&b->s_turnstile1) == 0 );	
		}
	}
	assert( sem_post(&b->s_mutex) == 0 );

	assert( sem_wait(&b->s_turnstile1) == 0 );
}

void barrier_phase2(BARRIER *b)
{
	printf("%s\n", __FUNCTION__);

	assert( sem_wait(&b->s_mutex) == 0 );
	--b->cnt;
	assert( b->cnt >= 0 );
	if( b->cnt == 0 )
	{
		assert( sem_wait(&b->s_turnstile1) == 0 );
		assert( sem_post(&b->s_turnstile2) == 0 );
	}
	assert( sem_post(&b->s_mutex) == 0 );

	assert( sem_wait(&b->s_turnstile2) == 0 );
	assert( sem_post(&b->s_turnstile2) == 0 );
}

void barrier_phase2_2(BARRIER *b)
{
	printf("%s\n", __FUNCTION__);

	assert( sem_wait(&b->s_mutex) == 0 );
	--b->cnt;
	assert( b->cnt >= 0 );
	if( b->cnt == 0 )
	{
		int i;
		for( i = 0; i < b->N; ++i )
		{
			assert( sem_post(&b->s_turnstile2) == 0 );	
		}
	}
	assert( sem_post(&b->s_mutex) == 0 );

	assert( sem_wait(&b->s_turnstile2) == 0 );
}

void *worker(void *arg)
{
	while( 1 )
	{
		if( ((BARRIER*)arg)->version == 1 )
		{
			barrier_phase1((BARRIER*)arg);
			// CS
			barrier_phase2((BARRIER*)arg);
		}
		else
		{
			barrier_phase1_2((BARRIER*)arg);
			// CS
			barrier_phase2_2((BARRIER*)arg);	
		}
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

// int main(int argc, char**argv)
// {
// 	int version = 1;
// 	if( argc > 0 )
// 		sscanf(argv[1], "%d", &version);

// 	BARRIER *b_example;
// 	int 	i,
// 		size = 5;
// 	pthread_t t_pool[size];

// 	b_example = barrier_init(size, version);
// 	assert( b_example != NULL );

// 	for( i = 0; i < size; ++i )
// 	{
// 		pthread_create(&t_pool[i], NULL, worker, b_example);
// 	}

// 	for( i = 0; i < size; ++i )
// 	{
// 		pthread_join(t_pool[i], NULL);
// 	}

// 	barrier_destroy(b_example);
// }
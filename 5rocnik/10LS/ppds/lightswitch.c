#include "lightswitch.h"

LIGHT_SWITCH *ls_init(void)
{
	LIGHT_SWITCH *ls = (LIGHT_SWITCH*) malloc(1 * sizeof(LIGHT_SWITCH));
	assert( ls != NULL );
	assert( sem_init(&ls->s_mutex, 1, 1) == 0 );
	ls->counter = 0;

	return ls;
}

void ls_lock(LIGHT_SWITCH *ls, sem_t *semaphore)
{
	assert( sem_wait(&ls->s_mutex) == 0 );
	++ls->counter;
	if( ls->counter == 1 )
		assert( sem_wait(semaphore) == 0 );
	assert( sem_post(&ls->s_mutex) == 0 );
}

void ls_unlock(LIGHT_SWITCH *ls, sem_t *semaphore)
{
	assert( sem_wait(&ls->s_mutex) == 0 );
	--ls->counter;
	if( ls->counter == 0 )
		assert( sem_post(semaphore) == 0 );
	assert( sem_post(&ls->s_mutex) == 0 );
}

void ls_free(LIGHT_SWITCH *ls)
{
	assert( sem_destroy(&ls->s_mutex) == 0 );
	free((void*)ls);
	ls = NULL;
}
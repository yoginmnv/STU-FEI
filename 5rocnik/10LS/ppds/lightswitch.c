#include "lightswitch.h"

LIGHT_SWITCH *ls_init(void)
{
	LIGHT_SWITCH *ls = (LIGHT_SWITCH*) malloc(1 * sizeof(LIGHT_SWITCH));
	assert( ls != NULL );
	assert( sem_init(&ls->s_mutex, 1, 1) == 0 );
	ls->s_semaphore = NULL;
	ls->counter = 0;

	return ls;
}

LIGHT_SWITCH *ls_init2(sem_t *s_semaphore)
{
	LIGHT_SWITCH *ls = ls_init();
	ls->s_semaphore = s_semaphore;
	return ls;
}

int ls_lock(LIGHT_SWITCH *ls, sem_t *s_semaphore)
{
	int count = UNINITIALIZED;

	assert( ls != NULL );
	if( ls != NULL )
	{
		assert( sem_wait(&ls->s_mutex) == 0 );
		count = ++ls->counter;
		if( ls->counter == 1 )
			assert( sem_wait(s_semaphore) == 0 );
		assert( sem_post(&ls->s_mutex) == 0 );
	}

	return count;
}

int ls_lock2(LIGHT_SWITCH *ls)
{
	assert( ls != NULL );
	if( ls != NULL )
	{
		assert( ls->s_semaphore != NULL );
		if( ls->s_semaphore != NULL )
		{
			return ls_lock(ls, ls->s_semaphore);
		}
	}

	return UNINITIALIZED;
}

int ls_unlock(LIGHT_SWITCH *ls, sem_t *s_semaphore)
{
	int count = UNINITIALIZED;

	assert( ls != NULL );
	if( ls != NULL )
	{
		assert( sem_wait(&ls->s_mutex) == 0 );
		count = --ls->counter;
		if( ls->counter == 0 )
			assert( sem_post(s_semaphore) == 0 );
		assert( sem_post(&ls->s_mutex) == 0 );
	}

	return count;
}

int ls_unlock2(LIGHT_SWITCH *ls)
{
	assert( ls != NULL );
	if( ls != NULL )
	{
		assert( ls->s_semaphore != NULL );
		if( ls->s_semaphore != NULL )
		{
			return ls_unlock(ls, ls->s_semaphore);
		}
	}

	return UNINITIALIZED;
}

void ls_free(LIGHT_SWITCH *ls)
{
	assert( ls != NULL );
	if( ls != NULL )
	{
		assert( sem_destroy(&ls->s_mutex) == 0 );
		free((void*)ls);
		ls = NULL;
	}
}

void ls_free2(LIGHT_SWITCH *ls)
{
	assert( ls != NULL );
	if( ls != NULL )
	{
		assert( ls->s_semaphore != NULL );
		if( ls->s_semaphore != NULL )
		{
			assert( sem_destroy(ls->s_semaphore) == 0 );
		}

		ls_free(ls);
	}
}


// int main(void)
// {
// 	LIGHT_SWITCH *ls_example = NULL;
// 	sem_t s_sem;
// 	sem_init(&s_sem, 1, 1);
// 	ls_example = ls_init2(&s_sem);

// 	printf("%d\n", ls_lock(ls_example, &s_sem));
// 	printf("%d\n", ls_lock2(ls_example));

// 	printf("%d\n", ls_unlock(ls_example, &s_sem));
// 	printf("%d\n", ls_unlock2(ls_example));

// 	ls_free2(ls_example);

// 	return 0;
// }
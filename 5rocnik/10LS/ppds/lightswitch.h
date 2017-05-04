/*
 * Maroš Polák, 4.5. 2017
 *
 * Implementation of light switch(synchronization object) in c
 * Compile:
 		gcc -pthread lightswitch.c your-code.c -o your-code
 */
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>

typedef struct
{
	sem_t s_mutex;
	int counter;
} LIGHT_SWITCH;

LIGHT_SWITCH *ls_init(void);
void ls_lock(LIGHT_SWITCH *ls, sem_t *semaphore);
void ls_unlock(LIGHT_SWITCH *ls, sem_t *semaphore);
void ls_free(LIGHT_SWITCH *ls);
/*
 * Maroš Polák, 4.5. 2017
 *
 * Implementation of light switch(synchronization object) in c.
 * When is used ls_init2 function use all functions ending with 2 as semaphore
 * was passed as argument and thereby is no need to pass it every time
 *
 * Compile:
 		gcc -pthread lightswitch.c your-code.c -o output
 */
#include <assert.h>
#include <limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define UNINITIALIZED INT_MIN

typedef struct
{
	sem_t 	s_mutex,
		*s_semaphore;
	int counter;
} LIGHT_SWITCH;

LIGHT_SWITCH *ls_init(void);
LIGHT_SWITCH *ls_init2(sem_t *s_semaphore);

int ls_lock(LIGHT_SWITCH *ls, sem_t *semaphore);
int ls_lock2(LIGHT_SWITCH *ls);

int ls_unlock(LIGHT_SWITCH *ls, sem_t *semaphore);
int ls_unlock2(LIGHT_SWITCH *ls);

void ls_free(LIGHT_SWITCH *ls);
void ls_free2(LIGHT_SWITCH *ls);

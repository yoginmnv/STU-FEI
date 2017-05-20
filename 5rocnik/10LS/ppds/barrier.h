/*
 * Maroš Polák, 4.5. 2017
 *
 * Implementation of reusable barrier(synchronization object) in c
 * Compile:
 		gcc -pthread barrier.c your-code.c -o output
 */
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	sem_t 	s_mutex,
		s_turnstile1,
		s_turnstile2;
	int 	cnt,
		N,
		version;
} BARRIER;

BARRIER *barrier_init(int N, int version);
void barrier_phase1(BARRIER *b);
void barrier_phase2(BARRIER *b);
void barrier_destroy(BARRIER *b);

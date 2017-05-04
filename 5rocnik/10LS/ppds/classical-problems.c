/*
 * Maroš Polák
 *
 * Some classical synchronizations problems from book Little Book of Semaphores by Allen B. Downey
 * compile:
 	gcc -Wall -pthread classical-problems.c lightswitch.c -o classical
 	./classical [number-of-problems-example]

	case 1:
		producer_consumer();
	case 2:
		readers_writers();
	case 3:
		dining_philosophers();
	case 4:
		dining_philosophers_tanenbaum();
	case 5:
		cigarette_smokers();
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


#define ENTER(msg) printf("%s: Executing %s %s\n", __FILE__, __FUNCTION__, msg)
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
	ENTER("");
	int i,
		threads_count = 5;

	ENTER("");
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
		printf("%s: Thread %lu update data=%d\n", __FUNCTION__, pthread_self(), data);

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
		printf("%s: Thread %lu read data=%d\n", __FUNCTION__, pthread_self(), data);

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
	ENTER("");
	int i,
		threads_count = 5,
		writers_count = 1,
		n = threads_count - writers_count;

	threads_count += 3; // x vlakien musi cakat pred multiplexom

	ENTER("");
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
	assert( sem_destroy(&s_multiplex) == 0 );
	assert( sem_destroy(&s_turnstile) == 0 );
	assert( sem_destroy(&s_room_empty) == 0 );
}
/* 4.2. Readers-writers problem END */



/* 4.4. Dining philosophers START */
/*
 * V klasickom scenari mame 5 filozofov za jednym stolom a 5 paliciek na jednie v strede je misa s ryzou a masom.
 * Filozof travi svoj cas rozmyslanim a jedenim, ak chce jest tak si musi zobrat palicku napravo a nalavo od seba
 * je to sice nechutne ale je to tak(pre predstavu takto funguje zdielanie prostriedkov v systeme), ak sa mu ich
 * podari zobrat moze sa pustit do jedenia, inak caka az kym nedoje filozof po jeho boku, ktory palicku zobral skor.
 *
 * Poziadavky:
 *		1. palicku moze drzat v danom case len jeden filozof
 *		2. nesmie vzniknut deadlock
 *		3. filozof nesmie umriet od hladu cakanim na palicku
 *		4. musi byt umoznene aby v danom case mohli jest aspon dvaja filozofi ak je ich spolu 5
 *
 * Aby nevznikol deadlock je mozne to riesit dvoma sposobmi:
 * 		1. zozenieme osobu ktora zabezpeci ze maximalne N - 1 filozofov si moze vziat palicku na jedenie a posledny caka
 *		2. aspon jeden filozof bude lavak, teda nezoberie si hned pravu palicku ale najprv lavu
 *
 * Aby neumreli od hladu: to je uz na planovaci ulohu - teda implementacii prepinania vlakien, ale spmenute riesenia pre
 * zamedzenie deadlocku to implicitne splnajux
 */
#define PHILOSOPHERS_COUNT 5
sem_t s_chopstick[PHILOSOPHERS_COUNT]; // mutex
sem_t s_footman; // multiplex

#define LEFTIE 0

void think(void)
{
	printf("%s: Thread %lu is thinking\n", __FUNCTION__, pthread_self());
	sleep(rand() % 1);
}

void eat()
{
	printf("%s: Thread %lu is eating\n", __FUNCTION__, pthread_self());
	sleep(rand() % 2);
}

// http://pages.cs.wisc.edu/~solomon/cs537/html/dphil1.gif
int get_chopsticks_on_right(int i)
{
	return i;
}

int get_chopsticks_on_left(int i)
{
	return ((i + 1) % PHILOSOPHERS_COUNT);
}

void get_chopsticks(int i)
{
#if LEFTIE == 0
	assert( sem_wait(&s_footman) == 0 );
	assert( sem_wait(&s_chopstick[get_chopsticks_on_right(i)]) == 0 );
	assert( sem_wait(&s_chopstick[get_chopsticks_on_left(i)]) == 0 );	
#else
	/* riesenie s jednym lavakom*/
	if( i == 0 ) 
	{
		assert( sem_wait(&s_chopstick[get_chopsticks_on_left(i)]) == 0 );
		assert( sem_wait(&s_chopstick[get_chopsticks_on_right(i)]) == 0 );
	}
	else
	{
		assert( sem_wait(&s_chopstick[get_chopsticks_on_right(i)]) == 0 );
		assert( sem_wait(&s_chopstick[get_chopsticks_on_left(i)]) == 0 );	
	}
#endif
}

void put_chopsticks(int i)
{
	assert( sem_post(&s_chopstick[get_chopsticks_on_right(i)]) == 0 );
	assert( sem_post(&s_chopstick[get_chopsticks_on_left(i)]) == 0 );
#if LEFTIE == 0
	assert( sem_post(&s_footman) == 0 );
#endif
}

void *_4worker(void *arg)
{
	while( 1 )
	{
		think();
		get_chopsticks(*(int*)arg);
		eat();
		put_chopsticks(*(int*)arg);
	}

	pthread_exit((void*) 0);
}

#include <string.h>
void dining_philosophers()
{
	int i;

#if LEFTIE == 0
	assert( sem_init(&s_footman, 1, PHILOSOPHERS_COUNT - 1) == 0 );
	ENTER("Solution with footman");
#else
	assert( sem_init(&s_footman, 1, PHILOSOPHERS_COUNT) == 0 );
	ENTER("Solution with leftie");
#endif

	// initializing chopsticks for philosophers
	for( i = 0; i < PHILOSOPHERS_COUNT; ++i )
		assert( sem_init(&s_chopstick[i], 1, 1) == 0 );

	// creating and running worker
	for( i = 0; i < PHILOSOPHERS_COUNT; ++i )
		assert( pthread_create(&t_pool[i], NULL, _4worker, &i) == 0 );

	// main thread waits until created threads ends
	for( i = 0; i < PHILOSOPHERS_COUNT; ++i )
	{
		assert( pthread_join(t_pool[i], NULL) == 0 );
		assert( sem_destroy(&s_chopstick[i]) == 0 );
	}
	
#if LEFTIE == 0
	assert( sem_destroy(&s_footman) == 0 );
#endif
}
/* 4.4. Dining philosophers END */



/* 4.4.5 Dining philosophers Tanenbaum START */
/*
 * V tomto rieseni problemu moze dojst k vyhladovaniu
 */
typedef enum {
	THINKING,
	HUNGRY,
	EATING
} PHILOSOPHER_STATE;

sem_t s_philosopher[PHILOSOPHERS_COUNT];
PHILOSOPHER_STATE philosopher_state[PHILOSOPHERS_COUNT];

// http://pages.cs.wisc.edu/~solomon/cs537/html/dphil1.gif
void test(int i)
{
	if(    philosopher_state[i] == HUNGRY
		&& philosopher_state[get_chopsticks_on_left(i)]  != EATING
		&& philosopher_state[get_chopsticks_on_right(i)] != EATING )
	{
		philosopher_state[i] = EATING;
		assert( sem_post(&s_philosopher[i]) == 0 );
	}
}

void get_chopsticks_tanenbaum(int i)
{
	assert( sem_wait(&s_mutex) == 0 );
	philosopher_state[i] = HUNGRY;
	test(i);
	assert( sem_post(&s_mutex) == 0 );
	assert( sem_wait(&s_philosopher[i]) == 0 );
}

void put_chopsticks_tanenbaum(int i)
{
	assert( sem_wait(&s_mutex) == 0 );
	philosopher_state[i] = THINKING;
	test(get_chopsticks_on_right(i));
	test(get_chopsticks_on_left(i));
	assert( sem_post(&s_mutex) == 0 );
}

void *_4worker_tanenbaum(void *arg)
{
	while( 1 )
	{
		think();
		get_chopsticks_tanenbaum(*(int*)arg);
		eat();
		put_chopsticks_tanenbaum(*(int*)arg);
	}

	pthread_exit((void*) 0);
}

void dining_philosophers_tanenbaum()
{
	ENTER("");
	int i;
	assert( sem_init(&s_mutex, 1, 1) == 0 );
	

	// initializing chopsticks at the table and letting philosophers to think
	for( i = 0; i < PHILOSOPHERS_COUNT; ++i )
	{
		assert( sem_init(&s_philosopher[i], 1, 0) == 0 );
		philosopher_state[i] = THINKING;
	}

	// creating and running worker(philosophers)
	for( i = 0; i < PHILOSOPHERS_COUNT; ++i )
		assert( pthread_create(&t_pool[i], NULL, _4worker_tanenbaum, &i) == 0 );

	// main thread waits until created threads ends
	for( i = 0; i < PHILOSOPHERS_COUNT; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );
	
	// destroy created semaphores
	assert( sem_destroy(&s_mutex) == 0 );
	for( i = 0; i < PHILOSOPHERS_COUNT; ++i )
		assert( sem_destroy(&s_philosopher[i]) == 0 );
}
/* 4.4.5 Dining philosophers Tanenbaum END*/



/* 4.5 Cigarette smokers problem START*/
/*
 * Agenti vytvaraju zdroje, pusheri cakaju na zdroje a nasledne ich predavaju fajciarom
 */
#define SMOKERS_COUNT 3
sem_t 	s_agent,
		s_signal,
		s_tobacco,
		s_paper,
		s_matches;
typedef enum
{
	TOBACCO,
	PAPER,
	MATCHES
} SMOKER_INGREDIENTS;


int is_tobacco 	= 0,
	is_paper 	= 0,
	is_match 	= 0;
sem_t 	s_pusher_tobacco,
		s_pusher_paper,
		s_pusher_matches;

void make_cigarette()
{
	printf("%s: Smoker %lu making cigarette\n", __FUNCTION__, pthread_self());
}

void smoke()
{
	printf("%s: Smoker %lu is smoking\n", __FUNCTION__, pthread_self());
}

void *_5worker_agent(void *arg)
{
	int type = *(int*)arg;
	assert( sem_post(&s_signal) == 0 );
	while( 1 )
	{
		assert( sem_wait(&s_mutex) == 0 );
		printf("%s %d\n", __FUNCTION__, type);
		switch( type )
		{
			/*
			 * aspon jeden agent zbehne a doda zdroje fajciarom a potom caka kym ho nejaky fajciar nevyzve
			 */
			case MATCHES:
				assert( sem_wait(&s_agent) == 0 );
				assert( sem_post(&s_tobacco) == 0 );
				assert( sem_post(&s_paper) == 0 );
				break;
			case TOBACCO:
				assert( sem_wait(&s_agent) == 0 );
				assert( sem_post(&s_paper) == 0 );
				assert( sem_post(&s_matches) == 0 );
				break;
			case PAPER:
				assert( sem_wait(&s_agent) == 0 );
				assert( sem_post(&s_tobacco) == 0 );
				assert( sem_post(&s_matches) == 0 );
				break;
			default:
				printf("%s\n", "Unsported ingredient");
				pthread_exit((void*) 0);
		}
		assert( sem_post(&s_mutex) == 0 );
		sleep((rand() % 5) + 1);
	}
}
void *_5worker_agent_a(void *arg) {
	while(1){
		assert( sem_wait(&s_agent) == 0 );
		assert( sem_post(&s_tobacco) == 0 );
		assert( sem_post(&s_paper) == 0 );
		sleep(rand()%10);
	}
}
void *_5worker_agent_b(void *arg) {
	while(1){
		assert( sem_wait(&s_agent) == 0 );
		assert( sem_post(&s_paper) == 0 );
		assert( sem_post(&s_matches) == 0 );
		sleep(rand()%10);
	}
}
void *_5worker_agent_c(void *arg) {
	while(1){
		assert( sem_wait(&s_agent) == 0 );
		assert( sem_post(&s_tobacco) == 0 );
		assert( sem_post(&s_matches) == 0 );
		sleep(rand()%10);
	}
}

void *_5worker_pusher(void *arg)
{
	int type = *(int*)arg;
	assert( sem_post(&s_signal) == 0 );
	while( 1 )
	{	
		switch( type )
		{
		case MATCHES: // MATCHES
			printf("%s %d\n", __FUNCTION__, type);
			assert( sem_wait(&s_matches) == 0 );

			assert( sem_wait(&s_mutex) == 0 );
			{
				if( is_tobacco )
				{
					is_tobacco = 0;
					assert( sem_post(&s_pusher_paper) == 0 );
				}
				else if( is_paper )
				{
					is_paper = 0;
					assert( sem_post(&s_pusher_tobacco) == 0 );
				}
				else
				{
					is_match = 1;
				}
			}
			assert( sem_post(&s_mutex) == 0 );
			break;
		case TOBACCO: // TOBACCO
			printf("%s %d\n", __FUNCTION__, type);
			assert( sem_wait(&s_tobacco) == 0 );

			assert( sem_wait(&s_mutex) == 0 );
			{
				if( is_paper )
				{
					is_paper = 0;
					assert( sem_post(&s_pusher_matches) == 0 );
				}
				else if( is_match )
				{
					is_match = 0;
					assert( sem_post(&s_pusher_paper) == 0 );	
				}
				else
				{
					is_tobacco = 1;
				}
			}
			assert( sem_post(&s_mutex) == 0 );
			break;
		case PAPER: // PAPER
			printf("%s %d\n", __FUNCTION__, type);
			assert( sem_wait(&s_paper) == 0 );

			assert( sem_wait(&s_mutex) == 0 );
			{
				if( is_match )
				{
					is_match = 0;
					assert( sem_post(&s_pusher_tobacco) == 0 );
				}
				else if( is_tobacco )
				{
					is_tobacco = 0;
					assert( sem_post(&s_pusher_matches) == 0 );
				}
				else
				{
					is_paper = 1;
				}
			}
			assert( sem_post(&s_mutex) == 0 );
			break;
		default:
			printf("%s\n", "Unsported ingredient");
			pthread_exit((void*) 0);
		}
	}

	pthread_exit((void*) 0);
}


void *_5worker_pusher_a(void *arg){
	while( 1 ){
		assert( sem_wait(&s_tobacco) == 0 );
		assert( sem_wait(&s_mutex) == 0 );
		{
			if( is_paper ) {
				is_paper = 0;
				assert( sem_post(&s_pusher_matches) == 0 );
			}
			else if( is_match ) {
				is_match = 0;
				assert( sem_post(&s_pusher_paper) == 0 );	
			}
			else {
				is_tobacco = 1;
			}
		}
		assert( sem_post(&s_mutex) == 0 );
	}
}
void *_5worker_pusher_b(void *arg){
	while( 1 ){
		assert( sem_wait(&s_paper) == 0 );
		assert( sem_wait(&s_mutex) == 0 );
		{
			if( is_match ) {
				is_match = 0;
				assert( sem_post(&s_pusher_tobacco) == 0 );
			}
			else if( is_tobacco ) {
				is_tobacco = 0;
				assert( sem_post(&s_pusher_matches) == 0 );
			}
			else {
				is_paper = 1;
			}
		}
		assert( sem_post(&s_mutex) == 0 );
	}
}
void *_5worker_pusher_c(void *arg){
	while( 1 ){
		assert( sem_wait(&s_matches) == 0 );
		assert( sem_wait(&s_mutex) == 0 );
		{
			if( is_tobacco ) {
				is_tobacco = 0;
				assert( sem_post(&s_pusher_paper) == 0 );
			}
			else if( is_paper ) {
				is_paper = 0;
				assert( sem_post(&s_pusher_tobacco) == 0 );
			}
			else {
				is_match = 1;
			}
		}
		assert( sem_post(&s_mutex) == 0 );
	}
}

void *_5worker_smoker(void *arg)
{
	int type = *(int*)arg;
	assert( sem_post(&s_signal) == 0 );
	while( 1 )
	{
		switch( type )
		{
			case TOBACCO: // TOBACCO
				// assert( sem_wait(&s_paper) == 0 );
				// assert( sem_wait(&s_matches) == 0 );
				// assert( sem_post(&s_agent) == 0 );
				printf("%s %d\n", __FUNCTION__, type);
				assert( sem_wait(&s_pusher_tobacco) == 0 );
				make_cigarette();
				assert( sem_post(&s_agent) == 0 );
				smoke();
				sleep((rand() % 3) + 1);
				break;
			case PAPER: // PAPER
				// assert( sem_wait(&s_tobacco) == 0 );
				// assert( sem_wait(&s_matches) == 0 );
				// assert( sem_post(&s_agent) == 0 );
				printf("%s %d\n", __FUNCTION__, type);
				assert( sem_wait(&s_pusher_paper) == 0 );
				make_cigarette();
				assert( sem_post(&s_agent) == 0 );
				smoke();
				sleep((rand() % 3) + 1);
				break;
			case MATCHES: // MATCHES
				// assert( sem_wait(&s_tobacco) == 0 );
				// assert( sem_wait(&s_paper) == 0 );
				// assert( sem_post(&s_agent) == 0 );
				printf("%s %d\n", __FUNCTION__, type);
				assert( sem_wait(&s_pusher_matches) == 0 );
				make_cigarette();
				assert( sem_post(&s_agent) == 0 );
				smoke();
				sleep((rand() % 3) + 1);
				break;
			default:
				printf("%s\n", "Unsported ingredient");
				pthread_exit((void*) 0);
		}
	}
}

void *_5worker_smoker_a(void *arg) {
	while( 1 ) {
		assert( sem_wait(&s_pusher_tobacco) == 0 );
		make_cigarette();
		assert( sem_post(&s_agent) == 0 );
		smoke(*(int*)arg);
		// wait a second, smoking is killing you, and don't throw your fucking cigarette on ground
		sleep(rand()%3);
	}
}
void *_5worker_smoker_b(void *arg) {
	while( 1 ) {
		assert( sem_wait(&s_pusher_paper) == 0 );
		make_cigarette(*(int*)arg);
		assert( sem_post(&s_agent) == 0 );
		smoke(*(int*)arg);
		// wait a second, smoking is killing you, and don't throw your fucking cigarette on ground
		sleep(rand()%3);
	}
}
void *_5worker_smoker_c(void *arg) {
	while( 1 ) {
		assert( sem_wait(&s_pusher_matches) == 0 );
		make_cigarette(*(int*)arg);
		assert( sem_post(&s_agent) == 0 );
		smoke(*(int*)arg);
		// wait a second, smoking is killing you, and don't throw your fucking cigarette on ground
		sleep(rand()%3);
	}
}

void cigarette_smokers(void)
{
	ENTER("");
	int i;

	srand(time(NULL));
	assert( sem_init(&s_signal	, 1, 0) == 0 );
	assert( sem_init(&s_mutex	, 1, 1) == 0 );
	assert( sem_init(&s_agent	, 1, 1) == 0 );

	assert( sem_init(&s_tobacco	, 1, 0) == 0 );
	assert( sem_init(&s_matches	, 1, 0) == 0 );
	assert( sem_init(&s_paper	, 1, 0) == 0 );

	assert( sem_init(&s_pusher_tobacco	, 1, 0) == 0 );
	assert( sem_init(&s_pusher_matches	, 1, 0) == 0 );
	assert( sem_init(&s_pusher_paper	, 1, 0) == 0 );

	//create 3 counts of threads: agents, pushers and smokers
	for( i = 0; i < SMOKERS_COUNT * 3; ++i )
	{
		if( i > 5 )
		{
			assert( pthread_create(&t_pool[i], NULL, _5worker_smoker, (void*)&i) == 0 );
			assert( sem_wait(&s_signal) == 0 );
		}
		else if( i > 2 )
		{
			assert( pthread_create(&t_pool[i], NULL, _5worker_pusher_b, (void*)&i) == 0 );
			assert( sem_wait(&s_signal) == 0 );
		}
		else
		{
			assert( pthread_create(&t_pool[i], NULL, _5worker_agent, (void*)&i) == 0 );
			assert( sem_wait(&s_signal) == 0 );
		}
	}

	// assert( pthread_create(&t_pool[0], NULL, _5worker_agent_a, (void*)&i) == 0 );
	// assert( pthread_create(&t_pool[1], NULL, _5worker_agent_b, (void*)&i) == 0 );
	// assert( pthread_create(&t_pool[2], NULL, _5worker_agent_c, (void*)&i) == 0 );

	// assert( pthread_create(&t_pool[3], NULL, _5worker_pusher_a, (void*)&i) == 0 );
	// assert( pthread_create(&t_pool[4], NULL, _5worker_pusher_b, (void*)&i) == 0 );
	// assert( pthread_create(&t_pool[5], NULL, _5worker_pusher_c, (void*)&i) == 0 );

	// assert( pthread_create(&t_pool[6], NULL, _5worker_smoker_a, (void*)&i) == 0 );
	// assert( pthread_create(&t_pool[7], NULL, _5worker_smoker_b, (void*)&i) == 0 );
	// assert( pthread_create(&t_pool[8], NULL, _5worker_smoker_c, (void*)&i) == 0 );
	
	// waiting for agents, pushers and smokers
	for( i = 0; i < SMOKERS_COUNT * 3; ++i )
		assert( pthread_join(t_pool[i], NULL) == 0 );

	// **********************************************************************//
	// uninitializing created instances
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_agent) == 0 );

	assert( sem_destroy(&s_tobacco) == 0 );
	assert( sem_destroy(&s_matches) == 0 );
	assert( sem_destroy(&s_paper) == 0 );

	assert( sem_destroy(&s_pusher_tobacco) == 0 );
	assert( sem_destroy(&s_pusher_matches) == 0 );
	assert( sem_destroy(&s_pusher_paper) == 0 );
}
/* 4.5 Cigarette smokers problem END*/



int main(int argc, char **argv)
{
	int u_choice = 5;

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
			dining_philosophers();
			break;
		case 4:
			dining_philosophers_tanenbaum(); // O Tannenbaum, O Tannenbaum|Wie treu sind deine Blatter!
			break;
		case 5:
			cigarette_smokers();
			break;
		default:
			printf("Peu si?\n");
	}
	return 0;
}
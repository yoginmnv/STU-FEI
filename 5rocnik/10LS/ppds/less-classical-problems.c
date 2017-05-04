/*
 * Maroš Polák
 *
 * Less classical synchronizations problems from book Little Book of Semaphores by Allen B. Downey
 * compile:
 	gcc -Wall -pthread lightswitch.c less-classical-problems.c -o less-classical
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
#include <semaphore.h>
/*
	int sem_init(sem_t *sem, int pshared, unsigned int value);
	The pshared argument indicates whether this semaphore is to be shared between the threads of a process, or between processes.
*/
#include <stdio.h>
#include <stdlib.h> // rand
#include <unistd.h> // sleep
#include "lightswitch.h"


#define ENTER(msg) printf("%s: Executing %s%s\n", __FILE__, __FUNCTION__, msg)
int counter = 0;
sem_t s_mutex;

/* 5.1. The dining savages problem START */
/*
 * Divosi z kmena jedia spolcne z velkeho kotla, do ktoreho sa zmesti M porci duseneho misionara.
 * Ak chce divoch jest tak sa pozrie ci je nieco v kotli, ak nie je tak zobudi spiaceho kuchara
 * a caka kym kuchar nenavari a nenaplni kotol, inak si zoberie porciu z kotla a zacina jest.
 *
 * Obmedzenia:
 *		1. divosi nemozu zobudit kuchara kym nie je kotol prazdny
 *		2. kuchar moze naplnit kotol len ak je prazdny
 */
#define M 10 // pocet porci misionara ktore sa zmestia do kotla
#define SAVAGE_COUNT 20 // pocet divochov
// semafori na signalizaciu
sem_t 	s_pot_empty,
		s_pot_full;
int servings_in_pot = 0;

const char *BODY_PARTS[] = {"head", "face", "neck", "palm", "finger", "shoulder", "arm", "wrist", "leg", "toe", "foot", "eye"};
#define BODY_PARTS_SIZE 11

void eat(void)
{
	printf("%s: savage %lu is eating %s\n", __FUNCTION__, pthread_self(), BODY_PARTS[rand() % BODY_PARTS_SIZE]);
}

void *_1worker_cook(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_pot_empty) == 0 );
		// nie je to jamie oliver tak to bude chvilku trvat kym nieco navari, vodu nabrat z potoka, rozlozit ohen, ...
		sleep(rand() % 2);
		// put servings to pot
		servings_in_pot = (rand() % SAVAGE_COUNT) + 1;
		// ak obet bola vacsia a nezmesti sa do kotla treba zvysok hodit zralokom alebo odlozit
		if( servings_in_pot > M )
			servings_in_pot -= servings_in_pot - M;
		printf("%s: Cook cooked %d portions of missionary\n", __FUNCTION__, servings_in_pot);
		assert( sem_post(&s_pot_full) == 0 );
	}

	pthread_exit((void*) 0);
}

void *_1worker_savage(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_mutex) == 0 );
		if( servings_in_pot == 0 )
		{
			assert( sem_post(&s_pot_empty) == 0 );
			assert( sem_wait(&s_pot_full) == 0 );
		}
		// get servings from pot
		--servings_in_pot;
		assert( servings_in_pot >= 0 );
		assert( sem_post(&s_mutex) == 0 );

		eat();
	}

	pthread_exit((void*) 0);
}

void dining_savages()
{
	ENTER("");
	int i;
	pthread_t 	t_cook,
				t_savage[SAVAGE_COUNT];

	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_pot_empty, 1, 0) == 0 );
	assert( sem_init(&s_pot_full, 1, 0) == 0 );

	//***********************************************************************//
	assert( pthread_create(&t_cook, NULL, _1worker_cook, NULL) == 0 );
	for( i = 0; i < SAVAGE_COUNT; ++i )
		assert( pthread_create(&t_savage[i], NULL, _1worker_savage, NULL) == 0 );

	//***********************************************************************//
	assert( pthread_join(t_cook, NULL) == 0 );
	for( i = 0; i < SAVAGE_COUNT; ++i )
		assert( pthread_join(t_savage[i], NULL) == 0 );

	//***********************************************************************//
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_pot_empty) == 0 );
	assert( sem_destroy(&s_pot_full) == 0 );
}
/* 5.1. The dining savages problem END */



/* 5.2 The barbershop problem START */
#define N 4 // cakaren s N stolickami
#define CUSTOMERS_COUNT 8 // pocet zakaznikov ktory sa mozu dotrepat

sem_t 	s_customer,
		s_barber,
		s_customer_done,
		s_barber_done;

void *_2worker_barber(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_customer) == 0 );
		assert( sem_post(&s_barber) == 0 );
		// cut hair
		printf("%s: making new frisure\n", __FUNCTION__);
		sleep(1);
		assert( sem_wait(&s_customer_done) == 0 );
		assert( sem_post(&s_barber_done) == 0 );
	}

	pthread_exit((void*) 0);
}

void *_2worker_customer(void *arg)
{
	while( 1 )
	{
		// check if waiting room is not full
		assert( sem_wait(&s_mutex) == 0 );
		if( counter == N )
		{
			assert( sem_post(&s_mutex) == 0 );
			continue;
		}
		// increment count of customers
		++counter;
		assert( sem_post(&s_mutex) == 0 );

		// rendezvous
		assert( sem_post(&s_customer) == 0 ); // oznamim ze som prisiel
		assert( sem_wait(&s_barber) == 0 ); // cakam kym ma holic prebudi
		// get hair cut
		printf("%s: %lu in expectations how it ends, curently at waiting room %d\n", __FUNCTION__, pthread_self(), counter);

		// rendezvous
		assert( sem_post(&s_customer_done) == 0 );
		assert( sem_wait(&s_barber_done) == 0 );


		// decrement count of customers
		assert( sem_wait(&s_mutex) == 0 );
		--counter;
		assert( sem_post(&s_mutex) == 0 );
		// leave waiting room
	}

	pthread_exit((void*) 0);
}

void barbershop()
{
	ENTER("");
	int i;
	pthread_t 	t_barber,
				t_customer[CUSTOMERS_COUNT];

	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_customer, 1, 0) == 0 );
	assert( sem_init(&s_customer_done, 1, 0) == 0 );
	assert( sem_init(&s_barber_done, 1, 0) == 0 );

	//***********************************************************************//
	assert( pthread_create(&t_barber, NULL, _2worker_barber, NULL) == 0 );
	for( i = 0; i < CUSTOMERS_COUNT; ++i )
		assert( pthread_create(&t_customer[i], NULL, _2worker_customer, NULL) == 0 );

	//***********************************************************************//
	assert( pthread_join(t_barber, NULL) == 0 );
	for( i = 0; i < CUSTOMERS_COUNT; ++i )
		assert( pthread_join(t_customer[i], NULL) == 0 );

	//***********************************************************************//
	assert( sem_destroy(&s_mutex) == 0 );
	assert( sem_destroy(&s_customer) == 0 );
	assert( sem_destroy(&s_customer_done) == 0 );
	assert( sem_destroy(&s_barber_done) == 0 );
}
/* 5.2 The barbershop problem END */



/* 6.1 The search-insert-delete problem START */
/*
 *
 */
#define LIST_MAX_SIZE 100
#define COUNT_OF_SEARCHERS 1
#define COUNT_OF_INSERTERS 2
#define COUNT_OF_DELETERS 1
typedef struct list
{
	int value;
	struct list *next;
} LIST;

int items_in_list = 0,
	actual_searcher_count = 0,
	actual_inserter_count = 0;

sem_t 	s_insert_mutex,
		s_ls_mutex_search,
		s_ls_mutex_insert,
		s_no_searcher,
		s_no_inserter;

/* list functions START */
void list_insert_front(LIST **lib, int value) // push_front
{
	LIST *pom = NULL;
	assert( (pom = (LIST*) malloc(sizeof(LIST))) != NULL );
	
	pom->value = value;
	pom->next = NULL;

	if( *lib == NULL )
	{
		*lib = pom;
	}
	else
	{
		pom->next = *lib;
		*lib = pom;
	}
}

void list_insert_back(LIST **lib, int value) // push_back
{
	LIST 	*pom = NULL,
			*actual = NULL;

	assert( (pom = (LIST*) malloc(sizeof(LIST))) != NULL );
	pom->value = value;
	pom->next = NULL;

	if( *lib == NULL )
	{
		*lib = pom;
	}
	else
	{
		actual = *lib;
		while( actual->next != NULL )
		{
			actual = actual->next;
		}
		actual->next = pom;
	}

	++items_in_list;
}

int list_search(LIST *linked_list, int value)
{
    while( linked_list != NULL )
    {
		if( linked_list->value == value )
			return 1;
        linked_list = linked_list->next;
    }

    return 0;
}

void list_delete(LIST **linked_list, int value)
{	
	LIST 	*pom = *linked_list,
			*actual = *linked_list;
	int index = 0;

	//if( pom->value == value )
	if( index == value)
	{
		*linked_list = pom->next;
		free((void*) pom);
	}
	else
	{
		while( pom->next != NULL )
		{
			actual = pom;
			pom = pom->next;

			++index;
			//if( pom->value == value )
			if( index == value )
			{
				actual->next = pom->next;
				free((void*)pom);
				return;
			}
		}

		//if( pom->value == value )
		if( index == value )
		{
			actual->next = NULL;
			free((void*) pom);
		}
	}

	--items_in_list;
}
/* list functions END */

void *_1worker_search(void *arg)
{
	ENTER("");
	LIST *linked_list = (LIST*)arg;
	while( 1 )
	{
		//*******************************************************************//
		// light switch searchSwitch.wait(s_no_searcher)
		assert( sem_wait(&s_ls_mutex_search) == 0 );
		++actual_searcher_count;
		if( actual_searcher_count == 1 )
			assert( sem_wait(&s_no_searcher) == 0 );
		assert( sem_post(&s_ls_mutex_search) == 0 );
		//*******************************************************************//
		if( items_in_list == 0 )
		{
			printf("%s: list is empty\n", __FUNCTION__);
		}
		else
		{
			int value = (rand() % items_in_list);
			printf("%s: searching %d in list\n", __FUNCTION__, value);
			list_search(linked_list, value);
			sleep(1);
		}
		//*******************************************************************//
		// light switch searchSwitch.post(s_no_searcher)
		assert( sem_wait(&s_ls_mutex_search) == 0 );
		--actual_searcher_count; // pocet citatelov
		if( actual_searcher_count == 0 )
			assert( sem_post(&s_no_searcher) == 0 );
		assert( sem_post(&s_ls_mutex_search) == 0 );
	}

	pthread_exit((void*) 0);
}

void *_1worker_insert(void *arg)
{
	ENTER("");
	LIST *linked_list = (LIST*)arg;
	while( 1 )
	{
		//*******************************************************************//
		// light switch insertSwitch.wait(s_no_inserter)
		assert( sem_wait(&s_ls_mutex_insert) == 0 );
		++actual_inserter_count;
		if( actual_inserter_count == 1 )
			assert( sem_wait(&s_no_inserter) == 0 );
		assert( sem_post(&s_ls_mutex_insert) == 0 );
		//*******************************************************************//
		assert( sem_wait(&s_insert_mutex) == 0 );
		if( items_in_list == LIST_MAX_SIZE )
		{
			printf("%s: list is full, size=%d\n", __FUNCTION__, LIST_MAX_SIZE);
		}
		else
		{
			int value = (rand() % 100);
			printf("%s: inserting %d into list\n", __FUNCTION__, value);
			list_insert_back(&linked_list, value);
			++items_in_list;
			sleep(1);
		}
		assert( sem_post(&s_insert_mutex) == 0 );
		//*******************************************************************//
		// light switch insertSwitch.post(s_no_inserter)
		assert( sem_wait(&s_ls_mutex_insert) == 0 );
		--actual_inserter_count;
		if( actual_inserter_count == 0 )
			assert( sem_post(&s_no_inserter) == 0 );
		assert( sem_post(&s_ls_mutex_insert) == 0 );
		//*******************************************************************//
	}

	pthread_exit((void*) 0);
}

void *_1worker_delete(void *arg)
{
	ENTER("");
	LIST *linked_list = (LIST*)arg;
	while( 1 )
	{
		assert( sem_wait(&s_no_searcher) == 0 );
		assert( sem_wait(&s_no_inserter) == 0 );
		//*******************************************************************//
		if( items_in_list == 0 )
		{
			printf("%s: list is empty\n", __FUNCTION__);
		}
		else
		{
			int value = (rand() % items_in_list);
			printf("%s: deleting %d from list\n", __FUNCTION__, value);
			list_delete(&linked_list, value);
			--items_in_list;
			sleep(1);
		}
		//*******************************************************************//
		assert( sem_post(&s_no_inserter) == 0 );
		assert( sem_post(&s_no_searcher) == 0 );
	}

	pthread_exit((void*) 0);
}

void search_insert_delete(void)
{
	ENTER("");
	LIST *linked_list = NULL;
	int i;
	pthread_t 	t_searcher[COUNT_OF_SEARCHERS],
				t_inserter[COUNT_OF_INSERTERS],
				t_deleter[COUNT_OF_DELETERS];

	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_insert_mutex, 1, 1) == 0 );
	assert( sem_init(&s_ls_mutex_search, 1, 1) == 0 );
	assert( sem_init(&s_ls_mutex_insert, 1, 1) == 0 );
	assert( sem_init(&s_no_searcher, 1, 1) == 0 );
	assert( sem_init(&s_no_inserter, 1, 1) == 0 );

	//***********************************************************************//
	for( i = 0; i < COUNT_OF_SEARCHERS; ++i )
		assert( pthread_create(&t_searcher[i], NULL, _1worker_search, &linked_list) == 0 );

	for( i = 0; i < COUNT_OF_INSERTERS; ++i )
		assert( pthread_create(&t_inserter[i], NULL, _1worker_insert, &linked_list) == 0 );

	for( i = 0; i < COUNT_OF_DELETERS; ++i )
		assert( pthread_create(&t_deleter[i], NULL, _1worker_delete, &linked_list) == 0 );

	//***********************************************************************//
	for( i = 0; i < COUNT_OF_SEARCHERS; ++i )
		assert( pthread_join(t_searcher[i], NULL) == 0 );

	for( i = 0; i < COUNT_OF_INSERTERS; ++i )
		assert( pthread_join(t_inserter[i], NULL) == 0 );

	for( i = 0; i < COUNT_OF_DELETERS; ++i )
		assert( pthread_join(t_deleter[i], NULL) == 0 );

	//***********************************************************************//
	assert( sem_destroy(&s_insert_mutex) == 0 );
	assert( sem_destroy(&s_ls_mutex_search) == 0 );
	assert( sem_destroy(&s_ls_mutex_insert) == 0 );
	assert( sem_destroy(&s_no_searcher) == 0 );
	assert( sem_destroy(&s_no_inserter) == 0 );

	free((void*)linked_list);
	linked_list = NULL;
}
/* 6.1 The search-insert-delete problem END */



/* 6.2 The unisex bathroom problem START */
/*
 *
 */
#define COUNT_OF_MENS 5
#define COUNT_OF_WOMENS 3
#define BATHROOM_CAPACITY 3
sem_t 	s_empty,
		s_multiplex_male,
		s_multiplex_female,
		// s_mutex_male,
		// s_mutex_female,
		s_turnstile;
LIGHT_SWITCH 	*ls_male,
				*ls_female;

int man_count = 0,
	woman_count = 0;

void shower_time(char *sex)
{
	printf("%s: %s in bathroom\n", __FUNCTION__, sex);
	sleep((rand() % 2) + 1);
}

void *_2worker_male(void *arg)
{
	LIGHT_SWITCH *ls = (LIGHT_SWITCH*)arg;
	while( 1 )
	{
		assert( sem_wait(&s_turnstile) == 0 );
		{
			ls_lock(ls, &s_empty);
			// assert( sem_wait(&s_mutex_male) == 0 );
			// ++man_count;
			// if( man_count == 1 )
			// 	assert( sem_wait(&s_empty) == 0 );
			// assert( sem_post(&s_mutex_male) == 0 );
		}
		assert( sem_post(&s_turnstile) == 0 );


		assert( sem_wait(&s_multiplex_male) == 0 );
		shower_time("male");
		assert( sem_post(&s_multiplex_male) == 0 );

		ls_unlock(ls, &s_empty);
		// assert( sem_wait(&s_mutex_male) == 0 );
		// --man_count;
		// if( man_count == 0 )
		// 	assert( sem_post(&s_empty) == 0 );
		// assert( sem_post(&s_mutex_male) == 0 );
	}

	pthread_exit((void*)0);
}

void *_2worker_female(void *arg)
{
	LIGHT_SWITCH *ls = (LIGHT_SWITCH*)arg;
	while( 1 )
	{
		assert( sem_wait(&s_turnstile) == 0 );
		{
			ls_lock(ls, &s_empty);
			// assert( sem_wait(&s_mutex_female) == 0 );
			// ++woman_count;
			// if( woman_count == 1 )
			// 	assert( sem_wait(&s_empty) == 0 );
			// assert( sem_post(&s_mutex_female) == 0 );
		}
		assert( sem_post(&s_turnstile) == 0 );


		assert( sem_wait(&s_multiplex_female) == 0 );
		shower_time("female");
		assert( sem_post(&s_multiplex_female) == 0 );

		ls_unlock(ls, &s_empty);
		// assert( sem_wait(&s_mutex_female) == 0 );
		// --woman_count;
		// if( woman_count == 1 )
		// 	assert( sem_post(&s_empty) == 0 );
		// assert( sem_post(&s_mutex_female) == 0 );
	}

	pthread_exit((void*)0);
}

void unisex_bathroom(void)
{
	ENTER("");
	int i;
	pthread_t 	t_male[COUNT_OF_MENS],
				t_female[COUNT_OF_WOMENS];

	assert( sem_init(&s_empty, 1, 1) == 0 );
	ls_male 	= ls_init();
	ls_female 	= ls_init();
	// assert( sem_init(&s_mutex_male, 1, 1) == 0 );
	// assert( sem_init(&s_mutex_female, 1, 1) == 0 );
	assert( sem_init(&s_multiplex_male, 1, BATHROOM_CAPACITY) == 0 );
	assert( sem_init(&s_multiplex_female, 1, BATHROOM_CAPACITY) == 0 );
	assert( sem_init(&s_turnstile, 1, 1) == 0 );

	//***********************************************************************//
	for( i = 0; i < COUNT_OF_MENS; ++i )
		assert( pthread_create(&t_male[i], NULL, _2worker_male, (void*)ls_male) == 0 );

	for( i = 0; i < COUNT_OF_WOMENS; ++i )
		assert( pthread_create(&t_female[i], NULL, _2worker_female, (void*)ls_female) == 0 );

	//***********************************************************************//
	for( i = 0; i < COUNT_OF_MENS; ++i )
		assert( pthread_join(t_male[i], NULL) == 0 );

	for( i = 0; i < COUNT_OF_WOMENS; ++i )
		assert( pthread_join(t_female[i], NULL) == 0 );

	//***********************************************************************//
	assert( sem_destroy(&s_empty) == 0 );
	ls_free(ls_male);
	ls_free(ls_female);
	// assert( sem_destroy(&s_mutex_male) == 0 );
	// assert( sem_destroy(&s_mutex_female) == 0 );
	assert( sem_destroy(&s_multiplex_male) == 0 );
	assert( sem_destroy(&s_multiplex_female) == 0 );
	assert( sem_destroy(&s_turnstile) == 0 );
}
/* 6.2 The unisex bathroom problem END */



/* 7.2 The child care problem START */
/*
 * Poziadavky:
 *		1. v sredisku starostlivosti o detia musi byt aspon 1 dospela osoba pre 3 deti
 */
#define COUNT_OF_ADULTS 1
#define COUNT_OF_CHILDREN 3
#define MAX_COUNT_OF_CHILDREN_PER_ADULT 3

sem_t s_multiplex;

void *_2worker_adult(void *arg) // nurse
{
	int i;
	while( 1 )
	{
		for( i = 0; i < COUNT_OF_CHILDREN; ++i )
		{
			assert( sem_post(&s_multiplex) == 0 );
		}

		// CS

		assert( sem_wait(&s_mutex) == 0 );
		for( i = 0; i < COUNT_OF_CHILDREN; ++i )
		{
			assert( sem_wait(&s_multiplex) == 0 );
		}
		assert( sem_wait(&s_mutex) == 0 );
	}

	pthread_exit((void*) 0);
}

void *_2worker_child(void *arg)
{
	while( 1 )
	{
		assert( sem_wait(&s_multiplex) == 0 );

		assert( sem_post(&s_multiplex) == 0 );
	}

	pthread_exit((void*) 0);
}

void child_care(void)
{
	ENTER("");
	int i;
	pthread_t 	t_adult[COUNT_OF_ADULTS],
				t_child[COUNT_OF_CHILDREN];

	if( COUNT_OF_ADULTS * MAX_COUNT_OF_CHILDREN_PER_ADULT < COUNT_OF_CHILDREN )
	{
		printf("%s: ERROR read the problem conditions\n", __FUNCTION__);
		return;
	}

	assert( sem_init(&s_mutex, 1, 1) == 0 );
	assert( sem_init(&s_multiplex, 1, COUNT_OF_ADULTS * MAX_COUNT_OF_CHILDREN_PER_ADULT) == 0 );

	//***********************************************************************//
	for( i = 0; i < COUNT_OF_ADULTS; ++i )
		assert( pthread_create(&t_adult[i], NULL, _2worker_adult, NULL) == 0 );

	for( i = 0; i < COUNT_OF_CHILDREN; ++i )
		assert( pthread_create(&t_child[i], NULL, _2worker_child, NULL) == 0 );

	//***********************************************************************//
	for( i = 0; i < COUNT_OF_ADULTS; ++i )
		assert( pthread_join(t_adult[i], NULL) == 0 );

	for( i = 0; i < COUNT_OF_CHILDREN; ++i )
		assert( pthread_join(t_child[i], NULL) == 0 );

	//***********************************************************************//
	assert( sem_destroy(&s_empty) == 0 );
	assert( sem_destroy(&s_mutex_male) == 0 );
	assert( sem_destroy(&s_mutex_female) == 0 );
	assert( sem_destroy(&s_multiplex_male) == 0 );
	assert( sem_destroy(&s_multiplex_female) == 0 );
	assert( sem_destroy(&s_turnstile) == 0 );
}
/* 7.2 The child care problem END */

int main(int argc, char **argv)
{
	int u_choice = 4;

	if( argc > 1 )
		sscanf(argv[1], "%d", &u_choice);
	
	switch( u_choice )
	{
		case 1:
			dining_savages();
			break;
		case 2:
			barbershop();
			break;
		case 3:
			search_insert_delete();
			break;
		case 4:
			unisex_bathroom();
			break;
		case 5:
			child_care();
			break;
		default:
			printf("Peu si?\n");
	}
	return 0;
}
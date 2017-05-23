/*
 * Maroš Polák
 *
 * Some classical synchronizations problems from book Little Book of Semaphores by Allen B. Downey
 * compile:
    gcc -Wall -pthread classical-problems.c lightswitch.c -o classical
    ./classical [number-of-problems-example] [type]

    case 1:
        producer_consumer();
    case 2:
        readers_writers(type); type = 1 for clasic, 2 for no starvation, 3 for priority writers
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
 *  int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
 */
#include <semaphore.h>
/*
 *  int sem_init(sem_t *sem, int pshared, unsigned int value);
 *  The pshared argument indicates whether this semaphore is to be shared between the threads of a process, or between processes.
 *
 *  int sem_destroy(sem_t *sem);
 */ 
#include <stdio.h>
#include <stdlib.h> // rand
#include <unistd.h> // sleep
#include "lightswitch.h"


#define ENTER(msg) printf("%s: Executing %s %s\n", __FILE__, __FUNCTION__, msg)
#define POOL_SIZE 10
pthread_t t_pool[POOL_SIZE];
int counter = 0;


/* 4.1. Producer-consumer problem START */
/*
 *
 */
int     items = 0;
sem_t   s_mutex,
    s_producer,
    s_consumer;
sem_t   s_items;

void *_1worker_produce(void *arg)
{
    // while( 1 )
    // {
    //  assert( sem_wait(&s_consumer) == 0 ); // cakaj kym nepride nejaky konzumator

    //  sleep(1);
    //  items = (rand() % 5) + 1;
    //  printf("_1worker_consume: Thread %lu \E[32mincrement\E[0m items=%d\n", pthread_self(), items);

    //  assert( sem_post(&s_producer) == 0 );
    // }

    while( 1 )
    {
        // wait for event
        sleep(rand() % 3);

        assert( sem_wait(&s_mutex) == 0 );
        // buffer.add( event )
        assert( sem_post(&s_mutex) == 0 );
        assert( sem_post(&s_items) == 0 );
    }

    pthread_exit((void*) 0);
}

void *_1worker_consume(void *arg)
{
    // while( 1 )
    // {
    //  assert( sem_wait(&s_mutex) == 0 );
    //  if( items == 0 )
    //  {
    //      assert( sem_post(&s_consumer) == 0 );
    //      assert( sem_wait(&s_producer) == 0 );
    //  }
    //  --items;
    //  printf("_1worker_consume: Thread %lu \E[31mdecrement\E[0m items=%d\n", pthread_self(), items);
    //  assert( sem_post(&s_mutex) == 0 );
    // }

    while( 1 )
    {
        assert( sem_wait(&s_items) == 0 );
        assert( sem_wait(&s_mutex) == 0 );
        // event = buffer.get()
        assert( sem_post(&s_mutex) == 0 );
        // event.process()
        // processing event
        sleep(rand() % 3);
    }

    pthread_exit((void*) 0);
}

void producer_consumer(void)
{
    ENTER("");
    int     i,
        threads_count = 5;

    ENTER("");
    assert( sem_init(&s_mutex, 1, 1) == 0 );
    assert( sem_init(&s_producer, 1, 0) == 0 );
    assert( sem_init(&s_consumer, 1, 0) == 0 );

    assert( sem_init(&s_items, 1, 0) == 0 );

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

    assert( sem_destroy(&s_items) == 0 );
}
/* 4.1. Producer-consumer problem END */



/* 4.2. Readers-writers problem START */
/*
 * 1. Classic: zapisovatelia mozu vyhladoviet lebo sa tam moze nahrnut vela citatelov a nemusi sa uvolnit miestnost
 * 2. Starvation: aby sa aj zapisovatelia dostali k slovu prida sa tam turniket
 * 3. Priority: zapisovatelia maju prednost pred citatelmi
 */
sem_t   s_turnstile,
    s_room_empty,
    s_no_reader,
    s_no_writer;
LIGHT_SWITCH    *ls_reader,
        *ls_writer;
int data = 0;

void *_2worker_writer_classic(void *arg)
{
    while( 1 )
    {
        assert( sem_wait(&s_room_empty) == 0 );

        // CS update
        data = rand() % 100;
        printf("%s: Thread %lu update data=%d\n", __FUNCTION__, pthread_self(), data);

        assert( sem_post(&s_room_empty) == 0 );

        sleep(rand() % 2);
    }

    pthread_exit((void*) 0);
}

void *_2worker_reader_classic(void *arg)
{
    while( 1 )
    {
        // LightSwitch lock
        assert( sem_wait(&s_mutex) == 0 );
        ++counter; // pocet citatelov
        if( counter == 1 )
            assert( sem_wait(&s_room_empty) == 0 );
        assert( sem_post(&s_mutex) == 0 );

        // CS read
        printf("%s: Thread %lu read data=%d\n", __FUNCTION__, pthread_self(), data);
        sleep(rand() % 2);

        // LightSwitch unlock
        assert( sem_wait(&s_mutex) == 0 );
        --counter; // pocet citatelov
        if( counter == 0 )
            assert( sem_post(&s_room_empty) == 0 );
        assert( sem_post(&s_mutex) == 0 );
    }

    pthread_exit((void*) 0);
}
/*****************************************************************************/
void *_2worker_writer_starvation(void *arg)
{
    while( 1 )
    {
        assert( sem_wait(&s_turnstile)  == 0 );
        assert( sem_wait(&s_room_empty) == 0 );

        // CS update
        data = rand() % 100;
        printf("%s: Thread %lu update data=%d\n", __FUNCTION__, pthread_self(), data);

        /* #IMP
         * ak by boli nasledujuce operacie naopak tak musi dojst k prepnutia naviac
         * lebo ak by bolo najprv sem_post(&s_turnstile) tak sa moze zobudit
         * ine vlakno no ale aj tak sa zasekne na sem_wait(&s_room_empty)
         */
        assert( sem_post(&s_room_empty) == 0 );
        assert( sem_post(&s_turnstile)  == 0 );

        sleep(rand() % 1);
    }

    pthread_exit((void*) 0);
}

void *_2worker_reader_starvation(void *arg)
{
    while( 1 )
    {
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
    }

    pthread_exit((void*) 0);
}
/*****************************************************************************/
void *_2worker_writer_priority(void *arg)
{
    while( 1 )
    {
        ls_lock(ls_writer, &s_no_reader);
        sem_wait(&s_no_writer);

        // CS update
        data = rand() % 100;
        printf("%s: Thread %lu update data=%d\n", __FUNCTION__, pthread_self(), data);

        sem_post(&s_no_writer);
        ls_unlock(ls_writer, &s_no_reader);

        sleep((rand() % 2));
    }

    pthread_exit((void*) 0);
}

void *_2worker_reader_priority(void *arg)
{
    while( 1 )
    {
        sem_wait(&s_no_reader);
        ls_lock(ls_reader, &s_no_writer);
        sem_post(&s_no_reader);
        // CS read
        printf("%s: Thread %lu read data=%d\n", __FUNCTION__, pthread_self(), data);
        ls_unlock(ls_reader, &s_no_writer);

        sleep((rand() % 2));
    }

    pthread_exit((void*) 0);
}

void readers_writers(int type)
{
    ENTER("");
    assert( (type > 0) && (type < 4) );

    int     i,
        threads_count = 5,
        writers_count = 1;
    void (*fnc[3][2])={
        {_2worker_reader_classic, _2worker_writer_classic},
        {_2worker_reader_starvation, _2worker_writer_starvation},
        {_2worker_reader_priority, _2worker_writer_priority}
    };

    
    threads_count += threads_count + writers_count; // x vlakien musi cakat pred multiplexom

    ENTER("");
    assert( sem_init(&s_mutex, 1, 1) == 0 );
    assert( sem_init(&s_turnstile, 1, 1) == 0 );
    assert( sem_init(&s_room_empty, 1, 1) == 0 );
    assert( sem_init(&s_no_reader, 1, 1) == 0 );
    assert( sem_init(&s_no_writer, 1, 1) == 0 );
    ls_reader = ls_init();
    ls_writer = ls_init();

    for( i = 0; i < threads_count; ++i )
    {
        if( i > writers_count )
            assert( pthread_create(&t_pool[i], NULL, fnc[type - 1][0], NULL) == 0 );
        else
            assert( pthread_create(&t_pool[i], NULL, fnc[type - 1][1], NULL) == 0 );
    }

    for( i = 0; i < threads_count; ++i )
        assert( pthread_join(t_pool[i], NULL) == 0 );
    
    assert( sem_destroy(&s_mutex) == 0 );
    assert( sem_destroy(&s_room_empty) == 0 );
    assert( sem_destroy(&s_no_reader) == 0 );
    assert( sem_destroy(&s_no_writer) == 0 );
    ls_free(ls_reader);
    ls_free(ls_writer);
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
 *      1. palicku moze drzat v danom case len jeden filozof
 *      2. nesmie vzniknut deadlock
 *      3. filozof nesmie umriet od hladu cakanim na palicku
 *      4. musi byt umoznene aby v danom case mohli jest aspon dvaja filozofi ak je ich spolu 5
 *
 * Aby nevznikol deadlock je mozne to riesit dvoma sposobmi:
 *      1. zozenieme osobu ktora zabezpeci ze maximalne N - 1 filozofov si moze vziat palicku na jedenie a posledny caka
 *      2. aspon jeden filozof bude lavak, teda nezoberie si hned pravu palicku ale najprv lavu
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
// v knihe to ma naopak #IMP 87-88
// takisto forks a potom pouziva fork 88
// she pre philosopher? 95
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
#if LEFTIE == 1
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
#else
    assert( sem_wait(&s_footman) == 0 );
    assert( sem_wait(&s_chopstick[get_chopsticks_on_right(i)]) == 0 );
    assert( sem_wait(&s_chopstick[get_chopsticks_on_left(i)]) == 0 );   
#endif
}

void put_chopsticks(int i)
{
    assert( sem_post(&s_chopstick[get_chopsticks_on_right(i)]) == 0 );
    assert( sem_post(&s_chopstick[get_chopsticks_on_left(i)]) == 0 );
#if LEFTIE == 1
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
 * Agenti vytvaraju ingrediencie(zdroje), pusheri cakaju na zdroje a nasledne ich posuvaju fajciarom
 *
 * V generalizovanom probleme sa vyuziva vzor Scoreboard, kde sa sleduje
 * kolko ingredienci je dostupnych v systeme a na zakladne toho sa vykonava
 * nejaka operacia.
 *
 * Cize ak su ingrediecnie na vytvorenie cigarety tak sa zoberu zo stola a 
 * pusher zobudi prislusneho fajciara v opacnom pripade sa necha ingrediencia 
 * na stole a caka sa na dalsieho pushera
 *  
 */
#define SMOKERS_COUNT 3
sem_t   s_signal;
typedef enum
{
    TOBACCO,
    PAPER,
    MATCHES
} SMOKER_INGREDIENTS;

sem_t   s_agent,
        s_ing_tobacco,
        s_ing_paper,
        s_ing_matches;

sem_t   s_pusher_tobacco,
        s_pusher_paper,
        s_pusher_matches;

int is_tobacco  = 0,
    is_paper    = 0,
    is_match    = 0;

void make_cigarette()
{
    printf("%s: Smoker %lu making cigarette\n", __FUNCTION__, pthread_self());
}

void smoke()
{
    printf("%s: Smoker %lu smoking\n", __FUNCTION__, pthread_self());
}

void do_rest(void)
{
    make_cigarette();
    assert( sem_post(&s_agent) == 0 );
    smoke();
    sleep((rand() % 3) + 1);
}

void *_5worker_agent(void *arg)
{
    int type = *(int*)arg;
    assert( sem_post(&s_signal) == 0 );
    while( 1 )
    {
        switch( type )
        {
            /*
             * aspon jeden agent zbehne a doda zdroje pusherom a potom caka
             * kym ho nejaky fajciar nevyzve o dalsie zdroje
             */
            case TOBACCO:
                assert( sem_wait(&s_agent) == 0 );
                printf("%s %d\n", __FUNCTION__, type);
                assert( sem_post(&s_ing_paper) == 0 );
                assert( sem_post(&s_ing_matches) == 0 );
                break;
            case PAPER:
                assert( sem_wait(&s_agent) == 0 );
                printf("%s %d\n", __FUNCTION__, type);
                assert( sem_post(&s_ing_tobacco) == 0 );
                assert( sem_post(&s_ing_matches) == 0 );
                break;
            case MATCHES:
                assert( sem_wait(&s_agent) == 0 );
                printf("%s %d\n", __FUNCTION__, type);
                assert( sem_post(&s_ing_tobacco) == 0 );
                assert( sem_post(&s_ing_paper) == 0 );
                break;
            default:
                printf("%s\n", "Unsported agent");
                pthread_exit((void*) 0);
        }
    }

    pthread_exit((void*) 0);
}

void *_5worker_pusher(void *arg)
{
    int type = *(int*)arg;
    assert( sem_post(&s_signal) == 0 );
    while( 1 )
    {   
        switch( type )
        {
        case TOBACCO:
            assert( sem_wait(&s_ing_tobacco) == 0 );
            // scoreboard
            assert( sem_wait(&s_mutex) == 0 );
            printf("%s %d\n", __FUNCTION__, type);
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
        case MATCHES:
            assert( sem_wait(&s_ing_matches) == 0 );
            // scoreboard
            assert( sem_wait(&s_mutex) == 0 );
            printf("%s %d\n", __FUNCTION__, type);
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
        case PAPER:
            assert( sem_wait(&s_ing_paper) == 0 );
            // scoreboard
            assert( sem_wait(&s_mutex) == 0 );
            printf("%s %d\n", __FUNCTION__, type);
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

void *_5worker_smoker(void *arg)
{
    int type = *(int*)arg;
    assert( sem_post(&s_signal) == 0 );
    while( 1 )
    {
        switch( type )
        {
            case TOBACCO:
                assert( sem_wait(&s_pusher_tobacco) == 0 );
                printf("%s %d\n", __FUNCTION__, type);
                do_rest();
                break;
            case MATCHES:
                assert( sem_wait(&s_pusher_matches) == 0 );
                printf("%s %d\n", __FUNCTION__, type);
                do_rest();
                break;
            case PAPER:
                assert( sem_wait(&s_pusher_paper) == 0 );
                printf("%s %d\n", __FUNCTION__, type);
                do_rest();
                break;
            default:
                printf("%s\n", "Unsported smoker");
                pthread_exit((void*) 0);
        }
    }

    pthread_exit((void*) 0);
}

void cigarette_smokers(void)
{
    ENTER("");
    int i,
        type;

    srand(time(NULL));

    assert( sem_init(&s_mutex   , 1, 1) == 0 );
    assert( sem_init(&s_agent   , 1, 1) == 0 );
    assert( sem_init(&s_signal  , 1, 0) == 0 );

    assert( sem_init(&s_ing_tobacco , 1, 0) == 0 );
    assert( sem_init(&s_ing_matches , 1, 0) == 0 );
    assert( sem_init(&s_ing_paper   , 1, 0) == 0 );

    assert( sem_init(&s_pusher_tobacco  , 1, 0) == 0 );
    assert( sem_init(&s_pusher_matches  , 1, 0) == 0 );
    assert( sem_init(&s_pusher_paper    , 1, 0) == 0 );

    //create 3 counts of threads: agents, pushers and smokers
    for( i = 0; i < SMOKERS_COUNT * 3; ++i )
    {
        type = i % SMOKERS_COUNT;
        if( i > 5 )
        {
            assert( pthread_create(&t_pool[i], NULL, _5worker_smoker, (void*)&type) == 0 );
        }
        else if( i > 2 )
        {
            assert( pthread_create(&t_pool[i], NULL, _5worker_pusher, (void*)&type) == 0 );
        }
        else
        {
            assert( pthread_create(&t_pool[i], NULL, _5worker_agent, (void*)&type) == 0 );
        }
        assert( sem_wait(&s_signal) == 0 );
    }
    
    // waiting for agents, pushers and smokers
    for( i = 0; i < SMOKERS_COUNT * 3; ++i )
        assert( pthread_join(t_pool[i], NULL) == 0 );

    // uninitializing created instances
    assert( sem_destroy(&s_mutex) == 0 );
    assert( sem_destroy(&s_agent) == 0 );
    assert( sem_destroy(&s_signal) == 0 );

    assert( sem_destroy(&s_ing_tobacco) == 0 );
    assert( sem_destroy(&s_ing_matches) == 0 );
    assert( sem_destroy(&s_ing_paper) == 0 );

    assert( sem_destroy(&s_pusher_tobacco) == 0 );
    assert( sem_destroy(&s_pusher_matches) == 0 );
    assert( sem_destroy(&s_pusher_paper) == 0 );
}
/* 4.5 Cigarette smokers problem END*/



int main(int argc, char **argv)
{
    int u_choice = 5,
    type = 1;

    if( argc > 1 )
        sscanf(argv[1], "%d", &u_choice);
    
    switch( u_choice )
    {
        case 1:
            producer_consumer();
            break;
        case 2:
            if( argc > 2 )
                sscanf(argv[2], "%d", &type);
            readers_writers(type);
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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>

#define PHILOSOPHERS 5
#define CLEANERS 1
#define PLATES 5


pthread_t thread_philosopher_ids[PHILOSOPHERS];
pthread_t thread_cleaner_ids[CLEANERS];
pthread_mutex_t mutex_chopstick[PHILOSOPHERS];
int args[PHILOSOPHERS];

sem_t sem_empty;
sem_t sem_full;
pthread_barrier_t barrier;

int buffer[PLATES];
pthread_mutex_t mutex_buffer;

int clean_plate_count = 0;



void* cleaner(void* arg) {

    while (1) {
        // clean plate to be put on the table
        int clean_plate = 1;
        
        // lock buffer and take away one from sem_empty if possible
        sem_wait(&sem_empty);
        pthread_mutex_lock(&mutex_buffer);

        // cleaner puts a clean plate on the table, which is read to be taken
        buffer[clean_plate_count] = clean_plate;
        clean_plate_count++;

        // unlock buffer and add one to sem_full
        pthread_mutex_unlock(&mutex_buffer);
        sem_post(&sem_full);
    }

    pthread_exit(NULL);
}


void* philosopher(void* arg) {
    int thread_id = *(int*) arg;

    while (1) {

        // philosopher tries to take one chopstick
        if (pthread_mutex_trylock( &mutex_chopstick[thread_id]) != 0) {
            continue;
        }
        
        // philosopher tries to take second chopstick if the philosopher
        // was able to take the first chopstick
        if (pthread_mutex_trylock( &mutex_chopstick[(thread_id + 1) % PHILOSOPHERS]) != 0) {
            pthread_mutex_unlock( &mutex_chopstick[thread_id] );
            continue;
        }

        // philospher takes a clean plate, minus one from sem_full
        // and lock buffer
        sem_wait(&sem_full);
        pthread_mutex_lock(&mutex_buffer);

        // philosopher is eating
        if (clean_plate_count > 0) {
            clean_plate_count--;
            printf("Philosopher %d is eating...\n", thread_id);
        }

        // add dirty plate to sem_empty, release buffer
        pthread_mutex_unlock(&mutex_buffer);
        sem_post(&sem_empty);

        // release chopsticks
        pthread_mutex_unlock( &mutex_chopstick[thread_id] );
        pthread_mutex_unlock( &mutex_chopstick[(thread_id + 1) % PHILOSOPHERS] );

        // each philosopher thinks after they have eaten
        sleep(0.5);

        // A barrier to wait for all 5 PHILOSOPHERS to finish eating
        // this is so that each philosopher eats evenly
        pthread_barrier_wait(&barrier);

    }

    pthread_exit(NULL);
}


int main(int argc, char* argv[]) {

    // initialise variables
    pthread_mutex_init(&mutex_buffer, NULL);
    sem_init(&sem_empty, 0, PLATES);
    sem_init(&sem_full, 0, 0);
    pthread_barrier_init(&barrier, NULL, PHILOSOPHERS);

    // create cleaners
    for (int i = 0; i < CLEANERS; i++) {
        if (pthread_create(&thread_cleaner_ids[i], NULL, &cleaner, NULL) != 0) {
            perror("Failed to create thread");
        }
    }

    // create philosophers
    for (int i = 0; i < PHILOSOPHERS; i++) {
        args[i] = i;
        if (pthread_create(&thread_philosopher_ids[i], NULL, &philosopher, (void*) &args[i]) != 0) {
            perror("Failed to create thread");
        }
    }
    
    // merge threads
    for (int i = 0; i < CLEANERS; i++) {
        pthread_join(thread_cleaner_ids[i], NULL);
    }
    for (int i = 0; i < PHILOSOPHERS; i++) {
        pthread_join(thread_philosopher_ids[i], NULL);
    }
    
    // destry semaphores and barrier
    sem_destroy(&sem_empty);
	sem_destroy(&sem_full);
    pthread_barrier_destroy(&barrier);

    return 0;
}
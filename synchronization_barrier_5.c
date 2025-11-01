#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdatomic.h>

// ---------------- Config ----------------
#define NUM_PRODUCERS 3
#define NUM_VALUES 5
const char *filename = "demo_file.txt";

// ---------------- Semaphores ----------------
sem_t file_semaphore;  // Protects file writes (mutual exclusion)

// ---------------- Mutex ----------------
pthread_mutex_t counter_lock;   // Protects shared counter
pthread_mutex_t producer_lock;  // Protects producers_finished & ready

// ---------------- Condition Variable ----------------
pthread_cond_t cond_var;        // Consumer waits until all producers finish

// ---------------- Barrier ----------------
pthread_barrier_t barrier;      // Synchronize all producers at midpoint

// ---------------- Shared Variables ----------------
int counter = 0;                 // Shared counter
int producers_finished = 0;      // Number of finished producers
int ready = 0;                   // Flag for condition variable

// ---------------- Memory Fence Variables ----------------
atomic_int fence_counter = 0;    // Final counter value
atomic_int fence_flag = 0;       // Signals that final counter is ready

// ---------------- File Creation Thread ----------------
void *create_file(void *arg) {
    FILE *f = fopen(filename, "w"); // Clear the file
    if (f) fclose(f);
    printf("File created/cleared: %s\n", filename);
    pthread_exit(NULL);
}

// ---------------- Producer Thread ----------------
void *producer(void *arg) {
    pthread_t tid = pthread_self();
    printf("Producer thread %lu started.\n", (unsigned long)tid);

    // Phase 1: write first half of values
    for (int i = 0; i < NUM_VALUES / 2; i++) {
        pthread_mutex_lock(&counter_lock); // Mutex: increment counter safely
        counter++;
        int my_value = counter;
        pthread_mutex_unlock(&counter_lock);

        sem_wait(&file_semaphore);         // Semaphore: exclusive file access
        FILE *f = fopen(filename, "a");
        if (f) fprintf(f, "%d\n", my_value);
        if (f) fclose(f);
        sem_post(&file_semaphore);

        usleep(10000); // Simulate work
    }

    // Barrier: all producers wait here before continuing
    pthread_barrier_wait(&barrier);

    // Phase 2: write remaining values
    for (int i = NUM_VALUES / 2; i < NUM_VALUES; i++) {
        pthread_mutex_lock(&counter_lock);
        counter++;
        int my_value = counter;
        pthread_mutex_unlock(&counter_lock);

        sem_wait(&file_semaphore);
        FILE *f = fopen(filename, "a");
        if (f) fprintf(f, "%d\n", my_value);
        if (f) fclose(f);
        sem_post(&file_semaphore);

        usleep(10000);
    }

    // Signal consumer if last producer
    pthread_mutex_lock(&producer_lock);
    producers_finished++;
    if (producers_finished == NUM_PRODUCERS) {
        // Memory fence: safely publish final counter
        atomic_store_explicit(&fence_counter, counter, memory_order_relaxed);
        atomic_thread_fence(memory_order_release);
        atomic_store_explicit(&fence_flag, 1, memory_order_relaxed);

        ready = 1;
        pthread_cond_signal(&cond_var);
    }
    pthread_mutex_unlock(&producer_lock);

    pthread_exit(NULL);
}

// ---------------- Consumer Thread ----------------
void *consumer(void *arg) {
    // Wait until all producers finish
    pthread_mutex_lock(&producer_lock);
    while (!ready) {
        pthread_cond_wait(&cond_var, &producer_lock);
    }
    pthread_mutex_unlock(&producer_lock);

    // Read file contents
    printf("Consumer reading file contents:\n");
    FILE *f = fopen(filename, "r");
    if (f) {
        int val;
        while (fscanf(f, "%d", &val) == 1) {
            printf("%d\n", val);
        }
        fclose(f);
    }

    // Memory fence: read final counter safely
    while (atomic_load_explicit(&fence_flag, memory_order_acquire) == 0);
    int final_count = atomic_load_explicit(&fence_counter, memory_order_relaxed);
    printf("Consumer sees final counter (fence): %d\n", final_count);

    pthread_exit(NULL);
}

// ---------------- Main ----------------
int main() {
    pthread_t create_thread, producers[NUM_PRODUCERS], consumer_thread;

    // Initialize synchronization primitives
    sem_init(&file_semaphore, 0, 1);          // Semaphore
    pthread_mutex_init(&counter_lock, NULL);  // Mutex
    pthread_mutex_init(&producer_lock, NULL); // Mutex
    pthread_cond_init(&cond_var, NULL);       // Condition variable
    pthread_barrier_init(&barrier, NULL, NUM_PRODUCERS); // Barrier

    // Create/clear file
    pthread_create(&create_thread, NULL, create_file, NULL);
    pthread_join(create_thread, NULL);

    // Start producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_create(&producers[i], NULL, producer, NULL);

    // Start consumer thread
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    // Wait for all threads
    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(producers[i], NULL);
    pthread_join(consumer_thread, NULL);

    // Cleanup
    sem_destroy(&file_semaphore);
    pthread_mutex_destroy(&counter_lock);
    pthread_mutex_destroy(&producer_lock);
    pthread_cond_destroy(&cond_var);
    pthread_barrier_destroy(&barrier);

    printf("Program completed successfully.\n");
    return 0;
}

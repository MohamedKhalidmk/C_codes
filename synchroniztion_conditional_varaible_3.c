#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

// ---------------- File ----------------
const char *filename = "empty_file.txt";

// ---------------- Semaphores ----------------
sem_t file_semaphore;      // Protects file access (mutual exclusion)

// ---------------- Mutex ----------------
pthread_mutex_t counter_lock;  // Protects shared counter
pthread_mutex_t producer_lock; // Protects producers_finished & ready

// ---------------- Condition Variable ----------------
pthread_cond_t cond_var;       // Consumer waits until producers finish

// ---------------- Shared Variables ----------------
int counter = 0;                 // Shared counter between producers
int producers_finished = 0;      // Number of finished producers
int ready = 0;                   // Flag to indicate all producers finished

// ---------------- File Creation Thread ----------------
void *create_file(void *arg) {
    printf("Creating the file (only if it doesn’t exist): %s\n", filename);
    fflush(stdout);

    if (access(filename, F_OK) != 0) {
        if (system("touch empty_file.txt") != 0) {
            perror("Failed to create the file");
            pthread_exit(NULL);
        } else {
            printf("File created successfully: %s\n", filename);
        }
    } else {
        FILE *f = fopen(filename, "w"); // Clear existing file
        if (f) fclose(f);
        printf("File already exists, skipping creation.\n");
    }

    pthread_exit(NULL);
}

// ---------------- Producer Thread ----------------
void *producer(void *arg) {
    pthread_t thread_id = pthread_self();
    printf("Producer writing random integers: %lu\n", (unsigned long)thread_id);
    fflush(stdout);

    for (int i = 0; i < 5; i++) {
        // Mutex: safely increment shared counter
        pthread_mutex_lock(&counter_lock);
        counter++;
        int my_value = counter;
        pthread_mutex_unlock(&counter_lock);

        // Semaphore: safely write to file
        sem_wait(&file_semaphore);
        FILE *f = fopen(filename, "a");
        if (f) {
            fprintf(f, "%d\n", my_value);
            fclose(f);
        } else {
            perror("Failed to append to file");
        }
        sem_post(&file_semaphore);
    }

    // Mutex + Condition Variable: signal consumer if last producer
    pthread_mutex_lock(&producer_lock);
    producers_finished++;
    if (producers_finished == 3) {
        ready = 1;
        pthread_cond_signal(&cond_var);
    }
    pthread_mutex_unlock(&producer_lock);

    pthread_exit(NULL);
}

// ---------------- Consumer Thread ----------------
void *consumer(void *arg) {
    printf("Consumer reading the file contents:\n");
    fflush(stdout);

    // Mutex + Condition Variable: wait until all producers finish
    pthread_mutex_lock(&producer_lock);
    while (!ready) {
        pthread_cond_wait(&cond_var, &producer_lock);
    }
    pthread_mutex_unlock(&producer_lock);

    // Read file contents (no semaphore needed; producers handled writes)
    FILE *f = fopen(filename, "r");
    if (f) {
        int value;
        while (fscanf(f, "%d", &value) == 1) {
            printf("%d\n", value);
        }
        fclose(f);
    } else {
        perror("Failed to read the file contents");
    }

    pthread_exit(NULL);
}

// ---------------- Main Function ----------------
int main() {
    pthread_t create_thread, producer_thread[3], consumer_thread;

    // Initialize synchronization primitives
    sem_init(&file_semaphore, 0, 1);       // Semaphore
    pthread_mutex_init(&counter_lock, NULL);  // Mutex
    pthread_mutex_init(&producer_lock, NULL); // Mutex
    pthread_cond_init(&cond_var, NULL);       // Condition Variable

    // Create file
    if (pthread_create(&create_thread, NULL, create_file, NULL) != 0) {
        perror("Failed to create file creation thread");
        return 1;
    }
    pthread_join(create_thread, NULL);

    // Start producer threads
    for (int i = 0; i < 3; i++) {
        if (pthread_create(&producer_thread[i], NULL, producer, NULL) != 0) {
            perror("Failed to create producer thread");
            return 1;
        }
    }

    // Start consumer thread
    if (pthread_create(&consumer_thread, NULL, consumer, NULL) != 0) {
        perror("Failed to create consumer thread");
        return 1;
    }

    // Wait for producer threads
    for (int i = 0; i < 3; i++)
        pthread_join(producer_thread[i], NULL);

    // Wait for consumer thread
    pthread_join(consumer_thread, NULL);

    // Cleanup synchronization primitives
    sem_destroy(&file_semaphore);
    pthread_mutex_destroy(&counter_lock);
    pthread_mutex_destroy(&producer_lock);
    pthread_cond_destroy(&cond_var);

    printf("Program completed successfully.\n");
    return 0;
}

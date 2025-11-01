#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>  // For semaphores

const char *filename = "empty_file.txt";

// Declare semaphore to protect file access
sem_t file_semaphore;

// Thread function to create the file
void *create_file(void *arg) {
    FILE *f = fopen(filename, "w"); // Clear the file
    if (f) fclose(f);
    printf("File created/cleared: %s\n", filename);
    pthread_exit(NULL);
}

// Thread function for the producer
void *producer(void *arg) {
    printf("Producer writing random integers to the file using 'echo':\n");
    srand(time(NULL)); // Seed random number generator

    for (int i = 0; i < 10; i++) {
        int random_num = rand() % 100; // Generate random integer < 100
        char command[100];

        // Protect file access with semaphore
        sem_wait(&file_semaphore);

        // Construct the echo command
        snprintf(command, sizeof(command), "echo %d >> %s", random_num, filename);

        // Execute the echo command
        if (system(command) != 0) {
            perror("Failed to append random number to file");
            sem_post(&file_semaphore);
            pthread_exit(NULL);
        }

        sem_post(&file_semaphore);
    }

    pthread_exit(NULL);
}

// Thread function for the consumer
void *consumer(void *arg) {
    printf("Consumer reading the file contents:\n");

    // Protect file access with semaphore
    sem_wait(&file_semaphore);

    // Use the 'cat' command to display the file contents
    if (system("cat empty_file.txt") != 0) {
        perror("Failed to read the file contents");
        sem_post(&file_semaphore);
        pthread_exit(NULL);
    }

    sem_post(&file_semaphore);
    pthread_exit(NULL);
}

int main() {
    pthread_t create_thread, producer_thread, consumer_thread;

    // Initialize semaphore with 1 (mutual exclusion)
    sem_init(&file_semaphore, 0, 1);

    // Create the file creation thread
    if (pthread_create(&create_thread, NULL, create_file, NULL) != 0) { // Fixed function name
        perror("Failed to create file creation thread");
        return 1;
    }

    pthread_join(create_thread, NULL);

    // Create the producer thread
    if (pthread_create(&producer_thread, NULL, producer, NULL) != 0) {
        perror("Failed to create producer thread");
        return 1;
    }

    // Create the consumer thread
    if (pthread_create(&consumer_thread, NULL, consumer, NULL) != 0) {
        perror("Failed to create consumer thread");
        return 1;
    }

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // Destroy the semaphore
    sem_destroy(&file_semaphore);

    printf("Program completed successfully.\n");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h> // For semaphores

const char *filename = "empty_file.txt";

// ---------------- Semaphores ----------------
sem_t file_semaphore;   // Protects file access (mutual exclusion)
sem_t producer_done;    // Signals when producer has finished writing

// ---------------- File Creation Thread ----------------
void *create_file(void *arg) {
    printf("Creating the file (only if it doesn’t exist): %s\n", filename);
    fflush(stdout);

    // Check if file exists
    if (access(filename, F_OK) != 0) {
        if (system("touch empty_file.txt") != 0) {
            perror("Failed to create the file");
            pthread_exit(NULL);
        } else {
            printf("File created successfully: %s\n", filename);
        }
    } else {
        FILE *f = fopen(filename, "w"); // Clear the file if it exists
        if (f) fclose(f);
        printf("File already exists, skipping creation.\n");
    }

    pthread_exit(NULL);
}

// ---------------- Producer Thread ----------------
void *producer(void *arg) {
    printf("Producer writing random integers to the file using 'echo':\n");
    fflush(stdout);

    srand(time(NULL)); // Seed random number generator

    // Wait for access to the file (mutual exclusion)
    sem_wait(&file_semaphore);

    for (int i = 0; i < 10; i++) {
        int random_num = rand() % 100; // Generate a random integer < 100
        char command[100];

        // Construct the echo command
        snprintf(command, sizeof(command), "echo %d >> %s", random_num, filename);

        // Execute the command
        if (system(command) != 0) {
            perror("Failed to append random number to file");
            sem_post(&file_semaphore); // Ensure semaphore is released
            pthread_exit(NULL);
        }
    }

    // Signal that the producer is done
    sem_post(&producer_done);

    // Release the file semaphore after finishing writing
    sem_post(&file_semaphore);

    pthread_exit(NULL);
}

// ---------------- Consumer Thread ----------------
void *consumer(void *arg) {
    printf("Consumer reading the file contents:\n");
    fflush(stdout);

    // Wait until producer signals that writing is done
    sem_wait(&producer_done);

    // Wait for access to the file (mutual exclusion)
    sem_wait(&file_semaphore);

    // Read and display file contents
    if (system("cat empty_file.txt") != 0) {
        perror("Failed to read the file contents");
        sem_post(&file_semaphore); // Ensure semaphore is released
        pthread_exit(NULL);
    }

    // Release file semaphore
    sem_post(&file_semaphore);

    pthread_exit(NULL);
}

// ---------------- Main Function ----------------
int main() {
    pthread_t create_thread, producer_thread, consumer_thread;

    // Initialize semaphores
    sem_init(&file_semaphore, 0, 1);  // Initialize to 1 for mutual exclusion
    sem_init(&producer_done, 0, 0);   // Initialize to 0; consumer waits

    // Create the file creation thread
    if (pthread_create(&create_thread, NULL, create_file, NULL) != 0) {
        perror("Failed to create file creation thread");
        return 1;
    }
    pthread_join(create_thread, NULL); // Wait for file creation

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

    // Wait for threads to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // Destroy semaphores
    sem_destroy(&file_semaphore);
    sem_destroy(&producer_done);

    printf("Program completed successfully.\n");
    return 0;
}

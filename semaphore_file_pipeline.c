#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define FILE_COUNT 2  // Number of files to create

// Array to hold the file names
const char *filenames[FILE_COUNT];

// Thread function to create files and store their names (thread 1)
void *create(void *arg) {
printf("Creating 2 files...\n");


// Create two empty files by opening and closing them
filenames[0] = "file1.txt";
filenames[1] = "file2.txt";

// Open the files for writing, this will create them if they do not exist
FILE *file1 = fopen(filenames[0], "w");
FILE *file2 = fopen(filenames[1], "w");

if (!file1 || !file2) {
    perror("Failed to create files");
    
}

// Close the files after creating them
fclose(file1);
fclose(file2);

// Print file names
printf("Files created: %s, %s\\n", filenames[0], filenames[1]);





}

// Thread function for the producer (thread 2)
void *producer(void *arg) {
printf("Producer writing random integers to files:\n");


srand(time(NULL));  // Seed for random number generation

for (int i = 0; i< FILE_COUNT; i++) {
    
    FILE *file = fopen(filenames[i], "a");
    if (!file) {
        perror("Failed to open file for writing");
        pthread_exit(NULL);
    }

    // Insert 3 random integers into the file (maximum of 3 lines)
    for (int i = 0; i < 3; i++) {
        int random_num = rand() % 100;  // Generate a random integer < 100
        fprintf(file, "%d\n", random_num);  // Use correct newline
    }

    fclose(file);  // Close the file after writing
}

pthread_exit(NULL);



}

// Thread function for the consumer (thread 3)
void *consumer(void *arg) {
printf("Consumer reading the files' contents:\n");

for (int i = 0; i< FILE_COUNT; i++) {
    // Open the file in read mode
    FILE *file = fopen(filenames[i], "r");
    if (!file) {
        perror("Failed to open file for reading");
        pthread_exit(NULL);
    }

    // Read and print the file contents
    printf("Contents of %s:\\n", filenames[i]);
    char line[100];

    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);  // Close the file after reading
}

pthread_exit(NULL);



}

int main() {
pthread_t create_thread, producer_thread, consumer_thread;


// Create the file creation thread (thread 1)
if (pthread_create(&create_thread, NULL, create, NULL) != 0) {
    perror("Failed to create file creation thread");
    return 1;
}



// Create the producer thread (thread 2)
if (pthread_create(&producer_thread, NULL, producer, NULL) != 0) {
    perror("Failed to create producer thread");
    return 1;
}

// Create the consumer thread (thread 3)
if (pthread_create(&consumer_thread, NULL, consumer, NULL) != 0) {
    perror("Failed to create consumer thread");
    return 1;
}



printf("Program completed successfully.\\n");
return 0;



}
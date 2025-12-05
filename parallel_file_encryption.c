#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main() {
    size_t N = 1000000;
    unsigned char *data = malloc(N);
    unsigned char *encrypted = malloc(N);
    unsigned char *decrypted = malloc(N); 
    unsigned char key = 0xAA;

    // Initialize data
    for(size_t i=0;i<N;i++) data[i] = i%256;

    #pragma omp parallel for
    for(size_t i=0;i<N;i++)
        encrypted[i] = data[i]^key;

    for(size_t i=0;i<N;i++)
        decrypted[i] = encrypted[i] ^ key;
    // Check
    printf("Original[0] = %x\n", data[0]);
    printf("Encrypted[0] = %x\n", encrypted[0]);
    printf("Decrypted[0] = %x\n", decrypted[0]);
    free(data); free(encrypted);
    return 0;
}

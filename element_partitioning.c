#include <stdio.h>
#include <omp.h>
#define N 1000

int main() {
    int A[N], B[N], C[N];

    for(int i=0;i<N;i++){
        A[i] = i;
        B[i] = N-i;
    }

    #pragma omp parallel for
    for(int i=0;i<N;i++)
        C[i] = A[i]+B[i];

    printf("C[0]=%d, C[N-1]=%d\n", C[0], C[N-1]);
    return 0;
}

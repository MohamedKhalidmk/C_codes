#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define CHUNK_SIZE 64   // smaller chunk for testing
#define MAX_CHUNKS 4    // fewer chunks for demonstration

// Simple RLE compression placeholder: returns new length
size_t rle_compress(const char *in, size_t inlen, char *out) {
    size_t oi = 0;
    for (size_t i = 0; i < inlen; ) {
        char c = in[i];
        size_t j = i;
        while (j < inlen && in[j] == c) j++;
        size_t run = j - i;
        out[oi++] = c;
        out[oi++] = (char)(run > 255 ? 255 : run);
        i = j;
    }
    return oi;
}

int main(void) {
    const char *infile = "demo_input.txt";
    const char *outfile = "demo_output.rle";

    // create small sample input
    FILE *fin = fopen(infile, "w");
    fprintf(fin, "AAAAABBBBCCCCDDDDDEEEE\nAABBCC\nAAAA\n");
    fclose(fin);

    // buffers and lengths
    char *bufs[MAX_CHUNKS];
    char *compbufs[MAX_CHUNKS];
    size_t buflen[MAX_CHUNKS];
    size_t complen[MAX_CHUNKS];
    for (int i = 0; i < MAX_CHUNKS; ++i) { bufs[i] = compbufs[i] = NULL; buflen[i] = complen[i] = 0; }

    int chunk_count = 0;

    fin = fopen(infile, "r");
    if (!fin) { perror("open input"); return 1; }

    omp_set_dynamic(0);
    #pragma omp parallel
    {
        #pragma omp single
        {
            while (!feof(fin) && chunk_count < MAX_CHUNKS) {
                bufs[chunk_count] = malloc(CHUNK_SIZE);
                size_t rd = fread(bufs[chunk_count], 1, CHUNK_SIZE, fin);
                buflen[chunk_count] = rd;

                // reader task
                #pragma omp task firstprivate(chunk_count) depend(out: bufs[chunk_count])
                {
                    printf("[reader] chunk %d read (%zu bytes) on thread %d\n",
                           chunk_count, buflen[chunk_count], omp_get_thread_num());
                    printf("[reader] data: ");
                    fwrite(bufs[chunk_count], 1, buflen[chunk_count], stdout);
                    printf("\n");
                }

                // compressor task
                compbufs[chunk_count] = malloc(CHUNK_SIZE * 2);
                #pragma omp task firstprivate(chunk_count) depend(in: bufs[chunk_count]) depend(out: compbufs[chunk_count])
                {
                    printf("[compress] chunk %d compressing on thread %d\n", chunk_count, omp_get_thread_num());
                    complen[chunk_count] = rle_compress(bufs[chunk_count], buflen[chunk_count], compbufs[chunk_count]);
                    printf("[compress] chunk %d compressed (%zu bytes): ", chunk_count, complen[chunk_count]);
                    for (size_t i = 0; i < complen[chunk_count]; ++i) {
                        printf("%c%u ", compbufs[chunk_count][i], (unsigned char)compbufs[chunk_count][++i]);
                    }
                    printf("\n");
                }

                // writer task
                #pragma omp task firstprivate(chunk_count) depend(in: compbufs[chunk_count])
                {
                    printf("[writer] chunk %d writing on thread %d\n", chunk_count, omp_get_thread_num());
                    FILE *fout = fopen(outfile, "ab");
                    if (fout) {
                        fwrite(&complen[chunk_count], sizeof(size_t), 1, fout);
                        fwrite(compbufs[chunk_count], 1, complen[chunk_count], fout);
                        fclose(fout);
                        printf("[writer] chunk %d written (%zu bytes)\n", chunk_count, complen[chunk_count]);
                    } else perror("writer open");
                }

                ++chunk_count;
            }
            #pragma omp taskwait
        }
    }

    // cleanup
    for (int i = 0; i < chunk_count; ++i) { free(bufs[i]); free(compbufs[i]); }
    fclose(fin);

    printf("\nPipeline finished. chunks=%d\n", chunk_count);
    return 0;
}

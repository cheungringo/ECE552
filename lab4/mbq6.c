// Open Conformation - COMPILED WITH O0

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 16 // 64B blocks
#define ARRAY_SIZE 16384
#define ITERATION_SIZE 40960

int main() {
	int * array = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_two = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_three = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_four = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_five = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_six = (int *) malloc(sizeof(int) * ARRAY_SIZE);



    int j = 1;
    int i = 0;
    int k = 0;
    int dummy = 0;
    int dummy2 = 0;

    // Stride always fails, open-ended will always prefetch correctly
	for(i = 0; i < ITERATION_SIZE; i++) {
        j = (int) pow(2, i % 14);
		dummy += array[j % ARRAY_SIZE];
        dummy += array_two[j % ARRAY_SIZE];
        dummy += array_three[j % ARRAY_SIZE];
        dummy += array_four[j % ARRAY_SIZE];
        dummy += array_five[j % ARRAY_SIZE];
        dummy += array_six[j % ARRAY_SIZE];
	}

    // Direct Mapped Estimate STRIDE - 40960*6 = 245760
    // Direct Mapped Estimate OPEN - Bias + 6*15

    // Direct Mapped STRIDE - dl1.misses                   245818 # total number of misses
    // Direct Mapped OPEN - dl1.misses                      573 # total number of misses

	return(dummy);
}

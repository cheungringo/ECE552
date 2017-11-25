// Next Line Confirmation

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 16 // 64B blocks
#define ARRAY_SIZE 8192
#define ITERATION_SIZE 409600

int main() {
	int * array = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int dummy = 0;
	int i;

	// Miss only on loop around 8192/16 = 512, 409600/512 = 800 times
	for(i = 0; i < ITERATION_SIZE; i++) { 
		dummy += array[(i*16) % ARRAY_SIZE];
	}

    // Miss always, two blocks over, failing on loop around 4096 times
    for(i = 0; i < ITERATION_SIZE/100; i++) {
        dummy += array[(i*64) % ARRAY_SIZE];
    }

    // Estimated Direct Mapped = 4896
    // Direct Mapped - dl1.misses                     4939 # total number of misses

	return(dummy);

}

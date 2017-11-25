// Stride Conformation

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 16 // 64B blocks
#define ARRAY_SIZE 8192
#define ITERATION_SIZE 409600

int main() {
	int * array = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_two = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_three = (int *) malloc(sizeof(int) * ARRAY_SIZE);
	int * array_four = (int *) malloc(sizeof(int) * ARRAY_SIZE);


	int dummy = 0;
	int i;

	// Fails on wrap around 8192/16 = 512; 409600/512 = 800 times
	for(i = 0; i < ITERATION_SIZE; i++) {
		dummy += array[(i*16) % ARRAY_SIZE];
	}

	// Fails on wrap around 8192/32 = 256; 409600/256 = 1600 times
	for(i = 0; i < ITERATION_SIZE; i++) {
		dummy += array_two[(i*32) % ARRAY_SIZE];
	}


	// Always Fails - Aliasing Index 4096*2 = 8192
	for(i = 0; i < ITERATION_SIZE/100; i++) {
		dummy += array_three[(i*32) % ARRAY_SIZE];
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
        __asm__("nop");
		dummy += array_four[(i*63) % ARRAY_SIZE];
	}
	

	// Estimated Direct Mapped - 800+1600+8192 = 10592
	// Direct Mapped - dl1.misses                    10636 # total number of misses


	return(dummy);
}

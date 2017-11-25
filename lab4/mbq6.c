// Stride Conformation

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 16 // 64B blocks
#define ARRAY_SIZE 32768
#define ITERATION_SIZE 40960

typedef struct node {
    struct node * nextNode;
} node;

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
    /*
    node * headNode = NULL;
    node * newNode = NULL;
    for(i = 0; i < 256; i++) {
        if(headNode == NULL) {
            headNode = (node *) malloc(sizeof(node));
            newNode = headNode;
        } else {
            newNode->nextNode = (node *) malloc(sizeof(node));
            newNode = newNode->nextNode;
        }
        j++;
        for(k = 0; k < j; k++) {
            malloc(sizeof(int));
        }
    }
    newNode->nextNode = headNode;
    */

	// Fails on wrap around 8192/16 = 256; 409600/512 = 800 times
	for(i = 0; i < ITERATION_SIZE; i++) {
        //newNode = newNode->nextNode;
        j = (int) pow(2, i % 15);
		dummy += array[j % ARRAY_SIZE];
        dummy += array_two[j % ARRAY_SIZE];
        dummy += array_three[j % ARRAY_SIZE];
        dummy += array_four[j % ARRAY_SIZE];
        dummy += array_five[j % ARRAY_SIZE];
        dummy += array_six[j % ARRAY_SIZE];
 
        //dummy2 += array_two[i*16 % ARRAY_SIZE];
        //dummy += array[(i+1)*16 % ARRAY_SIZE];
        //dummy += array[(i+1)*16 % ARRAY_SIZE];
	}

	// Fails on wrap around 8192/32 = 256; 409600/256 = 1600 times
	//for(i = 0; i < ITERATION_SIZE; i++) {
	//	dummy += array_two[(i*32) % ARRAY_SIZE];
	//}

    /*
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
    */
	

	// 800+1600+8192 = 10592
	//dl1.misses                    10636 # total number of misses


	return(dummy);
}

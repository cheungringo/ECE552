#include "predictor.h"
#define PERCEPTRON_HISTORY 36
#define PERCEPTRON_HISTORY_MASK 0x0fffffffff
#define PERCEPTRON_TABLE_LENGTH 512
#define PERCEPTRON_TABLE_MASK 0x01ff
/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

int pred_table_2bitsat[4096]; //initiaize to weak not-taken
void InitPredictor_2bitsat(){
	int i;
	for (i=0;i<4096;i++){
		pred_table_2bitsat[i] = 1;
	}
}

bool GetPrediction_2bitsat(UINT32 PC) {
  	// take lowest 12 bits first
  	int index = PC & 0x0fff;
 	int prediction = pred_table_2bitsat[index];
	if (prediction <= 1){
		return NOT_TAKEN;
  	}
	return TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	int index = PC & 0x0fff;
	if (resolveDir == TAKEN) {
		if (++pred_table_2bitsat[index] > 3){
			pred_table_2bitsat[index] = 3;
		}	
	}
	else {
		if (--pred_table_2bitsat[index] < 0){
			pred_table_2bitsat[index] = 0; 
		}
	}
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////
int BHR_2level[512]; // initialize history to TNTNTN
int PHT_2level[8][64]; // initialize history to weak not-taken
void InitPredictor_2level() {
	int i, j;
	for (i=0;i<8;i++){
		for(j=0;j<64;j++){
			PHT_2level[i][j] = 0x1;
		}
	}
	for (i=0;i<512;i++){
		BHR_2level[i] = 0x0;
	} 
}

bool GetPrediction_2level(UINT32 PC) {
	int PHT_index = PC & 0x00000007; // get lowest three bits (0:2)
  	int BHT_index = (PC >> 3) & 0x000001ff; // get bits 3:11 
  	int PHT_entry = BHR_2level[BHT_index];
  	int prediction = PHT_2level[PHT_index][PHT_entry];
  	if (prediction <= 1){
		return NOT_TAKEN;
  	}
  	return TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	int PHT_index = PC & 0x00000007; // get lowest three bits (0:2)
        int BHT_index = (PC >> 3) & 0x000001ff; // get bits 3:11
	int PHT_entry = BHR_2level[BHT_index];
	if (resolveDir == TAKEN) {
		if (++PHT_2level[PHT_index][PHT_entry] > 3){
			PHT_2level[PHT_index][PHT_entry] = 3;
		}
		// shift in a 1 (assume msb is oldest, lsb is youngest)
		BHR_2level[BHT_index] = ((BHR_2level[BHT_index] << 1) + 0x00000001) & 0x0000003f; // only keep the lowest 6 bits
	}
	else {
		if (--PHT_2level[PHT_index][PHT_entry] < 0){
			PHT_2level[PHT_index][PHT_entry] = 0;
		}
		// shift in a 0 to the lsb
		BHR_2level[BHT_index] = (BHR_2level[BHT_index] << 1) & 0x0000003f;  
	}
}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////
/* gshare, gets 7.53 average
int global_history = 0;
int bht[65536];
void InitPredictor_openend() {
	int i;
	for (i=0;i<65536;i++){
		bht[i] = 1;
	}
}

bool GetPrediction_openend(UINT32 PC) {
	int index = ((PC >> 1)^ global_history) & 0x0000ffff;
	int prediction = bht[index];
	if (prediction <= 1){
		return NOT_TAKEN;
	}
	return TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	int index = ((PC >> 1) ^ global_history) & 0x0000ffff;
	if (resolveDir == TAKEN){
		if (++bht[index] > 3){
			bht[index] = 3;
		}
		global_history = ((global_history << 1) + 0x1) & 0x0000ffff;
	}
	else {
		if (--bht[index] < 0){
			bht[index] = 0;
		}
		global_history = (global_history << 1) & 0x0000ffff;
	}
}
*/
long long global_history = 0; // this needs to be 36 bits, oldest is at bit 35, youngest at bit 0
int perceptron_table[PERCEPTRON_TABLE_LENGTH][PERCEPTRON_HISTORY]; // 512*36*7 < 128k (7 bits each weight because 1+log2(83)
int threshold = 83; // computed using floor(1.93*num_history_bits + 14) 
void InitPredictor_openend() {
	// initialize all the perceptron weights to 0
	int i, j;
	for (i=0;i<PERCEPTRON_TABLE_LENGTH;i++){
		for(j=0;j<PERCEPTRON_HISTORY;j++){
			perceptron_table[i][j] = 0;
		}
	}
}

int compute_y(int *perceptron, long long history){
	int weighted_sum = 1;
	int i, x;
	for (i=PERCEPTRON_HISTORY-1;i>-1;i--){
		x = ((history&0x1) == 1) ? 1 : -1;
		weighted_sum += perceptron[i]*x;
		history = history >> 1;
	}
	return weighted_sum;
}

bool GetPrediction_openend(UINT32 PC) {
	int index = ((PC >> 2)^(global_history&0xffffffff)) & PERCEPTRON_TABLE_MASK;
	int *perceptron = perceptron_table[index];
	int y = compute_y(perceptron, global_history);
	if (y < 0){
		return NOT_TAKEN;
	}
	return TAKEN;
}

void train_perceptron(int *perceptron, long long history, int t){
	int i, x;
	for (i=PERCEPTRON_HISTORY-1;i>-1;i--){
		x = ((history&0x1)==1) ? 1 : -1;
		perceptron[i] = perceptron[i] + t*x;
		if(perceptron[i] > 64) { perceptron[i] = 64; }
		if(perceptron[i] < -64) { perceptron[i] = -64; }
		history = history >> 1;
	}
}
void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	int index = ((PC >> 2)^(global_history&0xffffffff)) & PERCEPTRON_TABLE_MASK;
	int *perceptron = perceptron_table[index];
	int y = compute_y(perceptron, global_history);
	int abs_y = (y < 0) ? -1*y : y;
	if (resolveDir != predDir || abs_y <= threshold){
		int t = (resolveDir == TAKEN) ? 1 : -1;
		train_perceptron(perceptron, global_history, t);
	}
	if (resolveDir == TAKEN) {
		global_history = ((global_history << 1)+0x1) & PERCEPTRON_HISTORY_MASK; // only keep the lowest 36 bits of history
	}
	else {
		global_history = (global_history << 1) & PERCEPTRON_HISTORY_MASK;
	}
}

 
	

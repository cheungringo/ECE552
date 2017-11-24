
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE         10

#define RESERV_INT_SIZE    4
#define RESERV_FP_SIZE     2
#define FU_INT_SIZE        2
#define FU_FP_SIZE         1

#define FU_INT_LATENCY     4
#define FU_FP_LATENCY      9

/* IDENTIFYING INSTRUCTIONS */

//unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                         MD_OP_FLAGS(op) & F_UNCOND)

//conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

//floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

//intege computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

//load instruction
#define IS_LOAD(op)  (MD_OP_FLAGS(op) & F_LOAD)

//store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

//trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP) 

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

//prints info about an instruction
#define PRINT_INST(out,instr,str,cycle)	\
  myfprintf(out, "%d: %s", cycle, str);		\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

#define PRINT_REG(out,reg,str,instr) \
  myfprintf(out, "reg#%d %s ", reg, str);	\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

/* VARIABLES */

//instruction queue for tomasulo
static instruction_t* instr_queue[INSTR_QUEUE_SIZE];
//number of instructions in the instruction queue
static int instr_queue_size = 0;

//common data bus
static instruction_t* commonDataBus = NULL;

//The map table keeps track of which instruction produces the value for each register
static instruction_t* map_table[MD_TOTAL_REGS];

//the index of the last instruction fetched
static int fetch_index = 1;

/* RESERVATION STATIONS */

typedef struct my_reservation_station
{
	instruction_t *source_instrs[3]; // which three possible sources we have, NULL if not waiting on an instruction
	instruction_t *instr; // the instruction occupying this reservation station
	int ready_cycle; // the earliest cycle that this reservation station can execute, needed to make sure RAW dependencies don't execute a cycle early
	int functional_unit_index; // the index of the functional unit that is executing the instruction occupying this reservation station. -1 if empty.
	int fp; // 1 if this reservation station is a fp reservation station 0 otherwise
	int free; // 1 if this reservation station is free, 0 otherwise. Equivalent to checking if this->instr is NULL
}reservation_station;

// use these instead of the ones they give, makes coding simpler. The first RESERV_FP_SIZE will be for floating points, the rest for ints
static reservation_station *reservation_stations[RESERV_FP_SIZE + RESERV_INT_SIZE];

/* FUNCTIONAL UNITS */

typedef struct my_functional_unit
{
	int fp; // 1 if this functional unit is for floating points, 0 if for integers
	int free; // 1 if free, 0 if not free
	reservation_station *reservation_station; // a pointer to the reservation station containing the instruction that this functional unit is executing
	instruction_t *instr; // the instruction that this functional unit is executing, unnecessary since we can go through the reservation station
}functional_unit;

static functional_unit *functional_units[FU_FP_SIZE + FU_INT_SIZE];

// returns 1 if a reservation station is ready to execute, -1 if it is not
int is_ready(reservation_station* station){
	int i;
	for (i=0;i<3;i++){
		if (station->source_instrs[i] != NULL){
			return -1;
		}
	}
	return 1;
}

/* 
 * Description: 
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static int is_simulation_done(counter_t sim_insn) {
	// check if all the data structures are empty
	int i;
	if (fetch_index < sim_insn || instr_queue_size != 0){
		return -1;
	}
	for (i=0;i<RESERV_FP_SIZE+RESERV_INT_SIZE;i++){
		if (reservation_stations[i]->free == -1){
			return -1;
		}
	}
	for (i=0;i<FU_FP_SIZE+FU_INT_SIZE;i++){
		if (functional_units[i]-> free == -1){
			return -1;
		}
	}
	return 1;
}

// when an instruction finishes executing, broadcasts its destination registers to all reservation stations and sets their ready_cycle to current_cycle+1 to ensure they can only execute on the next cycle
void broadcast(instruction_t *instr, int current_cycle){
	int i, j;
	for (i=0;i<RESERV_FP_SIZE+RESERV_INT_SIZE;i++){
		for (j=0;j<3;j++){
			if (reservation_stations[i]->source_instrs[j] == instr){
				reservation_stations[i]->source_instrs[j] = NULL;
				reservation_stations[i]->ready_cycle = current_cycle + 1;
			}
		}
	}
}

/* allocates a reservation station to the instruction instr. 
 * returns 1 if the allocation was successful (there is a free reservation station of correct type
 * return -1 if the allocation was unsuccessful (there are no free reservation stations)
 */
int allocateReservationStation(instruction_t *instr, int is_int, int current_cycle){
	int source, j, destination;
	int i = (is_int == 1) ? RESERV_FP_SIZE : 0;
	int upper = (is_int == 1) ? RESERV_FP_SIZE+RESERV_INT_SIZE : RESERV_FP_SIZE;
	for (;i<upper;i++){
		if (reservation_stations[i]->free == 1){
			reservation_stations[i]->instr = instr;
			// check map table for source registers
			for (j=0;j<3;j++){
				source = instr->r_in[j];
				if (source > -1 && source < MD_TOTAL_REGS){
					reservation_stations[i]->source_instrs[j] = map_table[source];
				}
			}
			// write to the map table the destination registers so future instructions know what they're waiting for
			for (j=0;j<2;j++){
				destination = instr->r_out[j];
				if (destination > -1 && destination < MD_TOTAL_REGS){
					map_table[destination] = instr;
				}
			}
			reservation_stations[i]->free = -1;
			if (current_cycle == 19378){
				PRINT_INST(stdout, instr, "", current_cycle);
				printf("to reservation station: %d\n", i);
			}
			return 1;
		}
	}
	return -1;
}

// deallocate a functional unit and reservation station when an instruction finishes executing
void deallocate(instruction_t *instr, int fu_index, int current_cycle){
	int i;
	reservation_station *station = functional_units[fu_index]->reservation_station;
	for (i=0;i<3;i++){
		station->source_instrs[i] = NULL;
	}
	station->free = 1;
	station->ready_cycle = current_cycle; // this val can be anything 
	station->functional_unit_index = -1;
	functional_units[fu_index]->free = 1;
	functional_units[fu_index]->reservation_station = NULL;
	functional_units[fu_index]->instr = NULL;
}

/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	1 if CDB was written to or no instruction is ready, 0 if not. This is necessary since 
 *  there can be multiple instructions finishing execution in the same cycle so long as only
 *  one is writing to the cdb.
 */
void execute_To_CDB(int current_cycle) {
	int i, num_cycles, r_out, fu_index;
	int oldest = INT_MAX;
	instruction_t *instr = NULL, *oldest_instr = NULL; 
	int cdb_written_to = -1;
	while (cdb_written_to != 1){
		for (i=0;i<FU_FP_SIZE+FU_INT_SIZE;i++){
			instr = functional_units[i]->instr;
			if (instr != NULL){
				num_cycles = current_cycle - instr->tom_execute_cycle;
				if ((i >= FU_FP_SIZE && num_cycles >= 4) || (i < FU_FP_SIZE && num_cycles >= 9)){
					if (instr->index < oldest){
						oldest = instr->index;
						oldest_instr = instr;
						fu_index = i;
					}
				}
			}
		} // now we have the oldest instruction in the functional units
		if (oldest_instr == NULL){
			return;
		}
		if (WRITES_CDB(oldest_instr->op)){
			commonDataBus = oldest_instr;
			broadcast(oldest_instr, current_cycle);
			// clear map_table
			for (i=0;i<2;i++){
				r_out = oldest_instr->r_out[i];
				if (r_out > -1 && r_out < MD_TOTAL_REGS && map_table[r_out] == oldest_instr){
					map_table[r_out] = NULL;
				}
			}
			oldest_instr->tom_cdb_cycle = current_cycle;
			deallocate(oldest_instr, fu_index, current_cycle);
			cdb_written_to = 1;
		}
		else { // if we find that the oldest instruction done executing does not write to cdb, we can keep completing instructions
			deallocate(oldest_instr, fu_index, current_cycle);
		}
		oldest_instr = NULL;
		oldest = INT_MAX;
	}
}

// looks for a free functional unit, returns the index of the appropriate free functional unit, -1 if no free functional unit
int get_free_fu(int is_int){
	int i = (is_int == 1) ? FU_FP_SIZE : 0;
	int upper = (is_int == 1) ? FU_FP_SIZE + FU_INT_SIZE : FU_FP_SIZE;
	functional_unit * unit;
	for (;i<upper;i++){
		unit = functional_units[i];
		if (unit->free == 1){
			return i;
		}
	}
	return -1;
}

void execute_oldest_instructions(int lower, int upper, int is_int, int current_cycle){
	int i, fu, station_index;
	instruction_t *oldest_instr = NULL;
	int oldest_index = INT_MAX;
	reservation_station *station;
	while (1){
		for (i=lower;i<upper;i++){
			station = reservation_stations[i];
			if (station->free == -1 && is_ready(station) == 1 && current_cycle >= station->ready_cycle && station->functional_unit_index == -1){
				if (station->instr->index < oldest_index){
					oldest_index = station->instr->index;
					oldest_instr = station->instr;
					station_index = i;
				}
			}
		} // now we have the oldest instruction that is ready to execute
		if (oldest_instr == NULL){
			return;
		}
		fu = get_free_fu(is_int);
		if (fu == -1){
			return;
		}
		station = reservation_stations[station_index];
		station->functional_unit_index = fu;
		functional_units[fu]->reservation_station = station;
		functional_units[fu]->free = -1;
		functional_units[fu]->instr = oldest_instr;
		oldest_instr->tom_execute_cycle = current_cycle;
		oldest_instr = NULL;
		oldest_index = INT_MAX;
	}
}
/* 
 * Description: 
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void issue_To_execute(int current_cycle) {
	execute_oldest_instructions(0, RESERV_FP_SIZE, -1, current_cycle);
	execute_oldest_instructions(RESERV_FP_SIZE, RESERV_FP_SIZE+RESERV_INT_SIZE, 1, current_cycle);
}

/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t* trace) {
	if (instr_queue_size == INSTR_QUEUE_SIZE || fetch_index > sim_num_insn){
		return;
	}
	instruction_t* next_instr = get_instr(trace, fetch_index++);
	while (IS_TRAP(next_instr->op)){
		next_instr = get_instr(trace, fetch_index++);
	}	
	instr_queue[instr_queue_size++] = next_instr;
}
		
/* 
 * Description: 
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {
	fetch(trace);
	if (instr_queue_size == 0){
		return;
	}
	instruction_t *next_instr = instr_queue[instr_queue_size-1];
	if (next_instr->tom_dispatch_cycle == 0){
		next_instr->tom_dispatch_cycle = current_cycle;
	}
}

void dispatch_To_issue(instruction_trace_t *trace, int current_cycle){
	int i;
	int dispatched = -1;
	if (instr_queue_size == 0){
		return;
	}
	instruction_t *next_instr = instr_queue[0];
	// take jumps off IFQ immediately
	if (IS_UNCOND_CTRL(next_instr->op) || IS_COND_CTRL(next_instr->op)){
		for (i=0;i<instr_queue_size;i++){
			instr_queue[i] = instr_queue[i+1];
		}
		instr_queue_size--;
		return;
	}
	if (IS_ICOMP(next_instr->op) || IS_LOAD(next_instr->op) || IS_STORE(next_instr->op)){
		dispatched = allocateReservationStation(next_instr, 1, current_cycle);
	}
	else if (IS_FCOMP(next_instr->op)){
		//printf("FCOMP instruction on cycle %d\n", current_cycle);
//		exit(0);
		dispatched = allocateReservationStation(next_instr, -1, current_cycle);
	}
	else {
		dispatched = 1;
		printf("encountered some unknown instruction type, just going to dispatch");
	}
	if (dispatched == 1){
		next_instr->tom_issue_cycle = current_cycle;
		for (i=0;i<instr_queue_size;i++){
			instr_queue[i] = instr_queue[i+1];
		}
		instr_queue_size--;
	}
}

void print_map_table(int current_cycle){
	printf("Map table for current cycle: %d\n", current_cycle);
	int i;
	for (i=0;i<6;i++){
		printf("%d: %p ", i, map_table[i]);
	}
	printf("\n");
}
/* 
 * Description: 
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 * Extra Notes:
 * 	sim_num_insn: the number of instructions in the trace
 */
counter_t runTomasulo(instruction_trace_t* trace)
{
	//initialize instruction queue
	int i, j;
	for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
		instr_queue[i] = NULL;
	}

	//initialize reservation stations
	for (i = 0; i < RESERV_FP_SIZE + RESERV_INT_SIZE; i++) {
		reservation_station *station = malloc(sizeof(reservation_station));
		station->free = 1;
		station->instr = NULL;
		for (j=0;j<3;j++){
			station->source_instrs[j] = NULL;
		}
		station->fp = i < RESERV_FP_SIZE;
		station->functional_unit_index = -1;
		reservation_stations[i] = station;
	}

	//initialize functional units
	for (i = 0; i < FU_FP_SIZE+FU_INT_SIZE; i++) {
		functional_unit *unit = malloc(sizeof(functional_unit));
		unit->free = 1;
		unit->reservation_station = NULL;
		unit->fp = i < FU_FP_SIZE;
		unit->instr = NULL;
		functional_units[i] = unit;	
	}

	//initialize map_table to no producers
	int reg;
	for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
		map_table[reg] = NULL;
	}

	int cycle = 1;
	while (true) {
		execute_To_CDB(cycle);
		issue_To_execute(cycle);
		dispatch_To_issue(trace, cycle);
		fetch_To_dispatch(trace, cycle);
		if (commonDataBus != NULL){
			if (cycle > commonDataBus->tom_cdb_cycle){
				commonDataBus = NULL;
			}
		}
//		print_map_table(cycle);
		cycle++;
		if (is_simulation_done(sim_num_insn) == 1) break;
	}
//	print_all_instr(trace, sim_num_insn);
	for (i = 0; i < RESERV_FP_SIZE + RESERV_INT_SIZE; i++) {
		free(reservation_stations[i]);
	}

	for (i = 0; i < FU_FP_SIZE+FU_INT_SIZE; i++) {
		free(functional_units[i]);
	}
	
	return cycle;
}

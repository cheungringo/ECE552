
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

//integer computation
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
//static int instr_queue_size = 0;

//reservation stations (each reservation station entry contains a pointer to an instruction)
static instruction_t* reservINT[RESERV_INT_SIZE];
static instruction_t* reservFP[RESERV_FP_SIZE];

//functional units
static instruction_t* fuINT[FU_INT_SIZE];
static instruction_t* fuFP[FU_FP_SIZE];

//common data bus
static instruction_t* commonDataBus = NULL;

//The map table keeps track of which instruction produces the value for each register
static instruction_t* map_table[MD_TOTAL_REGS];

//the index of the last instruction fetched
static int fetch_index = 0;
bool startedSim = false;

// Instruction Queue of size INSTR_QUEUE_SIZE
instruction_t * instQueue [INSTR_QUEUE_SIZE];

// Counters to keep track of instruction queue
int headCounter;
int tailCounter;


// Helper function prototypes
void pushToReservation (instruction_t * inst, instruction_t ** reservationTable, int index, int cycle); 
bool checkDependency (instruction_t * inst, int current_cycle);
void issue_oldest_To_execute_INT(int current_cycle);
void issue_oldest_To_execute_FP(int current_cycle);
void RemoveFromReservationStation(instruction_t * removeInst);
void RemoveFromFunctionalUnit(instruction_t * removeInst);


/* 
 * Description: 
 * 	Pushes an instruction into a reservation station
 * Inputs:
 * 	inst: Instruction pointer
 *  reservationTable: Table to place insturction into
 *  index: Index of table of where instruction to move into
 *  cycle: Current Cycle
 * Returns:
 * 	None
 */
void pushToReservation (instruction_t * inst, instruction_t ** reservationTable, int index, int cycle) {
	int i = 0;
	for(i = 0; i < 3; i++) {
		if(inst->r_in[i] != DNA && inst->r_in[i] != 0) {
			inst->Q[i] = map_table[inst->r_in[i]];
		}
	}
	for(i = 0; i < 2; i++) {
		if(inst->r_out[i] != DNA && inst->r_out[i] != 0) {
			map_table[inst->r_out[i]] = inst;
		}
	}
	reservationTable[index] = inst;
	inst->tom_issue_cycle = cycle;
	return;
}


/* 
 * Description: 
 * 	Checks for dependencies in an instruction
 * Inputs:
 * 	inst: instruction to be checked for dependencies
 *  current_cycle: current clock cycle
 * Returns:
 * 	True: if no dependency
 */
bool checkDependency (instruction_t * inst, int current_cycle) {
  for(int i = 0; i < 3; i++) {
	// Check input registers if NULL
	if(inst->Q[i] != NULL) {
		return(false);
	}
  }
  return(true);
}


/* 
 * Description: 
 * 	Issues the oldest ALU instruction to an available INT functional unit
 * Inputs:
 *  current_cycle: Current Cycle
 * Returns:
 * 	None
 */
void issue_oldest_To_execute_INT(int current_cycle) {
  instruction_t * oldestInstruction = NULL;
  for(int i = 0; i < RESERV_INT_SIZE; i++) {
	if(reservINT[i] != NULL && reservINT[i]->tom_execute_cycle == 0) {
		if(checkDependency(reservINT[i], current_cycle) && (!oldestInstruction || oldestInstruction->index > reservINT[i]->index)) {
			oldestInstruction = reservINT[i];
		}
	}
  }
  if(oldestInstruction != NULL) {
	for(int i = 0; i < FU_INT_SIZE; i++) {
		if(fuINT[i] == NULL) {
			fuINT[i] = oldestInstruction;
			oldestInstruction->tom_execute_cycle = current_cycle;
			return;
		}
	}
  }
  return;
}


/* 
 * Description: 
 * 	Issues the oldest FP instruction to an available FU functional unit
 * Inputs:
 *  current_cycle: Current Cycle
 * Returns:
 * 	None
 */
void issue_oldest_To_execute_FP(int current_cycle) {
  instruction_t * oldestInstruction = NULL;
  for(int i = 0; i < RESERV_FP_SIZE; i++) {
	if (reservFP[i] != NULL && reservFP[i]->tom_execute_cycle == 0){
		if(checkDependency(reservFP[i], current_cycle) && (!oldestInstruction || oldestInstruction->index > reservFP[i]->index)) {
			oldestInstruction = reservFP[i];
		}
	}
  }
  if(oldestInstruction != NULL) {
	for(int i = 0; i < FU_FP_SIZE; i++) {
		if(fuFP[i] == NULL) {
			fuFP[i] = oldestInstruction;
			oldestInstruction->tom_execute_cycle = current_cycle;
			return;
		}
	}
  }
  return;
}


/* 
 * Description: 
 * 	Clears the reservation stations of the removeInst
 * Inputs:
 *  removeInst: Instruction to be removed from Reservation Stations
 * Returns:
 * 	None
 */
void RemoveFromReservationStation(instruction_t * removeInst) {
  for(int i = 0; i < RESERV_INT_SIZE; i++) {
	if(removeInst == reservINT[i]) { reservINT[i] = NULL; return; }
  }
  for(int i = 0; i < RESERV_FP_SIZE; i++) {
	if(removeInst == reservFP[i]) { reservFP[i] = NULL; return; }
  }
  return;
}


/* 
 * Description: 
 * 	Clears the functional units of the removeInst
 * Inputs:
 *  removeInst: Instruction to be removed from Functional Units
 * Returns:
 * 	None
 */
void RemoveFromFunctionalUnit(instruction_t * removeInst) {
  for(int i = 0; i < FU_INT_SIZE; i++) {
	if(removeInst == fuINT[i]) { fuINT[i] = NULL; return; }
  }
  for(int i = 0; i < FU_FP_SIZE; i++) {
	if(removeInst == fuFP[i]) { fuFP[i] = NULL; return; }
  }
  return;
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
static bool is_simulation_done(counter_t sim_insn) {

  /* ECE552: YOUR CODE GOES HERE */

  int i = 0;

  for(i = 0; i < RESERV_INT_SIZE; i++) {
	if(reservINT[i] != NULL) { return(false); }
  }
  for(i = 0; i < RESERV_FP_SIZE; i++) {
	if(reservFP[i] != NULL) { return(false); }
  }
  for(i = 0; i < FU_INT_SIZE; i++) {
	if(fuINT[i] != NULL) { return(false); }
  }
  for(i = 0; i < FU_FP_SIZE; i++) {
	if(fuFP[i] != NULL) { return(false); }
  }
  if(!startedSim) { return(false); }
  if(commonDataBus != NULL) { return(false); }
  if(fetch_index < sim_num_insn) { return(false); }
  if(headCounter != tailCounter) { return(false); }

  return true; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
}


/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

  // Check if common bus in use
	if (commonDataBus) {
	  // Remove dependencies in int reservation stations
	  for(int i = 0; i < RESERV_INT_SIZE; i++) {
		if(reservINT[i] != NULL) {
			for(int j = 0; j < 3; j++) {
				if(reservINT[i]->Q[j] == commonDataBus) {
					reservINT[i]->Q[j] = NULL;
				}
			}
		}
	  }
	  // Remove dependencies in fp reservation stations
	  for(int i = 0; i < RESERV_FP_SIZE; i++) {
		if(reservFP[i] != NULL) {
			for(int j = 0; j < 3; j++) {
				if(reservFP[i]->Q[j] == commonDataBus) {
					reservFP[i]->Q[j] = NULL;
				}
			}
		}
	  }
	  // Clear map_table if commonDataBus is still writing to it
	  for(int i = 0; i < 2; i++) {
		  if(commonDataBus->r_out[i] != DNA && map_table[commonDataBus->r_out[i]] == commonDataBus) {
			map_table[commonDataBus->r_out[i]] = NULL;
		  }
	  }  
	  commonDataBus = NULL; 
  }
}


/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void execute_To_CDB(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

  instruction_t * oldestFinished = NULL;

  // Check for completed INT ops
  for(int i = 0; i < FU_INT_SIZE; i++) {
	if((fuINT[i] != NULL) && (fuINT[i]->tom_execute_cycle != 0) && (fuINT[i]->tom_execute_cycle + FU_INT_LATENCY <= current_cycle)) {
		// If does not require CDB, can be immediately freed, otherwise must be retired in order
		if(!WRITES_CDB(fuINT[i]->op)) {
			RemoveFromReservationStation(fuINT[i]);
			RemoveFromFunctionalUnit(fuINT[i]);
		} else if(oldestFinished == NULL || oldestFinished->index > fuINT[i]->index) {
			oldestFinished = fuINT[i];
		}
	}
  }

  // Check for completed FP ops
  for(int i = 0; i < FU_FP_SIZE; i++) {
	if((fuFP[i] != NULL) && (fuFP[i]->tom_execute_cycle != 0) && (fuFP[i]->tom_execute_cycle + FU_FP_LATENCY <= current_cycle)) {
		// If does not require CDB, can be immediately freed, otherwise must be retired in order
		if(!WRITES_CDB(fuFP[i]->op)) {
			RemoveFromReservationStation(fuFP[i]);
			RemoveFromFunctionalUnit(fuFP[i]);
		} else if(oldestFinished == NULL || oldestFinished->index > fuFP[i]->index) {
			oldestFinished = fuFP[i];
		}
	}
  }

  // Oldest Completed instruction to be moved to CDB and removed from RS & FU
  if(oldestFinished != NULL && commonDataBus == NULL) {
	commonDataBus = oldestFinished;
	oldestFinished->tom_cdb_cycle = current_cycle;
	RemoveFromReservationStation(oldestFinished);
	RemoveFromFunctionalUnit(oldestFinished);
  }
  return;
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

  /* ECE552: YOUR CODE GOES HERE */

  // For each Int FU, attempt to execute into
  for(int i = 0; i < FU_INT_SIZE; i++) {
  	issue_oldest_To_execute_INT(current_cycle);
  }

  // For each FP FU, attempt to execute into
  for(int i = 0; i < FU_FP_SIZE; i++) {
	  issue_oldest_To_execute_FP(current_cycle);
  }
}


/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

  startedSim = true;
  instruction_t *instructionToIssue = NULL;

  // Pop Instruction Queue
  if(headCounter > tailCounter) {
	instructionToIssue = instQueue[tailCounter % INSTR_QUEUE_SIZE];
	tailCounter++;
  }
  if(instructionToIssue == NULL) {
	  return;
  }
 	
  enum md_opcode instOp = instructionToIssue->op;

  // Branch or control signal
  if(IS_COND_CTRL(instOp) || IS_UNCOND_CTRL(instOp) || instOp == 0) {
    startedSim = false; 
  	return;
  } else if(USES_FP_FU(instOp)) { // Floating point operation
	int i = 0;
	for(i = 0 ; i < RESERV_FP_SIZE; i++) {
		if(reservFP[i] == NULL) {
			pushToReservation(instructionToIssue, reservFP, i, current_cycle);
			return;
		}
	}
  } else if (USES_INT_FU(instOp)) { // Integer operation
	int i = 0;
	for(i = 0; i < RESERV_INT_SIZE; i++) {
		if(reservINT[i] == NULL) {
			pushToReservation(instructionToIssue, reservINT, i, current_cycle);	
			return;
		}
	}
  }
  // Push instruction queue - could not schedule
  tailCounter--;
  return;
}


/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t* trace, int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

 // Check if we can still fetch instructions, and buffer not full
 if(fetch_index <= sim_num_insn && headCounter-tailCounter < INSTR_QUEUE_SIZE) {
 	instruction_t * instructionToSchedule  = get_instr(trace, fetch_index);

 	// While NOP or TRAP, continue fetching
	while(fetch_index < sim_num_insn && (IS_TRAP(instructionToSchedule->op) || instructionToSchedule->op == 0)) {
		fetch_index++;
		instructionToSchedule = get_instr(trace, fetch_index);
	}

	// If Valid instruction, schedule
	if (fetch_index <= sim_num_insn)
	{
		instQueue[headCounter % INSTR_QUEUE_SIZE] = instructionToSchedule;
		instQueue[headCounter % INSTR_QUEUE_SIZE]->tom_dispatch_cycle = current_cycle;
		headCounter++;
		fetch_index++;
	}
 }
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
  
  /* ECE552: YOUR CODE GOES HERE */
  
  fetch(trace, current_cycle);
  return;
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
  headCounter = 0;
  tailCounter = 0;
  //initialize instruction queue
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
    instr_queue[i] = NULL;
  }

  //initialize reservation stations
  for (i = 0; i < RESERV_INT_SIZE; i++) {
      reservINT[i] = NULL;
  }

  for(i = 0; i < RESERV_FP_SIZE; i++) {
      reservFP[i] = NULL;
  }

  //initialize functional units
  for (i = 0; i < FU_INT_SIZE; i++) {
    fuINT[i] = NULL;
  }

  for (i = 0; i < FU_FP_SIZE; i++) {
    fuFP[i] = NULL;
  }

  //initialize map_table to no producers
  int reg;
  for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
    map_table[reg] = NULL;
  }
  
  int cycle = 1;
  while (true) {

     /* ECE552: YOUR CODE GOES HERE */
     if (is_simulation_done(sim_num_insn)) {
        break; 
     }

     CDB_To_retire(cycle);
     execute_To_CDB(cycle);
     issue_To_execute(cycle);
     dispatch_To_issue(cycle);
     fetch_To_dispatch(trace, cycle);

     cycle++;
  }
  return cycle;
}

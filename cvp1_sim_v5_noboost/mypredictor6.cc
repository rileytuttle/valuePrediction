#include <cinttypes>
#include "cvp.h"
#include <cassert>
#include <cstdio>
#include <stdio.h>
#include <cassert>
#include <string.h>
#include <inttypes.h>

using namespace std;
#include <stdlib.h>
#define INCLUDEPRED
#include "mypredictor.h"


my_predictor *mypred;
uint64_t stats[9];
bool branch_taken;
uint64_t pred_total = 0;
uint64_t pred_correct = 0;
bool getPrediction(uint64_t seq_no, uint64_t pc, uint8_t piece, uint64_t& predicted_value){
	uint8_t confidence = 0;
	confidence = mypred->getPrediction(seq_no, pc, piece, &predicted_value);
	pred_total++;
  return confidence >= 0;
}

void speculativeUpdate(uint64_t seq_no,    		// dynamic micro-instruction # (starts at 0 and increments indefinitely)
                       bool eligible,			// true: instruction is eligible for value prediction. false: not eligible.
		       uint8_t prediction_result,	// 0: incorrect, 1: correct, 2: unknown (not revealed)
		       // Note: can assemble local and global branch history using pc, next_pc, and insn.
		       uint64_t pc,
		       uint64_t next_pc,
		       InstClass insn,
		       uint8_t piece,
		       // Note: up to 3 logical source register specifiers, up to 1 logical destination register specifier.
		       // 0xdeadbeef means that logical register does not exist.
		       // May use this information to reconstruct architectural register file state (using log. reg. and value at updatePredictor()).
		       uint64_t src1,
		       uint64_t src2,
		       uint64_t src3,
		       uint64_t dst) {
	stats[insn]++;
	bool branch_instruction = insn == condBranchInstClass ||
														insn == uncondDirectBranchInstClass ||
														insn == uncondIndirectBranchInstClass;
	branch_taken = (branch_instruction && next_pc != pc+4);
	mypred->speculativeHistoryUpdate(seq_no, pc, insn, branch_taken, next_pc, piece);
}

void updatePredictor(uint64_t seq_no,		// dynamic micro-instruction #
		     uint64_t actual_addr,	// load or store address (0xdeadbeef if not a load or store instruction)
		     uint64_t actual_value,	// value of destination register (0xdeadbeef if instr. is not eligible for value prediction)
		     uint64_t actual_latency){	// actual execution latency of instruction
	uint64_t true_value;
	true_value = actual_addr == 0xdeadbeef?actual_value:actual_addr; 
	bool corr = mypred->update_predictor(seq_no,true_value);
	if(corr) pred_correct++;
}

void beginPredictor(int argc_other, char **argv_other){
  if (argc_other > 0)
    printf("CONTESTANT ARGUMENTS:\n");
  for (int i = 0; i < argc_other; i++)
    printf("\targv_other[%d] = %s\n", i, argv_other[i]);
  mypred = new my_predictor();
}
void endPredictor(){
  printf("CONTESTANT OUTPUT--------------------------\n");
 	for(int i = 0;i<9;i++)
 		printf("there were %" PRIu64 " of insn %ds\n",stats[i],i);
	printf("%" PRIu64 " correct predictions out of %" PRIu64 "\n",pred_correct,pred_total);
	printf("precentage = %" PRIu64 "\n",(pred_correct/pred_total)*1000);
  printf("--------------------------\n");
}

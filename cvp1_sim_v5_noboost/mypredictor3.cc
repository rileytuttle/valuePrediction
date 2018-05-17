// Author: A. Seznec March-April 2011

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
folded_history *myhist;


bool getPrediction(uint64_t seq_no, uint64_t pc, uint8_t piece, uint64_t& predicted_value){
  //get prediction based on history tables
  // check all components find the tradeoff between highest confidence and longest tags  
  //mypred->gindex(pc,  );
  int8_t confidence = mypred->predict_value(pc, &predicted_value);
  return confidence >= 1;
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
		       //if(prediction_result != 2) mypred->FetchHistoryUpdate(pc, insn, prediction_result == 1, piece);
		       //mypred->update_brindirect(pc, insn, prediction_result == 1, piece);


}

void updatePredictor(uint64_t seq_no,		// dynamic micro-instruction #
		     uint64_t actual_addr,	// load or store address (0xdeadbeef if not a load or store instruction)
		     uint64_t actual_value,	// value of destination register (0xdeadbeef if instr. is not eligible for value prediction)
		     uint64_t actual_latency) {	// actual execution latency of instruction

}

void beginPredictor(int argc_other, char **argv_other){
  if (argc_other > 0)
    printf("CONTESTANT ARGUMENTS:\n");
  for (int i = 0; i < argc_other; i++)
    printf("\targv_other[%d] = %s\n", i, argv_other[i]);
  mypred = new my_predictor();
  assert(mypred);
}
void endPredictor(){
  printf("CONTESTANT OUTPUT--------------------------\n");
  printf("--------------------------\n");
}

#ifdef INCLUDEPRED
/* 
Code has been largely inspired  by the tagged PPM predictor simulator from Pierre Michaud, the OGEHL predictor simulator from by André Seznec, the TAGE predictor simulator from  André Seznec and Pierre Michaud, the L-TAGE simulator by Andre Seznec
*/

#include <inttypes.h>
#include <math.h>
#define NHIST 15 /*number of tagged tables*/
//#define SHARINGTABLES /* share some interleaved tables*/
#define INITHISTLENGTH /* use tuned history lengths*/
/*list of the history length*/
int m[NHIST+1]={0, 0, 10, 16, 27, 44, 60, 96, 109, 219, 449, 487, 714, 1313, 2146, 3881};

     
#define IUM
//if a matching  entry corresponds to an inflight branch  use the real target of this inflight branch 

#define LOGG 12

#ifndef SHARINGTABLES
#define TBITS 7
#endif
#ifndef INITHISTLENGTH
#define MINHIST 8		// shortest history length
#define MAXHIST 2000
#endif


//#define DEBUG
#ifdef DEBUG
#define DEBUG1
#define DEBUG2
#define DEBUG3
#define DEBUG4
#define DEBUG6
#endif

int TICK;			//control counter for the resetting of useful bits
#define LOGTICK  6		//for management of the reset of useful counters
#define HISTBUFFERLENGTH 4096
//size of the history circular  buffer

// utility class for index computation
class folded_history{
// this is the cyclic shift register for folding 
// a long global history into a smaller number of bits; see P. Michaud's PPM-like predictor at CBP-1
public:
  unsigned comp;
  int CLENGTH;
  int OLENGTH;
  int OUTPOINT;
  folded_history(){
  }
  void init(int original_length, int compressed_length){
    comp = 0;
    OLENGTH = original_length;
    CLENGTH = compressed_length;
    OUTPOINT = OLENGTH % CLENGTH;
  }
  void update(uint8_t * h, int PT){
    comp = (comp << 1) | h[PT & (HISTBUFFERLENGTH - 1)];
    comp ^= h[(PT + OLENGTH) & (HISTBUFFERLENGTH - 1)] << OUTPOINT;
    comp ^= (comp >> CLENGTH);
    comp &= (1 << CLENGTH) - 1;
  }
};
//class for storing speculative predictions: i.e. provided by a table entry that has already provided a still speculative prediction
class specentry{
public:
  int32_t tag;
  uint64_t pred;
  specentry(){//nothing
	}
};
class gentry{			// ITTAGE global table entry{
public:
  int8_t ctr;			// 2 bits 
  uint16_t tag;			//width dependent on the table
  uint64_t target;		//25 bits (18 offset + 7-bit region pointer)
  int8_t u;			//1 bit
  gentry(){
    ctr = 0;
    tag = 0;
    u = 0;
    target = 0;
  }
};
#define UNUPDATEDENTRIES 10
#define LOGUNUPDATEDENTRIES 4
class unupdatedEntry{
public:
	uint64_t pc;
	uint8_t piece;
	uint64_t predicted_value;
	uint8_t result;
	bool ifAlt;
	int8_t confidence;
	unupdatedEntry(uint64_t pc, uint8_t piece, uint64_t predicted_value, bool ifAlt, int8_t confidence){
		this->pc = pc;
		this->piece = piece; 
		this->predicted_value = predicted_value;
		this->result = 0;
		this->ifAlt = ifAlt;
		this->confidence = confidence;
	}
};
unupdatedEntry *unupdated[3]; // at most 3 unupdated entries
class regionentry{		// ITTAGE target region  table entry{
public:
	uint16_t region; // 14 bits
  int8_t u; //1 bit
  regionentry(){
    region = 0;
    u = 0;
  }
};
int8_t USE_ALT_ON_NA;		// "Use alternate prediction on weak predictions":  a 4-bit counter  to determine whether the newly allocated entries should be considered as  valid or not for delivering  the prediction


//for managing global history  and path

uint8_t ghist[HISTBUFFERLENGTH];
//for management at fetch time
int Fetch_ptghist;
folded_history Fetch_ch_i[NHIST + 1];	//utility for computing TAGE indices
folded_history Fetch_ch_t[2][NHIST + 1];	//utility for computing TAGE tags

// for management at retire time
int Retire_ptghist;
folded_history Retire_ch_i[NHIST + 1];	//utility for computing TAGE indices
folded_history Retire_ch_t[2][NHIST + 1];	//utility for computing TAGE tags

//predictor tables
gentry *gtable[NHIST + 1];	// ITTAGE tables (T0 has no tags other tables are tagged) 
regionentry *rtable; // Target region tables 

int TB[NHIST + 1];		// tag width for the different tagged tables
int logg[NHIST + 1];		// log of number entries of the different tagged tables
int GI[NHIST + 1] ;		// indexes to the different tables are computed only once  
int GTAG[NHIST + 1];		// tags for the different tables are computed only once  

uint64_t pred_taken;		// prediction
uint64_t alttaken;		// alternate  TAGEprediction
uint64_t tage_pred;		// TAGE prediction
int HitBank;			// longest matching bank
int AltBank;			// alternate matching bank
uint64_t LongestMatchPred;

int Seed;			// for the pseudo-random number generator

//for the IUM
int PtIumRetire;
int PtIumFetch;
specentry *IUMPred;
#define LOGSPEC 6

InstClass last_insn = undefInstClass;
bool last_taken = false;
uint64_t last_pc = 0;
uint64_t last_seq_no = 0;
uint64_t last_target = 0;
uint8_t last_piece = 0;
uint8_t last_prediction_result = 0;
bool updated = false;
int usedAlt = 0;


class my_predictor{
public:
  my_predictor(void){
  USE_ALT_ON_NA = 0;
  Seed = 0;
  PtIumRetire = 0;
  PtIumFetch = 0;
  for(int i = 0; i < HISTBUFFERLENGTH; i++)
  	ghist[0] = 0;
  Fetch_ptghist = 0;
  Retire_ptghist = 0;
  // initialize tag bit widths
  TB[0] = 0;			// table T0 is tagless
  TB[1] = 6;
  for(int i = 2; i <= NHIST; i++){
		TB[i] = (TBITS + i);
		if(TB[i] >= 16)
	  	TB[i] = 16;
  }
  // initialize the number of entries of each table
  // there are 1<<logg[i] entries in table i
  logg[0] = LOGG;
  logg[1]= LOGG-1; 
  for(int i = 2; i <= 4; i++)
    logg[i] = LOGG -2;
  for(int i = 5; i <= 13; i++)
    logg[i] = LOGG - 3;
  for(int i = 14; i <= NHIST; i++)
    logg[i] = LOGG - 4;
	//initialisation of index and tag computation functions
  for(int i = 1; i <= NHIST; i++){
		Fetch_ch_i[i].init (m[i], (logg[i]));
		Fetch_ch_t[0][i].init (Fetch_ch_i[i].OLENGTH, TB[i]);
		Fetch_ch_t[1][i].init (Fetch_ch_i[i].OLENGTH, TB[i] - 1);
  }
  // not sure what these are for
  for(int i = 1; i <= NHIST; i++){
		Retire_ch_i[i].init (m[i], (logg[i]));
		Retire_ch_t[0][i].init (Retire_ch_i[i].OLENGTH, TB[i]);
		Retire_ch_t[1][i].init (Retire_ch_i[i].OLENGTH, TB[i] - 1);
  }
  // hopefully I can get rid of whole region table thing
  rtable = new regionentry[128];
  IUMPred = new specentry[1 << LOGSPEC];
  //initilaizing each table with the right number of entries
  for(int i = 0; i <= NHIST; i++)
		gtable[i] = new gentry[1 << (logg[i])];
	}




	// the index functions for the tagged tables uses path history as in the OGEHL predictor
	// gindex computes a full hash of pc, ghist 
  int gindex(uint64_t seq_no, uint64_t pc, uint8_t piece, int bank, folded_history * ch_i){
    int index;
    uint64_t temp = (pc<<2)|(piece&3);
    index =
    	temp ^ (temp >> (abs (logg[bank] - bank) + 1)) ^
      ch_i[bank].comp;
    return (index & ((1 << (logg[bank])) - 1));
  }
  //  tag computation
  uint16_t gtag(uint64_t seq_no, uint64_t pc, uint8_t piece, int bank, folded_history * ch0,
		 folded_history * ch1){
		 uint64_t temp = (pc<<2)|(piece&3);
  	int tag = temp ^ ch0[bank].comp ^ (ch1[bank].comp << 1);
    return (tag & ((1 << TB[bank]) - 1));
  }

//just a simple pseudo random number generator: use available information
// to allocate entries  in the loop predictor
  int MYRANDOM(){
    Seed++;
    Seed ^= Fetch_ptghist;  
    Seed = (Seed >> 11) + (Seed << 21);
    Seed += Retire_ptghist;
    return (Seed);
  }
  int8_t Tagepred(){
  	#ifdef DEBUG0
  	fprintf(stdout,"////////tagepred\n");
  	#endif
    alttaken = 0;
    HitBank = -1; 
    AltBank = -1;
    usedAlt = 0;
    int8_t conf = 0;
		//Look for the bank with longest matching history
    for(int i = NHIST; i >= 0; i--){
			if(gtable[i][GI[i]].tag == GTAG[i]){
	    	HitBank = i;
	    	conf = gtable[i][GI[i]].ctr;
	    	break;
	  	}
   	}
		//Look for the alternate bank
		// alternate bank has second longest matching history
    for(int i = HitBank - 1; i >= 0; i--){
			if(gtable[i][GI[i]].tag == GTAG[i]){
	    	AltBank = i;
	    	break;
	  	}
    }
		//computes the prediction and the alternate prediction
    if(AltBank >= 0)
      alttaken = gtable[AltBank][GI[AltBank]].target;
    tage_pred = gtable[HitBank][GI[HitBank]].target;
    LongestMatchPred = tage_pred;
		//if the entry is recognized as a newly allocated entry and 
		//USE_ALT_ON_NA is positive  use the alternate prediction
    /*if(AltBank >= 0){
      if((USE_ALT_ON_NA >= 0) & (gtable[HitBank][GI[HitBank]].ctr == 0)){
				tage_pred = alttaken;
				conf = gtable[AltBank][GI[AltBank]].ctr;
				usedAlt = 1;
			}
		}*/
		#ifdef DEBUG3
		fprintf(stdout,"tage_pred=%" PRIx64 "\n", tage_pred);
		#endif
		return conf;
  }
  // PREDICTION
  int8_t getPrediction(uint64_t seq_no, uint64_t pc, uint8_t piece, uint64_t& prediction){
  	#ifdef DEBUG0
  	fprintf(stdout,"////////getprediction\n");
  	#endif
  	int8_t conf = 0;
		// computes the table addresses and the partial tags
		for(int i = 0; i <= NHIST; i++){
    	//GI[i] = gindex(seq_no, pc, piece, i,  Fetch_ch_i);
    	//GTAG[i] = gtag(seq_no, pc, piece, i, Fetch_ch_t[0], Fetch_ch_t[1]);
    	GI[i] = gindex(seq_no, pc, piece, i, Retire_ch_i);
    	GTAG[i] = gtag(seq_no, pc, piece, i, Retire_ch_t[0], Retire_ch_t[1]);
  	}
  	GTAG[0] = 0;
  	GI[0] = ((pc<<2)|(piece&3)) & ((1 << logg[0]) - 1);
  	conf = Tagepred();
  	prediction = tage_pred;
  	#ifdef DEBUG1
  	fprintf(stdout,"getprediction seq=%" PRIx64 " pc=%" PRIx64 " piece=%d\n",seq_no,pc,piece);
  	fprintf(stdout,"getprediction pc=%" PRIx64 " LongestMatch=%" PRIx64 " HitBank=%d tag=%d ind=%d conf=%d\n",
  						pc,LongestMatchPred,HitBank,GTAG[HitBank],GI[HitBank], gtable[HitBank][GI[HitBank]].ctr);
  	if(AltBank >= 0)
  		fprintf(stdout,"getprediction pc=%" PRIx64 " AltPrediction=%" PRIx64 " AltBank=%d tag=%d ind=%d conf=%d\n",
  						pc,alttaken,AltBank,GTAG[AltBank],GI[AltBank], gtable[AltBank][GI[AltBank]].ctr);
  	fprintf(stdout,"getprediction pc=%" PRIx64 " usedAlt = %d\n", pc,usedAlt);
  	#endif
  	unupdated[seq_no%3] = new unupdatedEntry(pc,piece,tage_pred,usedAlt,conf);
		return conf;
  }
	//  UPDATE  FETCH HISTORIES  
	void speculativeHistoryUpdate(uint64_t seq_no, uint64_t pc, InstClass insn, bool taken,
			     uint64_t target, uint8_t piece, uint8_t prediction_result){
  	#ifdef DEBUG0
		fprintf(stdout,"////////spec update\n");
		#endif
		if(last_pc != pc){
			updated = true;
			last_seq_no = seq_no;
			last_pc = pc;
			last_insn = insn;
			last_taken = taken;
			last_target = target;
			last_piece = piece;
			last_prediction_result = prediction_result;
			HistoryUpdate(seq_no, pc, piece, insn, taken, target, Fetch_ptghist,
		     Fetch_ch_i, Fetch_ch_t[0], Fetch_ch_t[1]);
		}
  }
	void HistoryUpdate(uint64_t seq_no, uint64_t pc, uint8_t piece, InstClass insn, bool taken,
			uint64_t target,  int &Y, folded_history * H,
			folded_history * G, folded_history * J){
		uint64_t temp = (pc<<2)|(piece&3);
  	int maxt = (insn == uncondIndirectBranchInstClass) ? 10 : 0;
    int PATH = ((target ^ (target >> 3) ^ temp));
   	if(insn == condBranchInstClass)
    	PATH = taken;  
    for(int t = 0; t < maxt; t++){
	  	bool P = (PATH  & 1);
	  	PATH >>= 1; 
			//update  history
	  	Y--;
	  	ghist[Y & (HISTBUFFERLENGTH - 1)] = P;     
			//prepare next index and tag computations for user branchs 
	  	for(int i = 1; i <= NHIST; i++){
      	H[i].update (ghist, Y);
      	G[i].update (ghist, Y);
      	J[i].update (ghist, Y);
	  	}
		}
		//END UPDATE FETCH HISTORIES
  }
	// PREDICTOR UPDATE
  bool update_predictor(uint64_t seq_no, uint64_t target){
		#ifdef DEBUG0
  	fprintf(stdout,"////////update_predictor\n");
  	#endif
		int8_t conf;
		bool corr = false;
  	int NRAND = MYRANDOM();
		//Recompute  the prediction by the ITTAGE predictor: on an effective implementation one would try to avoid this recomputation by propagating the prediction with the branch instruction
		for(int i = 0; i <= NHIST; i++){
			GI[i] = gindex(seq_no, unupdated[seq_no%3]->pc, unupdated[seq_no%3]->piece, i, Retire_ch_i);
  		GTAG[i] = gtag(seq_no, unupdated[seq_no%3]->pc, unupdated[seq_no%3]->piece, i, Retire_ch_t[0], Retire_ch_t[1]);
  	}
  	GTAG[0] = 0;
  	GI[0] = (((unupdated[seq_no%3]->pc)<<2)|((unupdated[seq_no%3]->piece)&3)) & ((1 << logg[0]) - 1);
  	conf = Tagepred();
		corr = unupdated[seq_no%3]->predicted_value == target;
  	bool ALLOC = (LongestMatchPred != target && target != 0xdeadbeef);
  	#ifdef DEBUG1
  	fprintf(stdout,"pc=%" PRIx64 " corr=%d alttaken=%" PRIx64 "\n",
  					unupdated[seq_no%3]->pc,corr,alttaken);
  	#endif
  	if(gtable[0][GI[0]].target == target){
  		if(gtable[0][GI[0]].ctr<3)
  			gtable[0][GI[0]].ctr++;
  	} else {
  		if(gtable[0][GI[0]].ctr>0)
  			gtable[0][GI[0]].ctr--;
  		else
  			gtable[0][GI[0]].target = target;
  	}
  	if(alttaken > 0 && AltBank>0){
  		if(alttaken == target){
  			if(gtable[AltBank][GI[AltBank]].ctr<3)
  				gtable[AltBank][GI[AltBank]].ctr++;
  		} else {
  			if(gtable[AltBank][GI[AltBank]].ctr>0)
  				gtable[AltBank][GI[AltBank]].ctr--;
  		}
  	}
  	if(LongestMatchPred == target){
  		if(gtable[HitBank][GI[HitBank]].ctr<3)
  			gtable[HitBank][GI[HitBank]].ctr++;
  	} else {
  		if(gtable[HitBank][GI[HitBank]].ctr>0)
  			gtable[HitBank][GI[HitBank]].ctr--;
  	}
		//allocation if the Longest Matching entry does not provide the correct entry
  	if((HitBank > 0) & (AltBank >= 0)){
			// Manage the selection between longest matching and alternate matching
			// for "pseudo"-newly allocated longest matching entry
    	bool PseudoNewAlloc = (gtable[HitBank][GI[HitBank]].ctr == 0);
    	if(PseudoNewAlloc){
	  		if(alttaken){
	    		if(LongestMatchPred != alttaken){
						if((alttaken == target)
		    				|| (LongestMatchPred == target)){
		    			if(alttaken == target){
								if(USE_ALT_ON_NA < 7)
			  					USE_ALT_ON_NA++;
		      		}else{
								if(USE_ALT_ON_NA >= -8)
			  					USE_ALT_ON_NA--;
		      		}
		  			}
	      	}
	    	}
			}
  	}
  	#ifdef DEBUG2
		fprintf(stdout,"updatepredictor seq=%" PRIx64 " pc=%" PRIx64 " piece=%d prediction=%" PRIx64 "\n",
						seq_no,unupdated[seq_no%3]->pc,unupdated[seq_no%3]->piece,unupdated[seq_no%3]->predicted_value);
  	fprintf(stdout,"updatepredictor pc=%" PRIx64 " LongestMatch=%" PRIx64 " HitBank=%d tag=%d ind=%d conf=%d\n",
  			unupdated[seq_no%3]->pc,tage_pred,HitBank,GTAG[HitBank],GI[HitBank], gtable[HitBank][GI[HitBank]].ctr);
  	if(AltBank >=0)
  		fprintf(stdout,"updatepredictor pc=%" PRIx64 " AltPrediction=%" PRIx64 " AltBank=%d tag=%d ind=%d conf=%d\n",
  					unupdated[seq_no%3]->pc,tage_pred,AltBank,GTAG[AltBank],GI[AltBank], gtable[AltBank][GI[AltBank]].ctr);
  	fprintf(stdout,"updatepredictor pc=%" PRIx64 " actual=%" PRIx64 " usealtonna=%d\n",
  				unupdated[seq_no%3]->pc, target, USE_ALT_ON_NA);
  	#endif
  	if(ALLOC){
			// we allocate an entry with a longer history
			//to  avoid ping-pong, we do not choose systematically the next entry, but among the 2 next entries
    	int Y = NRAND;
    	int X = HitBank + 1;
    	// choose two tables up with probability of 1/32
    	if((Y&31) == 0) X++;
    	int T = 2; //allocating 3  entries on a misprediction just work a little bit better than a single allocation
    	for(int i = X; i <= NHIST; i++){
	  		if(gtable[i][GI[i]].u == 0){
	    		gtable[i][GI[i]].tag = GTAG[i];
	      	gtable[i][GI[i]].target = target;
	      	gtable[i][GI[i]].ctr = 0;
	      	gtable[i][GI[i]].u = 0;
	      	if(TICK > 0) TICK--;
	      	if(T == 0) break;
	      	T--;
	      	i++;
	    	} else TICK++;
			}
  		//not sure what TICK is for
  		// something to do with resetting the usefullness bit
  		if((TICK >= (1 << LOGTICK))){
  			TICK = 0;
				// reset the useful  bit
    		for(int i = 0; i <= NHIST; i++)
					for(int j = 0; j < (1 << logg[i]); j++)
	  				gtable[i][j].u = 0;
  		}
  		if(LongestMatchPred == target){
  			if(gtable[HitBank][GI[HitBank]].ctr < 3)
					gtable[HitBank][GI[HitBank]].ctr++;
  		} else {
  			if(gtable[HitBank][GI[HitBank]].ctr > 0)
					gtable[HitBank][GI[HitBank]].ctr--;
    		else {// replace the target field by the new target
						// Need to compute the target field :-)
	  			gtable[HitBank][GI[HitBank]].target = target;
				}
			}
			// update the u bit
  		if(HitBank != 0){
    		if(LongestMatchPred != alttaken){
					if(LongestMatchPred == target){
	    			gtable[HitBank][GI[HitBank]].u = 1;
	  			}
    		}
    	}
			//END PREDICTOR UPDATE
			return corr;
		}
		//  UPDATE  RETIRE HISTORY PATH
		if(updated){  
  		HistoryUpdate(last_seq_no, last_pc, last_piece, last_insn,
  						last_taken, last_target,  Retire_ptghist,Retire_ch_i,
  						Retire_ch_t[0], Retire_ch_t[1]);
  		updated = false;
  	}
		//Retire_ptghist = Fetch_ptghist;
		//Retire_ch_i = Fetch_ch_i;
		//Retire_ch_t[0] = Fetch_ch_t[0];
		//Retire_ch_t[1] = Fetch_ch_t[1];
	}
};
#endif

#ifdef INCLUDEPRED
/* 
Code has been largely inspired  by the tagged PPM predictor simulator from Pierre Michaud, the OGEHL predictor simulator from by André Seznec, the TAGE predictor simulator from  André Seznec and Pierre Michaud, the L-TAGE simulator by Andre Seznec
*/

#include <inttypes.h>
#include <math.h>
#include <iostream>
#define NHIST 15 /*number of tagged tables*/
#define INITHISTLENGTH /* use tuned history lengths*/
/*list of the history length*/
int m[NHIST+1]={0, 0, 10, 16, 27, 44, 60, 96, 109, 219, 449, 487, 714, 1313, 2146, 3881};


#define DEBUG
#ifdef DEBUG
//#define DEBUG1
//#define DEBUG2
#define DEBUG3
//#define DEBUG4
//#define DEBUG6
//#define DEBUG7
#define DEBUG8
#endif

     
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
bool NOTAGMATCH = false;
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
	folded_history(){}
	void init(int original_length, int compressed_length){
		comp = 0;
		OLENGTH = original_length;
		CLENGTH = compressed_length;
		OUTPOINT = OLENGTH % CLENGTH;
	}
	void update(uint8_t *h, int PT){
		comp = (comp << 1) | h[PT & (HISTBUFFERLENGTH - 1)];
		comp ^= h[(PT + OLENGTH) & (HISTBUFFERLENGTH - 1)] << OUTPOINT;
		comp ^= (comp >> CLENGTH);
		comp &= (1 << CLENGTH) - 1;
	}
};
class gentry{			// value prediction global table entry
	public:
  	int8_t ctr;			// 2 bits 
 		int16_t tag;			//width dependent on the table
  	uint64_t value;		//64 bit predicted value
  	int8_t u;			//1 bit
    gentry(){
    	ctr = 0;
    	tag = 0;
    	u = 0;
    	value = 0;
  	}
};

//for managing global history  and path
uint64_t ghist;//[HISTBUFFERLENGTH];
uint64_t oldghist;

//predictor tables
gentry *gtable[NHIST + 1];	// ITTAGE tables (T0 has no tags other tables are tagged) 

int TB[NHIST + 1];		// tag width for the different tagged tables
int logg[NHIST + 1];		// log of number entries of the different tagged tables
int GI[NHIST + 1] ;		// indexes to the different tables are computed only once  
int GTAG[NHIST + 1];		// tags for the different tables are computed only once 
uint64_t numberofentries[NHIST]; // only need to keep track of tagged entries 
uint64_t pred_taken;		// prediction
uint64_t last_pc;
uint8_t prediction_result;
int HitBank;			// longest matching bank
int AltBank;			// alternate matching bank
uint64_t HitIndex; // index of hit in a table

int Seed;			// for the pseudo-random number generator

class my_predictor{
	public:
		my_predictor(void){
			Seed = 0;
			//for(int i = 0; i < HISTBUFFERLENGTH; i++)
			//	ghist[i] = 0;
			ghist = 0;
			oldghist = ghist;
			// initalizing tag bit widths
			for(int i = 2; i <= NHIST; i++){
				TB[i] = (TBITS + i);
				if(TB[i] >= 16)
					TB[i] = 16;
			}
			TB[0] = 0;			// table T0 is tagless
			TB[1] = 6;
			// each table is an array of entries that is 2^logg[i] long.
			// logg is an array of log(number of entries) for each table
			logg[0] = LOGG; //gtable(0) can have 2^12 entries
			logg[1]= LOGG-1; //gtable(1) can have 2^11 entries
			for(int i = 2; i <= 4; i++)
				logg[i] = LOGG -2;
			for(int i = 5; i <= 13; i++)
				logg[i] = LOGG - 3;
			for(int i = 14; i <= NHIST; i++)
				logg[i] = LOGG - 4;
			// make NHIST tables each with 2^logg[i] entries
			for(int i = 0; i <= NHIST; i++)
				gtable[i] = new gentry[1 << (logg[i])];
			for(int i = 0; i<NHIST; i++)
				numberofentries[i] = 0;
		}
		void updateGBH(uint64_t pc, uint64_t next_pc, InstClass insn, uint8_t result){
			last_pc = pc;
			oldghist = ghist;
			prediction_result = result;
			if(insn == condBranchInstClass || insn == uncondDirectBranchInstClass
				|| insn == uncondIndirectBranchInstClass){
				ghist <<=1; //shift in 0
				if(next_pc != pc+4){ // we took the branch
					#ifdef DEBUG2
					fprintf(stdout,"took branch  gist = %" PRIx64 "\n",ghist);
					#endif
					ghist |= 1; //shift in 1;
				}
			}
		}
		int16_t gtag(uint64_t pc, uint64_t h){
			int16_t hashed_tag = 0;
			hashed_tag = pc^h^(h<<1);//^(ghist+0xAB01);
			return hashed_tag;
		}
		int8_t get_prediction(uint64_t pc, uint64_t* prediction){
			//use hash of pc gbh to find longest matching tag
			// loop through tables and entries to find matching tags
			// return confidence of that entry
			// set hitbank to use for update and index
			// only check tag&((1<<TB[i])-1) so we only compare same number of bits of tag
			int8_t tagged_conf = -1;
			int8_t untagged_conf = 0;
			//int8_t confidence = 0;
			int16_t tag = gtag(pc,ghist);
			uint64_t index = pc&((1<<logg[0])-1);
			#ifdef DEBUG4
			fprintf(stdout,"pc is %" PRIx64 "\t" "tag is %" PRIx16 "\n",pc,tag);
			#endif
			#ifdef DEBUG1
			fprintf(stdout,"number of table entries is \n");
			for(int i=0;i<NHIST;i++)
				fprintf(stdout,"%" PRIx64 "\t", numberofentries[i]);
			fprintf(stdout,"\n");
			getchar();
			#endif
			untagged_conf = gtable[0][index].ctr;
			#ifdef DEBUG6
			fprintf(stdout,"looking for tag %" PRIx16 "\n",tag);
			#endif
			for(int i=1;i<NHIST+1;i++){//loop through the tables
				#ifdef DEBUG6
				fprintf(stdout,"for table %d\n",i);
				#endif
				//for(int j=0;j<(1<<logg[i]);j++){ // loop through the entries
				for(int j=0;j<numberofentries[i-1];j++){
					#ifdef DEBUG6
					fprintf(stdout,"%" PRIx16 "\t%" PRIx64 "\n",gtable[i][j].tag,gtable[i][j].value);
					#endif
					//if(gtable[i][j].tag&((1<<TB[i])-1) == tag&((1<<TB[i])-1)){
					if(gtable[i][j].tag == tag && gtable[i][j].ctr >= tagged_conf){
						HitBank = i;
						HitIndex = j;
						//temp1 = &gtable[i][j].value;
						tagged_conf = gtable[i][j].ctr;
					}
				}
			}
			#ifdef DEBUG8
			fprintf(stdout,"tagconf = %d\tuntagconf = %d\n", tagged_conf,untagged_conf);
			#endif
			if(tagged_conf>=untagged_conf){
				#ifdef DEBUG1
				fprintf(stdout,"took tagged\n");
				#endif
				pred_taken = gtable[HitBank][HitIndex].value;
				prediction = &pred_taken;
				return tagged_conf;
			} else {
				#ifdef DEBUG1
				fprintf(stdout,"took untagged\n");
				#endif
				pred_taken = gtable[0][index].value;
				prediction = &pred_taken;
				HitBank = 0;
				HitIndex = index;
				return untagged_conf;
			}
		}
		void update_predictor(uint64_t actual_value){
			// add actual value to lowest level table
			// if incorrect prediction add entry in next table
			// if correct prediction increment confidence counter
			uint64_t index = last_pc&((1<<logg[0])-1);
			gtable[0][index].value = actual_value;
			gtable[0][index].ctr = 1;
			gtable[0][index].tag = gtag(last_pc,oldghist);
			#ifdef DEBUG3
			fprintf(stdout, "pred = %" PRIx64 " bank %d\t real = %" PRIx64
							"\t result = %d\n",pred_taken,HitBank,actual_value,prediction_result);
			#endif
			//get hashes of tags based on pc
			#ifdef DEBUG7
				fprintf(stdout,"HitBank is %d\n",HitBank);
			#endif
			if(prediction_result==1){ // correct prediction
				if(gtable[HitBank][HitIndex].ctr < 3)
					gtable[HitBank][HitIndex].ctr++;
			} else if(prediction_result==0){ // incorrect prediction
				//if the table is not full add to bottom
				if(gtable[HitBank][HitIndex].ctr > 0) gtable[HitBank][HitIndex].ctr--;
				if(HitBank<NHIST){
					if(numberofentries[HitBank]<(1<<logg[HitBank+1])){//numberofentries[HitBank] matches with gtable[HitBank+1] because I need 1 fewer of them
						gtable[HitBank+1][numberofentries[HitBank]+1].tag = gtable[HitBank][HitIndex].tag;
						gtable[HitBank+1][numberofentries[HitBank]+1].value = actual_value;
						gtable[HitBank+1][numberofentries[HitBank]+1].ctr = 0; // initalize to 1 so it doesnt immediatley get replaced
						numberofentries[HitBank]++;
					} else {
						// replace first element with 0 ctr value
						int index = 0;
						for(int i=1;i<(1<<logg[HitBank]);i++)
							if(gtable[HitBank+1][i].ctr<gtable[HitBank+1][index])
								index = i;
						gtable[HitBank+1][index].value = actual_value;
						gtable[HitBank+1][index].ctr = 0;
					}
				}
			} // else we didnt make a prediction
		}
};
#endif













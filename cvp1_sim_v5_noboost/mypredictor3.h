#define NHIST 15 /*number of tagged tables*/
int m[NHIST+1]={0, 0, 10, 16, 27, 44, 60, 96, 109, 219, 449, 487, 714, 1313, 2146, 3881};
#define MINHIST 8		// shortest history length
#define MAXHIST 2000
#define HISTBUFFERLENGTH 4096 //size of the history circular  buffer
#define LOGG 12
#define TBITS 7


class folded_history{
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
class gentry{
	public:
		int16_t tag;
		uint64_t predicted_value;
		int8_t ctr;
		int8_t u;
	gentry(){
		ctr = 0;
		tag = 0;
		u = 0;
		predicted_value = 0;
	}
};


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

int TB[NHIST + 1];		// tag width for the different tagged tables
int logg[NHIST + 1];		// log of number entries of the different tagged tables
int GI[NHIST + 1] ;		// indexes to the different tables are computed only once
int GTAG[NHIST + 1];		// tags for the different tables are computed only once

uint64_t predicted_value;
uint64_t tage_pred;		// TAGE prediction
int8_t confidence;
int HitBank;			// longest matching bank
int Seed;
int TICK;
#define LOGTICK 6;



class my_predictor{
	public:
		my_predictor(void){
			Seed = 0;
			for (int i = 0; i < HISTBUFFERLENGTH; i++)
      			ghist[0] = 0; //initializing entire buffer length to 0
      		// initializing ptghist values
      		Fetch_ptghist = 0;
    		Retire_ptghist = 0;
    		TB[0] = 0;			// table T0 is tagless
			TB[1] = 6;
      		for (int i = 2; i <= NHIST; i++){ // set tag lengths table 0 not tagged, last table has max length tag 16
				TB[i] = (TBITS + i);
				if(TB[i] >= 16) TB[i] = 16;
			}
			logg[0] = LOGG;
			logg[1]= LOGG-1;
			for (int i = 2; i <= 4; i++) logg[i] = LOGG -2;
			for (int i = 5; i <= 13; i++) logg[i] = LOGG - 3;
			for (int i = 14; i <= NHIST; i++) logg[i] = LOGG - 4;
			//initialisation of index and tag computation functions
    		for (int i = 1; i <= NHIST; i++){
				Fetch_ch_i[i].init (m[i], (logg[i]));
				Fetch_ch_t[0][i].init (Fetch_ch_i[i].OLENGTH, TB[i]);
				Fetch_ch_t[1][i].init (Fetch_ch_i[i].OLENGTH, TB[i] - 1);
      		}
    		for (int i = 1; i <= NHIST; i++){
				Retire_ch_i[i].init(m[i], (logg[i]));
				Retire_ch_t[0][i].init(Retire_ch_i[i].OLENGTH, TB[i]);
				Retire_ch_t[1][i].init(Retire_ch_i[i].OLENGTH, TB[i] - 1);
      		}
      		// each table is an array of entries that is 2^logg[i] long.
      		// logg is an array of log(number of entries) for each table
      		for (int i = 0; i <= NHIST; i++)
				gtable[i] = new gentry[1 << (logg[i])];
     	}
      	// gindex computes a full hash of pc, ghist
  		int gindex (uint64_t pc, int bank, folded_history * ch_i){
    		int index;
        	index = pc^(pc>>(abs(logg[bank]-bank)+1))^ch_i[bank].comp;
    		return(index&((1<<(logg[bank]))-1));
  		}
		//  tag computation
	  	uint16_t gtag(unsigned int pc,int bank,folded_history *ch0,folded_history *ch1){
			int tag = pc^ch0[bank].comp^(ch1[bank].comp<<1);
			return (tag&((1<<TB[bank])-1));
	  	}
	  	int MYRANDOM(){
	  	    Seed++;
			Seed ^= Fetch_ptghist;
			Seed = (Seed>>11)+(Seed<<21);
			Seed += Retire_ptghist;
			return Seed;
		}
		void Tagepred(){
			HitBank = -1;
			//look for the bank with longest matching history
			for(int i = NHIST; i >= 0; i--){
				if(gtable[i][GI[i]].tag == GTAG[i]){
	    			HitBank = i;
	    			break;
	  			}
      		}
			tage_pred = gtable[HitBank][GI[HitBank]].predicted_value;
	  		confidence = gtable[HitBank][GI[HitBank]].predicted_value;

	  	}
	  	int8_t predict_value(uint64_t pc, uint64_t* pred_taken){
			// computes the table addresses and the partial tags
			for (int i = 0; i <= NHIST; i++){
	    		GI[i] = gindex (pc, i,  Fetch_ch_i);
	    		GTAG[i] = gtag (pc, i, Fetch_ch_t[0], Fetch_ch_t[1]);
	  		}
	    	GTAG[0] = 0;
	    	GI[0] = pc & ((1 << logg[0]) - 1);
	    	Tagepred ();
			//if the entry providing the target already provides
			return tage_pred;
      	}
      	void FetchHistoryUpdate(uint64_t pc,bool taken,uint64_t prediction){
      		HistoryUpdate(pc,taken,predicted_value,Fetch_ptghist,
		    	Fetch_ch_i, Fetch_ch_t[0], Fetch_ch_t[1]);
		}
		void HistoryUpdate(uint64_t pc, bool taken, uint64_t prediction, int &Y,
			folded_history *H, folded_history *G, folded_history *J){
      		//int maxt = (brtype & IS_BR_INDIRECT) ? 10 : 0;
      		//if (!(brtype & IS_BR_INDIRECT))
      		//	if (brtype & IS_BR_CALL)
			//		maxt = 5;
			int maxt = 10;
      int PATH = ((prediction ^ (prediction >> 3) ^ pc));
      // if (brtype & IS_BR_CONDITIONAL)
      // 	PATH = taken;
      for(int t = 0; t < maxt; t++){
	  		bool P = (PATH  & 1);
	  		PATH >>= 1;
				//update  history
	  		Y--;
	  		ghist[Y & (HISTBUFFERLENGTH - 1)] = P;
				//prepare next index and tag computations for user branchs
	  		for (int i = 1; i <= NHIST; i++){
	      	H[i].update (ghist, Y);
	      	G[i].update (ghist, Y);
	      	J[i].update (ghist, Y);
	   		}
			}

    	}//END UPDATE FETCH HISTORIES
    	void update_brindirect (uint64_t pc, bool taken,
			    uint64_t prediction){
      		int NRAND = MYRANDOM ();
      // 		if (brtype & IS_BR_INDIRECT){
					if(true){
	  	// 		PtIumRetire--;
			//Recompute  the prediction by the ITTAGE predictor: on an effective implementation one would try to avoid this recomputation by propagating the prediction with the branch instruction
	  			for (int i = 0; i <= NHIST; i++){
	      			GI[i] = gindex (pc, i, Retire_ch_i);
	      			GTAG[i] = gtag (pc, i, Retire_ch_t[0], Retire_ch_t[1]);
	    		}
	  			GTAG[0] = 0;
	  			GI[0] = pc & ((1 << logg[0]) - 1);
	  			Tagepred();
	  			// bool ALLOC = (LongestMatchPred != target);
					bool ALLOC = true;
				//allocation if the Longest Matching entry does not provide the correct entry
	  			// if((HitBank > 0) & (AltBank >= 0)){
				// Manage the selection between longest matching and alternate matching
				// for "pseudo"-newly allocated longest matching entry
	    //   			bool PseudoNewAlloc = (gtable[HitBank][GI[HitBank]].ctr == 0);
	    //   			if(PseudoNewAlloc){
		  // 				if(alttaken)
		  //   				if(LongestMatchPred != alttaken){
			// 					if((alttaken==target)||(LongestMatchPred==target)){
			//     					if(alttaken == target){
			// 							if(USE_ALT_ON_NA < 7) USE_ALT_ON_NA++;
			//       					} else {
			// 							if(USE_ALT_ON_NA >= -8) USE_ALT_ON_NA--;
			//       					}
			//   					}
			//
		  //     				}
			// 		}
	    // 		}
	  			if(ALLOC){
			// 		//Need to compute the target field (Offset + Region pointer)
	      			int Region = (prediction >> 18);
	      			int PtRegion = -1;
					//Associative search on the region table
	    //   			for(int i = 0; i < 128; i++)
			// 			if(rtable[i].region == Region){
		  //   				PtRegion = i;
		  //   				break;
		  // 				}
	    //   			if(PtRegion == -1){
			// 			//miss in the target region table, allocate a free entry
		  // 				for(int i = 0; i < 128; i++)
		  //   				if(rtable[i].u == 0){
			// 					PtRegion = i;
			// 					rtable[i].region = Region;
			// 					rtable[i].u = 1;
			// 					break;
			// 				}
			// 			// a very simple  replacement policy (don't care for the competition, but some replacement is needed in a real processor)
		  // 				if(PtRegion == -1){
		  //     				for(int i = 0; i < 128; i++)
			// 					rtable[i].u = 0;
		  //     				PtRegion = 0;
		  //     				rtable[0].region = Region;
		  //     				rtable[0].u = 1;
		  //   			}
					}
	      	int tempPrediction = (prediction & ((1 << 18) - 1)) + (PtRegion << 18);
					// we allocate an entry with a longer history
					//to  avoid ping-pong, we do not choose systematically the next entry, but among the 2 next entries
					int Y = NRAND;
					int X = HitBank + 1;
					if((Y & 31) == 0) X++;
					int T = 2;
					//allocating 3  entries on a misprediction just work a little bit better than a single allocation
	      			for(int i = X; i <= NHIST; i += 1){
		  					if(gtable[i][GI[i]].u == 0){
										gtable[i][GI[i]].tag = GTAG[i];
										gtable[i][GI[i]].predicted_value = tempPrediction;
										gtable[i][GI[i]].ctr = 0;
										gtable[i][GI[i]].u = 0;
										if(TICK > 0) TICK--;
										if(T == 0) break;
										T--;
										i += 1;
		    				} else TICK++;
							}
	    // 		}
	  	// 		if((TICK >= (1 << LOGTICK))){
	    //   			TICK = 0;
			// 		// reset the useful  bit
	    //   			for(int i = 0; i <= NHIST; i++)
			// 			for(int j = 0; j < (1 << logg[i]); j++)
		  // 					gtable[i][j].u = 0;
			//
	    // 		}
	  	// 		if(LongestMatchPred == target){
	    //   			if(gtable[HitBank][GI[HitBank]].ctr < 3)
			// 			gtable[HitBank][GI[HitBank]].ctr++;
	    // 		} else {
	    //   			if (gtable[HitBank][GI[HitBank]].ctr > 0)
			// 			gtable[HitBank][GI[HitBank]].ctr--;
	    //   			else {
	    //   				// replace the target field by the new target
			// 			// Need to compute the target field :-)
		  // 				int Region = (target >> 18);
		  // 				int PtRegion = -1;
		  // 				for(int i = 0; i < 128; i++)
		  //   				if(rtable[i].region == Region){
			// 					PtRegion = i;
			// 					break;
		  //   				}
		  // 				if(PtRegion == -1){
		  //     				for(int i = 0; i < 128; i++)
			// 					if(rtable[i].u == 0){
			//    	 					PtRegion = i;
			//     					rtable[i].region = Region;
			//     					rtable[i].u = 1;
			//     					break;
			//   					}
			// 				//a very simple  replacement policy (don't care for the competition, but some replacement is needed in a real processor)
	    //   					if(PtRegion == -1){
		  // 						for(int i = 0; i < 128; i++)
		  //   						rtable[i].u = 0;
		  // 						PtRegion = 0;
		  // 						rtable[0].region = Region;
		  // 						rtable[0].u = 1;
			// 				}
		  //   			}
		  // 				int IndTarget = (target&((1<<18)-1))+(PtRegion<<18);
		  // 				gtable[HitBank][GI[HitBank]].target = IndTarget;
			// 		}
	    // 		}
			// 	// update the u bit
	  	// 		if(HitBank != 0)
	    // 			if(LongestMatchPred != alttaken){
			// 			if(LongestMatchPred == target){
		  //   				gtable[HitBank][GI[HitBank]].u = 1;
		  // 				}
	      			}
					//END PREDICTOR UPDATE
			}
		// HistoryUpdate(pc, brtype, taken, target, Retire_ptghist, Retire_ch_i, Retire_ch_t[0], Retire_ch_t[1]);
};

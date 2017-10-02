/**************************************************************/
/* CS/COE 1541				 			
   just compile with gcc -o pipeline pipeline.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  

***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h> 
#include "CPU.h" 

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;
  struct trace_item IF;
  struct trace_item temp1;
  struct trace_item temp2;
  struct trace_item ID;
  struct trace_item EX;
  struct trace_item MEM;
  struct trace_item WB; 	
  	  	  		
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  int prediction_method = 0;
  int stop = -1;
  int flag = 0;
  int branchFlag = 0;
  int branchStop = 0;
  
  unsigned char t_type = 0;
  unsigned char t_sReg_a= 0;
  unsigned char t_sReg_b= 0;
  unsigned char t_dReg= 0;
  unsigned int t_PC = 0;
  unsigned int t_Addr = 0;
  unsigned int cycle_number = 0;
  
	if (argc == 3){
	trace_file_name = argv[1];
	trace_view_on = atoi(argv[2]);
	}
	
	else if (argc == 2){
	trace_file_name = argv[1];
	}
	
	else if (argc == 4){ 
	trace_file_name = argv[1];
	trace_view_on = atoi(argv[2]); 
	prediction_method = atoi(argv[3]);

	}

	else{
	fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character> <prediction - any character\n");
	fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
	fprintf(stdout, "\n which prediction method \n");
	exit(0);
	}
	fprintf(stdout, "\n ** opening file %s\n", trace_file_name);
	trace_fd = fopen(trace_file_name, "rb");
  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  while(1) {
  
	if((EX.type == 3) && (EX.dReg == IF.sReg_a || EX.dReg == IF.sReg_b)){
		*tr_entry = IF;
		IF = ID;
		ID.type = 0;
	}
	

	else if(EX.type == 5){
		if (prediction_method == 0){
			if(ID.PC - EX.PC != 4){
				branchFlag = 1;
				branchStop = cycle_number + 2;
			}
			size = trace_get_item(&tr_entry);
		}
		if(prediction_method == 1){
			size = trace_get_item(&tr_entry);
		}
	}

  	//EDIT: we need to make sure it stops if hazard is last instruction

  	else{
		 size = trace_get_item(&tr_entry);

		  if(cycle_number == stop){
			  printf("+ Simulation terminates at cycle : %u\n", cycle_number);
			  break;
		  }
	  }
	  
		  //branch control
		  if((cycle_number == branchStop) && branchFlag == 1){
		  	cycle_number++;
		  	printf("[cycle %d]",cycle_number);
		  	printf("SQUASHED!\n");
		  	cycle_number++;		
		  	printf("[cycle %d]",cycle_number);  	
		  	printf("SQUASHED!\n");
		  }
		  	  
		  WB = MEM;
		  MEM = EX;
		  EX = ID;
		  ID = IF;


		  if(size)IF = *tr_entry;
		  if(!size && flag == 0){ 
			flag = 1;
			stop = cycle_number + 4;
		  }
		  cycle_number++;    
      
/* 
      
      t_type = tr_entry->type;
      t_sReg_a = tr_entry->sReg_a;
      t_sReg_b = tr_entry->sReg_b;
      t_dReg = tr_entry->dReg;
      t_PC = tr_entry->PC;
      t_Addr = tr_entry->Addr;
 */  

// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE

    if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
    	//simulate 5 stage pipeline. Print out cycle, then for loop switch 	
    	  printf("[cycle %d]",cycle_number);
				switch(WB.type) {
					case ti_NOP:
					  printf("NOP:\n");
					  break;
					case ti_RTYPE:
					  printf("RTYPE:");
					  printf(" (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", WB.PC, WB.sReg_a, WB.sReg_b, WB.dReg);
					  break;
					case ti_ITYPE:
					  printf("ITYPE:");
					  printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", WB.PC, WB.sReg_a, WB.dReg, WB.Addr);
					  break;
					case ti_LOAD:
					  printf("LOAD:");		 
					  printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", WB.PC, WB.sReg_a, WB.dReg, WB.Addr);
					  break;
					case ti_STORE:
					  printf("STORE:");	  
					  printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", WB.PC, WB.sReg_a, WB.sReg_b, WB.Addr);
					  break;
					case ti_BRANCH:
					  printf("BRANCH:");
					  printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", WB.PC, WB.sReg_a, WB.sReg_b, WB.Addr);
					  break;
					case ti_JTYPE:
					  printf("JTYPE:");
					  printf(" (PC: %x)(addr: %x)\n", WB.PC,WB.Addr);
					  break;
					case ti_SPECIAL:
					  printf("SPECIAL:\n");			
					  break;
					case ti_JRTYPE:
					  printf("JRTYPE:");
					  printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", WB.PC, WB.dReg, WB.Addr);
					  break;
			  }
    	}
	}
  
  trace_uninit();

  exit(0);
}




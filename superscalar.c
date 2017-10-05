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

void trace_view(struct trace_item stage, int cycle_number)
{
	printf("[cycle %d] ", cycle_number);
	switch(stage.type)
	{
		case ti_NOP:
			printf("NOP:\n");
			break;
		case ti_RTYPE:
			printf("RTYPE:");
			printf(" (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", stage.PC, stage.sReg_a, stage.sReg_b, stage.dReg);
			break;
		case ti_ITYPE:
			printf("ITYPE:");
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.dReg, stage.Addr);
			break;
		case ti_LOAD:
			printf("LOAD:");		 
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.dReg, stage.Addr);
			break;
		case ti_STORE:
			printf("STORE:");	  
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.sReg_b, stage.Addr);
			break;
		case ti_BRANCH:
			printf("BRANCH:");
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.sReg_b, stage.Addr);
			break;
		case ti_JTYPE:
			printf("JTYPE:");
			printf(" (PC: %x)(addr: %x)\n", stage.PC, stage.Addr);
			break;
		case ti_SPECIAL:
			printf("SPECIAL:\n");			
			break;
		case ti_JRTYPE:
			printf("JRTYPE:");
			printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", stage.PC, stage.dReg, stage.Addr);
			break;
		default :
			printf("NOP:\n");
			break;
	}
}

int main(int argc, char **argv)
{
	struct trace_item A;
	struct trace_item B;
	struct trace_item prevA;
	struct trace_item prevB;
	struct trace_item instruction_buffer[2];
	struct trace_item *tr_entry;
	struct trace_item IF_1;
	struct trace_item temp1;
	struct trace_item temp2;
	struct trace_item ID_1;
	struct trace_item EX_1;
	struct trace_item MEM_1;
	struct trace_item WB_1;
	struct trace_item IF_2;
	struct trace_item ID_2;
	struct trace_item EX_2;
	struct trace_item MEM_2;
	struct trace_item WB_2;
	
	size_t size;
	char *trace_file_name;
	int trace_view_on = 0;
	int prediction_method = 0;
	int stop = -1;
	int flag = 0;
	int branch_flag = 0;
	int branch_stop = 0;
	int branch_mask = 1008;//(1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9);
	unsigned int last_result_index;
	unsigned int branch_index;
	unsigned int prev_branch_index;
	int prediction;
	int prediction_pos = -1;
	int prediction_lookup;
	int prediction_table[3] = {-1, -1, -1};
	int squash_pos = 0;
	int squash_table[3] = {-1, -1, -1};
	int branch_table[64][3];
	unsigned int pos_row;
	unsigned int pos_col;
	int fetch = 2;

	unsigned char t_type = 0;
	unsigned char t_sReg_a= 0;
	unsigned char t_sReg_b= 0;
	unsigned char t_dReg= 0;
	unsigned int t_PC = 0;
	unsigned int t_Addr = 0;
	unsigned int cycle_number = 0;

	// Handling Inputs
	if (argc == 3)
	{
		trace_file_name = argv[1];
		trace_view_on = atoi(argv[2]);
	}
	else if (argc == 2)
	{
		trace_file_name = argv[1];
		trace_view_on = 0;
		prediction_method = 0;
	}
	else if (argc == 4)
	{ 
		trace_file_name = argv[1];
		trace_view_on = atoi(argv[3]); 
		prediction_method = atoi(argv[2]);
	}
	else
	{
		fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character> <prediction - any character\n");
		fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
		fprintf(stdout, "\n which prediction method \n");
		exit(0);
	}


	// Checking File
	fprintf(stdout, "\n ** opening file %s\n", trace_file_name);
	trace_fd = fopen(trace_file_name, "rb");
	if (!trace_fd)
	{
		fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
		exit(0);
	}

	trace_init();
	
	// Initialize Branch Table
	for (pos_row = 0; pos_row < 63; pos_row++)
	{
		for (pos_col = 0; pos_col < 2; pos_col++)
		{
			branch_table[pos_row][pos_col] = -1;
		}
	}

	// Start Processes
	while(1)
	{
		//control variable on which instructions to fetch 
		//fetch = 0 dont fetch instructions
		//fetch = 1 fetch A
		//fetch = 2 fetch A and B
		//note: Will never have to fetch only B	
		
		switch(fetch){
			prevA = instruction_buffer[0]; 
			prevB = instruction_buffer[1];
		case 0:
			
			break;
	
		case 1:
			//Retain previous instruction
			prevA = instruction_buffer[0]; 
			prevB.type = 0;
			//Get the instructions into the instruction buffer
			size = trace_get_item(&tr_entry);
			instruction_buffer[0] = *tr_entry;
			break;
	
		case 2:
			//Retain previous instructions
			prevA = instruction_buffer[0]; 
			prevB = instruction_buffer[1]; 
			//Get the instructions into the instruction buffer
			size = trace_get_item(&tr_entry);
			instruction_buffer[0] = *tr_entry;
			size = trace_get_item(&tr_entry);
			instruction_buffer[1] = *tr_entry;
			break;
	
		}
			
		//check the two instructions in the instruction_buffer to issue to REG	
		A = instruction_buffer[0];
		B = instruction_buffer[1];
		
		
	if(((A.type == 3 || A.type == 4) && (B.type == 1 || B.type == 2 || B.type == 5|| B.type == 6 || B.type == 8))||((B.type == 3 || B.type == 4) && (A.type == 1 || A.type == 2 ||A.type == 5|| A.type == 6 ||A.type == 8))){
	
		if(A.type != 5 && A.type != 6 && A.type != 8){ //first inst is not branch/jump

			if((A.sReg_a != B.dReg && A.sReg_b != B.dReg) && (B.sReg_a != A.dReg && B.sReg_b != A.dReg)){//no data dependence 

				if (prevB.type == 3 && prevA.type == 3){ 
					if((prevA.dReg != B.sReg_a && prevA.dReg != B.sReg_b) && (prevA.dReg != A.sReg_a && prevA.dReg != A.sReg_b) && (prevB.dReg != B.sReg_a && prevB.dReg != B.sReg_b) && (prevB.dReg != A.sReg_a && prevB.dReg != A.sReg_b)){//check load dependence for previous instructions{
						fetch = 2;
					}
				}
				else if(prevA.type == 3 && prevB.type != 3){
					if(prevA.dReg != A.sReg_a && prevA.dReg != A.sReg_b && prevA.dReg != B.sReg_a && prevA.dReg != B.sReg_b){//good
							fetch = 2;
						}
					}
				else if (prevB.type == 3 && prevA.type != 3){ //if the first of the two instructions does not have a load/use dependence
					if(prevB.dReg != A.sReg_a && prevB.dReg != A.sReg_b && prevB.dReg != B.sReg_a && prevB.dReg != B.sReg_b){//good
							fetch = 2;
						}
					}
				else{
						fetch = 2;
					}
				}
			}
		}


		else if(prevA.type == 3 && prevB.type != 3){ //if the first of the two instructions does not have a load/use dependence
			if(prevA.dReg != A.sReg_a && prevA.dReg != A.sReg_b){//good
				B.type = 0;
				fetch = 1;
			}
		}
		else if (prevB.type == 3 && prevA.type != 3){ //if the first of the two instructions does not have a load/use dependence
			if(prevB.dReg != A.sReg_a && prevB.dReg != A.sReg_b){//good
				B.type = 0;
				fetch = 1;
			}
		}
		else if (prevB.type == 3 && prevA.type == 3){ //if the first of the two instructions does not have a load/use dependence
			if((prevB.dReg != A.sReg_a && prevB.dReg != A.sReg_b) && (prevA.dReg != A.sReg_a && prevA.dReg != A.sReg_b)){
				B.type = 0;
				fetch = 1;
			}
		}
		else if(prevB.type != 3 && prevA.type != 3){
				B.type = 0;
				fetch = 1;
		}
		
		else{
			A.type = 0;
			B.type = 0;
			fetch = 0;
		}

		//Now we need to decide which instruction to push to which pipeline
		//A and B are pushed
		//A
		//none pushed
		
		//utilize fetch
		//check A for ALU
		
		switch(fetch){
		case 0://both instructions no ops.
			IF_1 = A;
			IF_2 = B;
			break;
		case 1:
			if(A.type == 3 || A.type == 4){//A is lw/sw instruction
				IF_1 = A;
				IF_2 = B;
			}
			else{//A is not lw/sw instruction. B is noOP
				IF_1 = B;
				IF_2 = A;
			}
			break;				
		case 2:
			if(A.type == 3 || A.type == 4){//A is lw/sw instruction
				IF_1 = A;
				IF_2 = B;
			}
			else{
				IF_1 = B;
				IF_2 = A;
			}
			break;		
		}

		// IF Processing
		if((prediction_method == 1) && (IF_2.type == 5))
		{
			last_result_index = (IF_2.PC & branch_mask) >> 4;
			prediction = branch_table[last_result_index][0];
			prediction_table[prediction_pos] = prediction;
		}
		
		// When IF Instruction is in EX stage, prediction_pos will be the same
		prediction_pos++;
		if (prediction_pos > 2)
		{
			prediction_pos = 0;
		}
		
		// EX Processing
		// Data Hazard
// 		if((EX.type == 3) && (EX.dReg == IF.sReg_a || EX.dReg == IF.sReg_b))
// 		{
// 			*tr_entry = IF;
// 			IF = ID;
// 			ID.type = 0;
// 		}
		// Branch Prediction
		else if(EX_2.type == 5)
		{
			// Always Predict Not Taken
			if (prediction_method == 0)
			{
				// Branch Was Taken
				if(ID_2.PC - EX_2.PC != 4)
				{
					squash_table[squash_pos] = 1;
				}
				else
				{
					// Branch Was Not Taken
				}
		
				size = trace_get_item(&tr_entry);
			}
			// Log Actual Result
			if(prediction_method == 1)
			{				
				// Mask Bits To Get 9-4 Of Address
				branch_index = (EX_2.PC & branch_mask) >> 4;
				branch_table[branch_index][1] = EX_2.PC;
				
				prediction = prediction_table[prediction_pos];
				
				// Compare Prediction To Current
				// Prediction = Prev Actual = Current Item in Branch Table
				// Do Not Have To Flag At ID
				if(ID_2.PC - EX_2.PC != 4)
				{
					// Prediction != 1
					if (prediction != 1)
					{
						squash_table[squash_pos] = 1;
					}
					branch_table[branch_index][0] = 1;
					branch_table[branch_index][2] = EX_2.Addr;
				}
				else
				{
					branch_table[branch_index][0] = 0;
					branch_table[branch_index][2] = EX_2.PC + 4;
				}
				
				size = trace_get_item(&tr_entry);
			}
			
		}
		// Stop When Hazard Last Instruction
		else
		{
			size = trace_get_item(&tr_entry);
			if(cycle_number == stop)
			{
				printf("+ Simulation terminates at cycle : %u\n", cycle_number);
				break;
			}
			
		}
		
		// When EX Instruction is in WB stage, squash_pos will be the same
		squash_pos++;
		if (squash_pos > 2)
		{
			squash_pos = 0;
		}

		// Branch Control
		if(squash_table[squash_pos] == 1)
		{
			// Insert Squashed "Cycles"
			cycle_number++;
			if (trace_view_on)
			{
				printf("[cycle %d]SQUASHED!\n", cycle_number);
				printf("[cycle %d]SQUASHED!\n", cycle_number);
			}
			cycle_number++;
			if (trace_view_on)
			{
				printf("[cycle %d]SQUASHED!\n", cycle_number);
				printf("[cycle %d]SQUASHED!\n", cycle_number);
			}
			squash_table[squash_pos] = 0;
		}
		
		// Cascade States
		WB_1 = MEM_1;
		MEM_1 = EX_1;
		EX_1 = ID_1;
		ID_1 = IF_1;
		
		WB_2 = MEM_2;
		MEM_2= EX_2;
		EX_2 = ID_2;
		ID_2 = IF_2;
		
		if(size)
		{
			IF_2 = *tr_entry;
		}
			IF_1 = *tr_entry;
		if(!size && flag == 0)
		{ 
			flag = 1;
			stop = cycle_number + 4;
		}
		
		cycle_number++;

		// Print Executed Instructions (trace_view_on=1)
		if (trace_view_on)
		{
			trace_view(WB_1, cycle_number);
			trace_view(WB_2, cycle_number);
		}
}
	trace_uninit();
	exit(0);

}

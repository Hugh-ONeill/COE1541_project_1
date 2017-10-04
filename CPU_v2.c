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
	int branch_flag = 0;
	int branch_stop = 0;
	int branch_mask = (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9);
	unsigned int last_result_addr;
	unsigned int branch_addr;
	unsigned int prev_branch_addr;
	int prediction;
	int branch_table[64][3];
	unsigned int pos_row;
	unsigned int pos_col;

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
	}
	else if (argc == 4)
	{ 
		trace_file_name = argv[1];
		trace_view_on = atoi(argv[2]); 
		prediction_method = atoi(argv[3]);
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
		// IF Processing
		printf("[IF: type: %d]\n", IF.type);
		if((prediction_method == 1) && (IF.type == 5))
		{
			last_result_addr = (IF.PC & branch_mask) >> 4;
			printf("[IF: addr: %x]\n", last_result_addr);
			prediction = branch_table[last_result_addr][0];
			//printf("[prediction IF %d]\n", prediction);
		}
		
		// EX Processing
		if((EX.type == 3) && (EX.dReg == IF.sReg_a || EX.dReg == IF.sReg_b))
		{
			*tr_entry = IF;
			IF = ID;
			ID.type = 0;
		}
		// Branch Prediction
		else if(EX.type == 5)
		{
			// Always Predict Not Taken
			if (prediction_method == 0)
			{
				// Branch Was Taken
				if(ID.PC - EX.PC != 4)
				{
					branch_flag = 1;
					branch_stop = cycle_number + 2;
				}
				else
				{
					// Branch Was Not Taken
				}
		
				size = trace_get_item(&tr_entry);
			}
			// TODO
			// Predict Last Branch Condition
			if(prediction_method == 1)
			{				
				// Impliment 64 Entry Hash Table
				// Mask Bits To Get 9-4 Of Address
				branch_addr = (EX.Addr & branch_mask) >> 4;
				branch_table[branch_addr][1] = EX.PC;
				
				// Branch Taken? Untaken = 0? I'm not sure how to detect that
				// Somewhere, we have the last address come to mean what we predict next
				if(ID.PC - EX.PC != 4)
				{
					branch_table[branch_addr][0] = 1;
					branch_table[branch_addr][1] = EX.Addr;
				}
				else
				{					
					branch_table[branch_addr][0] = 0;
					branch_table[branch_addr][1] = EX.PC + 4;
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

		// Branch Control
		// When Branch in WB was in EX, command in ID was not (PC+4)
		// So a Branch was Taken
		if((cycle_number == branch_stop) && branch_flag == 1)
		{
			cycle_number++;
			printf("[cycle %d]",cycle_number);
			printf("SQUASHED!\n");
			cycle_number++;		
			printf("[cycle %d]",cycle_number);  	
			printf("SQUASHED!\n");
		}
		
		// Cascade States
		WB = MEM;
		MEM = EX;
		EX = ID;
		ID = IF;

		// What is this?
		if(size)
		{
			IF = *tr_entry;
		}
		
		if(!size && flag == 0)
		{ 
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

		// Print Executed Instructions (trace_view_on=1)
		if (trace_view_on)
		{
			printf("[cycle %d]", cycle_number);
			switch(WB.type)
			{
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
					printf(" (PC: %x)(addr: %x)\n", WB.PC, WB.Addr);
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




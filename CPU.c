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
	printf("[cycle %d]", cycle_number);
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
		// IF Processing
		if((prediction_method == 1) && (IF.type == 5))
		{
			last_result_index = (IF.PC & branch_mask) >> 4;
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
				branch_index = (EX.PC & branch_mask) >> 4;
				branch_table[branch_index][1] = EX.PC;
				
				prediction = prediction_table[prediction_pos];
				
				// Compare Prediction To Current
				// Prediction = Prev Actual = Current Item in Branch Table
				// Do Not Have To Flag At ID
				if(ID.PC - EX.PC != 4)
				{
					// Prediction != 1
					if (prediction != 1)
					{
						squash_table[squash_pos] = 1;
					}
					branch_table[branch_index][0] = 1;
					branch_table[branch_index][2] = EX.Addr;
				}
				else
				{
					branch_table[branch_index][0] = 0;
					branch_table[branch_index][2] = EX.PC + 4;
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
			}
			cycle_number++;
			if (trace_view_on)
			{
				printf("[cycle %d]SQUASHED!\n", cycle_number);
			}
			squash_table[squash_pos] = 0;
		}
		
		// Cascade States
		WB = MEM;
		MEM = EX;
		EX = ID;
		ID = IF;
		
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

		// Print Executed Instructions (trace_view_on=1)
		if (trace_view_on)
		{
			trace_view(WB, cycle_number);
		}
	}

	trace_uninit();
	exit(0);
}

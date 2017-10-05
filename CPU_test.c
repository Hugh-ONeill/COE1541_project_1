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

void trace_view(struct trace_item stage, int cycle_number, char* name)
{
	printf("[%s]\t", name);
	printf("[cycle %d]\t", cycle_number);
	switch(stage.type)
	{
		case ti_NOP:
			printf("NOP:\n");
			break;
		case ti_RTYPE:
			printf("RTYPE:\t");
			printf(" (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", stage.PC, stage.sReg_a, stage.sReg_b, stage.dReg);
			break;
		case ti_ITYPE:
			printf("ITYPE:\t");
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.dReg, stage.Addr);
			break;
		case ti_LOAD:
			printf("LOAD:\t");		 
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.dReg, stage.Addr);
			break;
		case ti_STORE:
			printf("STORE:\t");	  
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.sReg_b, stage.Addr);
			break;
		case ti_BRANCH:
			printf("BRANCH:\t");
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", stage.PC, stage.sReg_a, stage.sReg_b, stage.Addr);
			break;
		case ti_JTYPE:
			printf("JTYPE:\t");
			printf(" (PC: %x)(addr: %x)\n", stage.PC, stage.Addr);
			break;
		case ti_SPECIAL:
			printf("SPECIAL:\n");			
			break;
		case ti_JRTYPE:
			printf("JRTYPE:\t");
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
	struct trace_item ID;
	struct trace_item EX;
	struct trace_item MEM;
	struct trace_item WB;
	struct trace_item DATA_TEMP;
	struct trace_item BRANCH_TEMP_1;
	struct trace_item BRANCH_TEMP_2;
	
	
	size_t size;
	char *trace_file_name;
	int trace_view_on = 0;
	int prediction_method = 0;

	int stop_flag = 0;
	int stop_counter = 0;
	
	int data_flag = 0;
	
	int branch_flag = 0;
	
	int branch_mask = 1008;//(1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9);
	unsigned int last_result_index;
	unsigned int branch_index;
	unsigned int prev_branch_index;
	
	int prediction;
	int prediction_pos = 0;
	int prediction_lookup;
	int prediction_table[3] = {-1, -1, -1};
	
	int squash_pos = 0;
	int squash_table[3] = {-1, -1, -1};
	
	int branch_table[64][3];
	unsigned int branch_row;
	unsigned int branch_col;

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
	for (branch_row = 0; branch_row < 63; branch_row++)
	{
		for (branch_col = 0; branch_col < 2; branch_col++)
		{
			branch_table[branch_row][branch_col] = -1;
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
			if (trace_view_on)
			{
				printf("[IF:%d]------------------------------------------------------------------------------------------\n", cycle_number);
				printf("[Index]:\t%d\n[Position]:\t%d\n[Prediction]:\t%d\n", last_result_index, prediction_pos, prediction);
				//printf("[IF:%d]------------------------------------------------------------------------------------------\n", cycle_number);
			}
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
			DATA_TEMP = IF;
			IF = ID;
			ID.type = 0;
			data_flag = 1;
			if (trace_view_on)
			{
				printf("------------------------------------------------------------------------------------------------\n");
				printf("DATA HAZARD DETECTED!\tSTALL!\n");
			}
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
					if (trace_view_on)
					{
						printf("[EX:%d]------------------------------------------------------------------------------------------\n", cycle_number);
						printf("[Index]:\t%d\n[Position]:\t%d\n[Prediction]:\t%d\n[Actual]:\t1\n", branch_index, prediction_pos, prediction);
						//printf("[EX:%d]------------------------------------------------------------------------------------------\n", cycle_number);
					}
					// Prediction != 1
					if (prediction != 1)
					{
						BRANCH_TEMP_1 = IF;
						BRANCH_TEMP_2 = ID;
						
						IF.type = 0;
						ID.type = 0;
						
						branch_flag = 2;
						
						if (trace_view_on)
						{
							printf("------------------------------------------------------------------------------------------------\n");
							printf("MISPREDICTION DETECTED!\tSQUASH!\n");
							printf("[BT1] (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", BRANCH_TEMP_1.PC, BRANCH_TEMP_1.sReg_a, BRANCH_TEMP_1.sReg_b, BRANCH_TEMP_1.dReg);
							printf("[BT2] (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", BRANCH_TEMP_2.PC, BRANCH_TEMP_2.sReg_a, BRANCH_TEMP_2.sReg_b, BRANCH_TEMP_2.dReg);
						}
					}
					
					branch_table[branch_index][0] = 1;
					branch_table[branch_index][2] = EX.Addr;
						
				}
				else
				{
					if (trace_view_on)
					{
						printf("[Index]:\t%d\n[Prediction]:\t%d\n[Actual]:\t0\n", branch_index, prediction);
					}
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
			if(stop_counter >= 4)
			{
				printf("+ Simulation terminates at cycle : %u\n", cycle_number);
				break;
			}
			
		}
		
		// Cascade States
		WB = MEM;
		MEM = EX;
		EX = ID;
		ID = IF;
		
		if(size)
		{
			if (data_flag == 1)
			{
				IF = DATA_TEMP;
				data_flag = 0;
			}
			else if (branch_flag > 0)
			{
				if (branch_flag == 2)
				{
					IF = BRANCH_TEMP_2;
					printf("------------------------------------------------------------------------------------------------\n");
					printf("BRANCHING INSERT %d\n", branch_flag);
					printf("[BT1] (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", BRANCH_TEMP_1.PC, BRANCH_TEMP_1.sReg_a, BRANCH_TEMP_1.sReg_b, BRANCH_TEMP_1.dReg);
					printf("[BT2] (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", BRANCH_TEMP_2.PC, BRANCH_TEMP_2.sReg_a, BRANCH_TEMP_2.sReg_b, BRANCH_TEMP_2.dReg);
						
				}
				else if (branch_flag == 1)
				{
					IF = BRANCH_TEMP_1;
					printf("------------------------------------------------------------------------------------------------\n");
					printf("BRANCHING INSERT %d\n", branch_flag);
					printf("[BT1] (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", BRANCH_TEMP_1.PC, BRANCH_TEMP_1.sReg_a, BRANCH_TEMP_1.sReg_b, BRANCH_TEMP_1.dReg);
					printf("[BT2] (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", BRANCH_TEMP_2.PC, BRANCH_TEMP_2.sReg_a, BRANCH_TEMP_2.sReg_b, BRANCH_TEMP_2.dReg);
				}
				else
				{
					printf("------------------------------------------------------------------------------------------------\n");
					printf("BRANCHING ERROR %d\n", branch_flag);
				}
				
				branch_flag--;
			}		
			else
			{
				IF = *tr_entry;
				printf("------------------------------------------------------------------------------------------------\n");
				printf("TR ENTRY %d\n", IF.type);
				printf("[IF] (PC: %x)(sReg_a: %xd)(sReg_b: %d)(dReg: %d)\n", IF.PC, IF.sReg_a, IF.sReg_b, IF.dReg);
			}
		}
		
		if(!size && stop_flag == 0)
		{ 
			stop_flag = 1;
		}
		
		if (stop_flag == 1)
		{
			stop_counter++;
			IF.type = 0;
		}
		
		cycle_number++;

		// Print Executed Instructions (trace_view_on=1)
		if (trace_view_on)
		{
			printf("------------------------------------------------------------------------------------------------\n");
			trace_view(IF, cycle_number, "IF");
			trace_view(ID, cycle_number, "ID");
			trace_view(EX, cycle_number, "EX");
			trace_view(MEM, cycle_number, "MEM");
			trace_view(WB, cycle_number, "WB");
		}
	}
	
	trace_uninit();
	exit(0);
}


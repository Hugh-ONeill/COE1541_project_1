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
	
	unsigned char t_type = 0;
	unsigned char t_sReg_a= 0;
	unsigned char t_sReg_b= 0;
	unsigned char t_dReg= 0;
	unsigned int t_PC = 0;
	unsigned int t_Addr = 0;
	unsigned int cycle_number = 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	//show_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
				//runAll();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		case 'f':
		case 'F':
			if (scanf("%d", &ENABLE_FORWARDING) != 1) {
				
				break;
			}
			ENABLE_FORWARDING == 0 ? printf("Forwarding OFF\n") : printf("Forwarding ON\n");
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	printf("\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
	WB();
	MEM();
	EX();
	ID();
	IF();
	
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	uint32_t rs;
	uint32_t rt;
	uint32_t rd;
	printf("\n**************************************Writeback Stage**************************************\n");
	uint32_t opcode = (0xFC000000 & MEM_WB.IR ); 
	rt = (0x001F0000 & MEM_WB.IR) >> 16;
	rd = (0x0000F800 & MEM_WB.IR ) >> 11;
	
	switch (opcode) {
		
	/***********************************************************R-Type************************************************************************/
		case 0x00000000: 
		//WB_Value - data dependency bet ID and WB, data is transfered to EXE
		WB_Value=MEM_WB.ALUOutput;
		NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
		rdd=rd;
		if (aa==1){
		   printf("\n REGS[rd] = 0x%08x \n", NEXT_STATE.REGS[rd]);}
		break;
	
	/***********************************************************I-Type************************************************************************/
		case 0x24000000: //For ADDIU 
		WB_Value=MEM_WB.ALUOutput;
		NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
		rdd=rt;
		printf("\n REGS[rt] = 0x%08x \n", NEXT_STATE.REGS[rt]);
		break;
		
		case 0x3C000000: //For LUI
		WB_Value=MEM_WB.ALUOutput;
		NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
		rdd=rt;
		printf("\nREGS[rt] = 0x%08x \n", NEXT_STATE.REGS[rt]);
		break;
		
		case 0x38000000: //For XORI 
		WB_Value=MEM_WB.ALUOutput;
		NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
		rdd=rt;
		printf("\nREGS[rt] = 0x%08x \n", NEXT_STATE.REGS[rt]);
		break;
	
	/***********************************************************LW/SW-Type************************************************************************/
		case 0x8C000000:	//For LW.
		WB_Value=MEM_WB.LMD;
		NEXT_STATE.REGS[rt] = MEM_WB.LMD;
		rdd=rt;
		printf("\nLMD = 0x%08x \n", NEXT_STATE.REGS[rt]);
		break;

	}	
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	uint32_t rs;
	uint32_t rt;
	uint32_t rd;
	MEM_WB.IR = EX_MEM.IR;	

        //Check System call instruction
        if(MEM_WB.IR == 0x0000000c) {
		RUN_FLAG = FALSE;
	}

	printf("\n**************************************Memory Stage*****************************************\n");
	printf("MEM/WB.IR=  0x%08x", MEM_WB.IR );
	uint32_t opcode = (0xFC000000 & MEM_WB.IR ); 
	rd = (0x0000F800 & MEM_WB.IR ) >> 11;
	rt = (0x001F0000 & IF_EX.IR ) >> 16;
	//MEM_WB_rd -- check stall condition in decode stage
	MEM_WB_rd = rd;
	switch (opcode) {
		
		/***********************************************************R-Type************************************************************************/
		case 0x00000000: 
		MEM_WB.ALUOutput = EX_MEM.ALUOutput;
		FW_Mem=MEM_WB.ALUOutput;
		printf("\nMEM/WB.ALUOutput = 0x%08x \n", MEM_WB.ALUOutput);
		break;
		
		/***********************************************************I-Type************************************************************************/
		case 0x24000000: //For ADDIU 
		MEM_WB.ALUOutput = EX_MEM.ALUOutput;
		printf("\nMEM/WB.ALUOutput = 0x%08x \n", MEM_WB.ALUOutput);
		break;
		
		case 0x3C000000: //For LUI
		MEM_WB.ALUOutput = EX_MEM.ALUOutput;
		printf("\nMEM/WB.ALUOutput = 0x%08x \n", MEM_WB.ALUOutput);
		break;
		
		case 0x38000000: //For XORI 
		MEM_WB.ALUOutput = EX_MEM.ALUOutput;
		printf("\nMEM/WB.ALUOutput = 0x%08x \n", MEM_WB.ALUOutput);
		break;
		
		/***********************************************************LW/SW-Type************************************************************************/
		case 0xAC000000:  //For SW.
		mem_write_32(EX_MEM.ALUOutput, EX_MEM.B);
		printf("\nMEM/WB.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
		break;
		
		case 0x8C000000:	//For LW.
		MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
		FW_Mem=MEM_WB.LMD;	
		printf("\nMEM/WB.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
		printf("\nMEM/WB.LMD = 0x%08x \n", MEM_WB.LMD);
		break;
	}
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	uint32_t rs;
	uint32_t rt;
	uint32_t rd;
	if (sh==1){ //1 stall
		if(sh_A==1){
			IF_EX.A=WB_Value;
			sh_A=0;
			sh=0;}
		if(sh_B==1){
			IF_EX.B=WB_Value;
			sh_B=0;
			sh=0;}
		}
	if (p>=1){ // 2 stall
		if(p==3){
			stallflag=0;
			p=0;
			if(A==1){
				IF_EX.A=WB_Value;
				A=0;}
			if(B==1){
				IF_EX.B=WB_Value;
				B=0;}
		}
		else
			p++;
	}
	if(p1>=1){ // load forwarding
		if(p1==2){
		FW_LW=0;
		p1=0;
			if(A==1){
				IF_EX.A=FW_Mem;
				A=0;}
			if(B==1){
				IF_EX.B=FW_Mem;
				B=0;}
		}
		else
			p1++;		
	}

	if (p ==0 && p1 ==0){

	printf("\n**************************************Execute Stage****************************************\n");
	EX_MEM.IR = IF_EX.IR;
	printf("\nEX/MEM.IR=0x%08x\n", EX_MEM.IR );
	uint32_t opcode = (0xFC000000 & EX_MEM.IR ); 
	
	switch (opcode) {
		
		/***********************************************************R-Type************************************************************************/
		case 0x00000000:
		rs = (0x03E00000 & EX_MEM.IR ) >> 21;
	        rt = (0x001F0000 & EX_MEM.IR ) >> 16;
		rd = (0x0000F800 & EX_MEM.IR ) >> 11;
		EX_MEM_rd = rd;
		printf("\n EX_MEM_rd ++++++++++++++= %d", EX_MEM_rd );
	
		uint32_t sa = (0x000007C0 & EX_MEM.IR ) >> 6;
		uint32_t function = (0x0000003F & EX_MEM.IR ); 

		
		switch (function) {

					/***********************************ALU instruction**********************************************/
					
					/*************--ADD function*******************/
					case 0x00000020:
					EX_MEM.ALUOutput = IF_EX.A + IF_EX.B;
					FW_Exe=EX_MEM.ALUOutput;
					printf("\nEX/MEM.A = 0x%08x \n", IF_EX.A);
					printf("\nEX/MEM.B = 0x%08x \n", IF_EX.B);
					printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
					aa=1;
					break;
					
					/************--AND function*****************/
					case 0x00000024:
					EX_MEM.ALUOutput = IF_EX.A & IF_EX.B;
					FW_Exe=EX_MEM.ALUOutput;
					printf("\nEX/MEM.A = 0x%08x \n", IF_EX.A);
					printf("\nEX/MEM.B = 0x%08x \n", IF_EX.B);
					printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
					aa=1;
					break;
					
					/****************--XOR function*************/ 
					//(if rs and rt is different, rd = 1 else if rs and rt are same then rd is 0)
					case 0x00000026:
					EX_MEM.ALUOutput = IF_EX.A ^ IF_EX.B;
					FW_Exe=EX_MEM.ALUOutput;
					printf("\nEX/MEM.A = 0x%08x \n", IF_EX.A);
					printf("\nEX/MEM.B = 0x%08x \n", IF_EX.B);
					printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
					aa=1;
					break;
			}
		break;
		
	/******************************************************I-Type***********************************************************************/
	//(ADDIU- Add immediate unisigned)
		case 0x24000000:
		rd = (0x001f0000 & EX_MEM.IR ) >> 16;
		EX_MEM_rd = rd;
		printf("\n EX_MEM_rd ++++++++++++++= %d", EX_MEM_rd );
		EX_MEM.ALUOutput = IF_EX.A + IF_EX.imm;
		FW_Exe=EX_MEM.ALUOutput;
		printf("\nEX/MEM.A = 0x%08x \n", IF_EX.A);
		printf("\nEX/MEM.B = 0x%08x \n", IF_EX.imm);
		printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
		break;
		
	/****************--(LUI- Load upper immediate)***************/	
		case 0x3C000000:
		rd = (0x001f0000 & EX_MEM.IR ) >> 16;
		EX_MEM_rd = rd;
		printf("\n EX_MEM_rd ++++++++++++++= %d", EX_MEM_rd );
		EX_MEM.ALUOutput = IF_EX.imm << 16;
		FW_Exe=EX_MEM.ALUOutput;
		printf("\nEX/MEM.A = 0x%08x \n", IF_EX.imm);
		printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
		break;
	
	/********************--(SW- Store word)********************/
		case 0xAC000000:
		EX_MEM.ALUOutput =  IF_EX.A + IF_EX.imm;
		EX_MEM.B = IF_EX.B; //did this because the value of Rt will be needed on the next stage. 
		printf("\nEX/MEM.A = 0x%08x \n", IF_EX.A);
		printf("\nEX/MEM.B = 0x%08x \n", IF_EX.imm);
		printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
		break;
	
	/*****************--(LW- Load Word)******************/
		case 0x8C000000:
		rd = (0x001f0000 & EX_MEM.IR ) >> 16;
		EX_MEM_rd = rd;
		printf("\n EX_MEM_rd ++++++++++++++= %d", EX_MEM_rd );
		EX_MEM.ALUOutput =  IF_EX.A + IF_EX.imm;
		FW_Exe=EX_MEM.ALUOutput;
		FW_LW=1;
		printf("\nEX/MEM.A = 0x%08x \n", IF_EX.A);
		printf("\nEX/MEM.B = 0x%08x \n", IF_EX.imm);
		printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
		
		break;

	/****************--(XORI- Exclusive OR immediate)******/
		case 0x38000000:
		rd = (0x001f0000 & EX_MEM.IR ) >> 16;
		EX_MEM_rd = rd;
		printf("\n EX_MEM_rd ++++++++++++++= %d", EX_MEM_rd );
		EX_MEM.ALUOutput = IF_EX.A ^ IF_EX.imm;
		FW_Exe=EX_MEM.ALUOutput;
		printf("\nEX/MEM.A = 0x%08x \n", IF_EX.A);
		printf("\nEX/MEM.B = 0x%08x \n", IF_EX.imm);
		printf("\nEX/MEM.ALUOutput = 0x%08x \n", EX_MEM.ALUOutput);
		break;
		
	}
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                             */ 
/************************************************************/
void ID()
{
	uint32_t rs;
	uint32_t rt;
	uint32_t rd;
	uint32_t immediate_value;
	uint32_t immediate_value_unsign;
	uint32_t offset_value;
	uint32_t Op_upper;
	uint32_t target_value;
	uint32_t immediate_sign_decode;
	uint32_t immediate_value_decode;
	
	if (p ==0 && p1 ==0){
    	IF_EX.IR = ID_IF.IR;
	printf("\n**************************************Decode Stage*****************************************\n");
	printf("\nID/EX.IR=0x%08x \n", IF_EX.IR);
	print_program(ID_PC);
	uint32_t opcode = (0xFC000000 &  IF_EX.IR); 
	
	switch (opcode) {
		
		/***********************************************************R-Type************************************************************************/
		case 0x00000000:
		rs = (0x03E00000 & IF_EX.IR) >> 21;
		D_rs=rs;
	        rt = (0x001F0000 & IF_EX.IR ) >> 16;
		D_rt=rt;
		rd = (0x0000F800 & IF_EX.IR ) >> 11;
		uint32_t sa = (0x000007C0 & IF_EX.IR ) >> 6;
		uint32_t function = (0x0000003F & IF_EX.IR ); 
		
		switch (function) {
					
					/***********************************ALU instruction**********************************************/
					
					/*************--ADD function*******************/
					case 0x00000020:			
					IF_EX.A = CURRENT_STATE.REGS[rs];
					IF_EX.B = CURRENT_STATE.REGS[rt];
					if (ENABLE_FORWARDING == 0){
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
							stallflag = 1;
							A=1;
						}
						else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
							stallflag = 1;
							B=1;
						}
						else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
							stallflag = 1;
							A=1;;
						}
						else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
							stallflag = 1;
							B=1;
						}
					}
					else{
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
							IF_EX.A=FW_Exe;
							if(FW_LW==1){
								A=1;}
						}
						else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
							IF_EX.B=FW_Exe;
							if(FW_LW==1){
								B=1;}
						}

						else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) {
							IF_EX.A=FW_Mem;
						}
						else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
							IF_EX.B=FW_Mem;
						}
					}
					printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
					printf("\nID/EX.B = 0x%08x \n", IF_EX.B);
					break;
					
					/************--AND function*****************/
					case 0x00000024:
					IF_EX.A = CURRENT_STATE.REGS[rs];
					IF_EX.B = CURRENT_STATE.REGS[rt];
					if (ENABLE_FORWARDING == 0){
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
							stallflag = 1;
							A=1;
						}
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
							stallflag = 1;
							B=1;
						}
						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
							stallflag = 1;
							A=1;
						}
						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
							stallflag = 1;
							B=1;
						}
					}
					else{
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
							IF_EX.A=FW_Exe;
							if(FW_LW==1){                        //check  if last inst is LW we need 1 stall
								A=1;}
						}
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
							IF_EX.B=FW_Exe;
							if(FW_LW==1){
								B=1;}
						}

						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) { //just fw no stall
							IF_EX.A=FW_Mem;
						}
						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
							IF_EX.B=FW_Mem;
						}
					}
					printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
					printf("\nID/EX.B = 0x%08x \n", IF_EX.B);
					break;
					
					/****************--XOR function*************/ 
					//(if rs and rt is different, rd = 1 else if rs and rt are same then rd is 0)
					case 0x00000026:
					IF_EX.A = CURRENT_STATE.REGS[rs];
					IF_EX.B = CURRENT_STATE.REGS[rt];

					printf("\nfirsttttt Rt=%d Value=0x%08x \n", rt, CURRENT_STATE.REGS[rt]);
					if (ENABLE_FORWARDING == 0){
						/////we need two stall for these condition
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
							stallflag = 1;
							A=1;
						}
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
							stallflag = 1;
							B=1;
						}
						/////we need one  stall for these condition
						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
							stallflag = 1;
							sh_A=1;
							sh=1;
						}
						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
							stallflag = 1;
							sh_B=1;
							sh=1;
						}
					}
					else{
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
							IF_EX.A=FW_Exe;
							if(FW_LW==1){
								A=1;}
						}
						if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
							IF_EX.B=FW_Exe;
							if(FW_LW==1){
								B=1;}
						}
						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) {
							IF_EX.A=FW_Mem;
						}
						if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
							IF_EX.B=FW_Mem;
						}
					}
					printf("\nMEM_WB_rd=%d,   EX_MEM_rd=%d\n", MEM_WB_rd, EX_MEM_rd);
					printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
					printf("\nID/EX.B = 0x%08x \n", IF_EX.B);
					break;
					
					
					/****************--System Call function*************/ 
					case 0x0000000c:
					IF_EX.A = CURRENT_STATE.REGS[rs];
					IF_EX.B = CURRENT_STATE.REGS[rt];
					
					printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
					printf("\nID/EX.B = 0x%08x \n", IF_EX.B);
					break;
		
		
		}
		
		break;		//This break is for R-type instructions
		
/******************************************************I-Type***********************************************************************/
	
		//(ADDIU- Add immediate unisigned)
		case 0x24000000:
		rs = (0x03E00000 & IF_EX.IR) >> 21;
		D_rs=rs;
		immediate_value_unsign = (0x0000FFFF & IF_EX.IR);
		IF_EX.A = CURRENT_STATE.REGS[rs];
		IF_EX.imm = immediate_value_unsign;
		printf("\nEX_MEM_rd=0x%08x \n ",EX_MEM_rd );
		if (ENABLE_FORWARDING == 0){
			if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
				stallflag = 1;
				A=1;
			}
			else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
				stallflag = 1;
				B=1;
			}
			else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
				stallflag = 1;
				A=1;
			}
			else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
				stallflag = 1;
				B=1;
			}
		}
		else{
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					IF_EX.A=FW_Exe;
					if(FW_LW==1){
						A=1;}
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					IF_EX.B=FW_Exe;
					if(FW_LW==1){
						B=1;}
				}

				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) {
					IF_EX.A=FW_Mem;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
					IF_EX.B=FW_Mem;
				}
			}

		printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
		printf("\nID/EX.imm = 0x%08x \n", IF_EX.imm);
		
		break;
		
		/****************--(LUI- Load upper immediate)***************/	
		case 0x3C000000:
		rs = (0x03E00000 & IF_EX.IR ) >> 21;
		D_rs=rs;
		immediate_value_decode = (0x0000FFFF & IF_EX.IR);
		immediate_sign_decode = (0x0000FFFF & IF_EX.IR)>>15; 
			switch(immediate_sign_decode){

			case 0x00000001:
			immediate_value = (immediate_value_decode | 0XFFFF0000);
			break;
			
			case 0x00000000: 
			immediate_value = (immediate_value_decode | 0X000000000);
			break;

			}

			IF_EX.imm = immediate_value;

			if (ENABLE_FORWARDING == 0){
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					stallflag = 1;
					A=1;
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					stallflag = 1;
					B=1;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
					stallflag = 1;
					A=1;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
					stallflag = 1;
					B=1;
				}
			}
			else{
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					IF_EX.A=FW_Exe;
					if(FW_LW==1){
						A=1;}
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					IF_EX.B=FW_Exe;
					if(FW_LW==1){
						B=1;}
				}

				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) {
					IF_EX.A=FW_Mem;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
					IF_EX.B=FW_Mem;
				}
			}

			printf("\nID/EX.imm = 0x%08x \n", IF_EX.imm);
			break;
		
		
		/********************--(SW- Store word)********************/
		case 0xAC000000:
		rs = (0x03E00000 & IF_EX.IR) >> 21;
		D_rs=rs;
		rt = (0x001F0000 & IF_EX.IR) >> 16;
		D_rt=rt;
		immediate_value_decode = (0x0000FFFF & IF_EX.IR);
		immediate_sign_decode = (0x0000FFFF & IF_EX.IR)>>15; 
			switch(immediate_sign_decode){

			case 0x00000001:
			immediate_value = (immediate_value_decode | 0XFFFF0000);
			break;
				
			case 0x00000000: 
			immediate_value = (immediate_value_decode | 0X000000000);
			
			break;

			}
			printf("\n immediate_value = 0x%08x", immediate_value);
			if(rs == rdd){
				CURRENT_STATE.REGS[rs]=WB_Value;
			}
			IF_EX.A = CURRENT_STATE.REGS[rs];
			IF_EX.B =CURRENT_STATE.REGS[rt];
			IF_EX.imm = immediate_value;
			if (ENABLE_FORWARDING == 0){
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					stallflag = 1;
					A=1;
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					stallflag = 1;
					B=1;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
					stallflag = 1;
					A=1;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
					stallflag = 1;
					B=1;
				}
			}
			else{
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					IF_EX.A=FW_Exe;
					if(FW_LW==1){
						A=1;}
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					IF_EX.B=FW_Exe;
					if(FW_LW==1){
						B=1;}
				}

				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) {
					IF_EX.A=FW_Mem;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
					IF_EX.B=FW_Mem;
				}
			}
			printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
			printf("\nID/EX.B = 0x%08x \n", IF_EX.B);
			printf("\nID/EX.imm = 0x%08x \n", IF_EX.imm);
			break;
		
		
		/*****************--(LW- Load Word)******************/
		case 0x8C000000:
			rs = (0x03E00000 &IF_EX.IR) >> 21;
			D_rt=0;
			D_rs=rs;
			immediate_value_decode = (0x0000FFFF &IF_EX.IR);
			immediate_sign_decode = (0x0000FFFF & IF_EX.IR)>>15;
			switch(immediate_sign_decode){

			case 0x00000001:
			immediate_value = (immediate_value_decode | 0Xffff0000);
			break;
				
			case 0x00000000: 
			immediate_value = (immediate_value_decode | 0X000000000);
			break;
			}
			if(rs == rdd){
				CURRENT_STATE.REGS[rs]=WB_Value;
			}
			IF_EX.A = CURRENT_STATE.REGS[rs];
			IF_EX.imm = immediate_value;

			if (ENABLE_FORWARDING == 0){
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					stallflag = 1;
					A=1;
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					stallflag = 1;
					B=1;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
					stallflag = 1;
					A=1;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
					stallflag = 1;
					B=1;
				}
			}
			else{
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					IF_EX.A=FW_Exe;
					if(FW_LW==1){
						A=1;}
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					IF_EX.B=FW_Exe;
					if(FW_LW==1){
						B=1;}
				}

				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) {
					IF_EX.A=FW_Mem;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
					IF_EX.B=FW_Mem;
				}
			}

			printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
			printf("\nID/EX.imm = 0x%08x \n", IF_EX.imm);
			break;
		
		/****************--(XORI- Exclusive OR immediate)******/
		case 0x38000000:
			rs = (0x03E00000 & IF_EX.IR ) >> 21;
			D_rs=rs;
			D_rt=0;
			immediate_value_unsign = (0x0000FFFF & IF_EX.IR);
			IF_EX.A = CURRENT_STATE.REGS[rs];
			IF_EX.imm = immediate_value_unsign;
			if (ENABLE_FORWARDING == 0){
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					stallflag = 1;
					IF_EX.A=WB_Value;
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					stallflag = 1;
					IF_EX.B=WB_Value;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs)) {
					stallflag = 1;
					IF_EX.A=WB_Value;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt)) {
					stallflag = 1;
					IF_EX.B=WB_Value;
				}
			}
			else{
				if ((EX_MEM_rd != 0) && (EX_MEM_rd == rs)) { 
					IF_EX.A=FW_Exe;
					if(FW_LW==1){
						A=1;}
				}
				else if ((EX_MEM_rd != 0) && (EX_MEM_rd == rt)) {
					IF_EX.B=FW_Exe;
					if(FW_LW==1){
						B=1;}
				}

				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rs) && (EX_MEM_rd != rs)) {
					IF_EX.A=FW_Mem;
				}
				else if ((MEM_WB_rd != 0) && (MEM_WB_rd == rt) && (EX_MEM_rd != rt)) {
					IF_EX.B=FW_Mem;
				}
			}
			printf("\nID/EX.A = 0x%08x \n", IF_EX.A);
			printf("\nID/EX.imm = 0x%08x \n", IF_EX.imm);
		break;
		

	}

}
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{	
	if (p ==0 && p1 ==0){
		ID_IF.IR  = mem_read_32(CURRENT_STATE.PC);
		ID_PC = CURRENT_STATE.PC;
		printf("\n**************************************Fetch Stage******************************************\n");
	    	NEXT_STATE.PC   = CURRENT_STATE.PC + 4;
		ID_IF_PC=NEXT_STATE.PC;
		printf("\nCURRENT PC=0x%08x \n", CURRENT_STATE.PC );
		printf("\nIF/ID.IR=0x%08x \n", ID_IF.IR );
		print_program(CURRENT_STATE.PC);
		printf("\nIF/ID.PC=0x%08x\n", NEXT_STATE.PC);
		if (stallflag ==1){
			p=1;
		}
		if(FW_LW==1)
			p1=1;
	}
	

}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(uint32_t addr){
	/*IMPLEMENT THIS*/
	
	uint32_t rs;
	uint32_t rt;
	uint32_t rd;
	uint32_t immediate_value;
	uint32_t immediate_value_unsign;
	uint32_t offset_value;
	uint32_t Op_upper;
	uint32_t target_value;
	uint32_t immediate_sign_decode;
	uint32_t immediate_value_decode;
	int i;
	uint32_t instruction =  mem_read_32(addr);
	
	//Decode the opcode from the instruction
	uint32_t opcode = (0xFC000000 & instruction);
	
	//printf("\nOpcode = 0x%08x ",opcode);
	
	switch (opcode) {
		/**************************************************R-Type*****************************************************************************/
		case 0x00000000:
		//printf("\n R-type");
	       
		//get the address value of rs 
		rs = (0x03E00000 & instruction) >> 21;
		//printf("\n rs = %d", rs);
		
		//get the address value of rt
	        rt = (0x001F0000 & instruction) >> 16;
		//printf("\n rt = %d", rt);
		
		//get the address value of rd
		rd = (0x0000F800 & instruction) >> 11;
		//printf("\n rd = %d", rd);
		
		//get the value of shift amount
		uint32_t sa = (0x000007C0 & instruction) >> 6;
		//printf("\n sa = %d", sa);
		      
		//decoding the function bit
		uint32_t function = (0x0000003F & instruction); 
		//printf("\n function = 0x%08x", function);
		
		
		switch (function) {
		
				//ADD Instruction
				case 0x00000020:
				printf("\n Add R%d, R%d, R%d ", rd, rs, rt);
				break;
				
				//AND function
				case 0x00000024:
				printf("\n And R%d, R%d, R%d ", rd, rs, rt );
				break;
				
				//XOR function (if rs and rt is different, rd = 1 else 0)
				case 0x00000026:
				printf("\n Xor R%d, R%d, R%d ", rd, rs, rt );
				break;
				
				// system call
				case 0x0000000c:
				printf("\n c \n");
				break;
		}
		break;
		
		//(ADDIU- Add immediate unisigned)
		case 0x24000000:
		//get the address value of rs 
		rs = (0x03E00000 & instruction) >> 21;
		//printf("\n rs = %d", rs);
		
		//get the address value of rt
		rt = (0x001F0000 & instruction) >> 16;
		//printf("\n rt = %d", rt);
		
		immediate_value_unsign = (0x0000FFFF & instruction);
		printf("\n Addiu R%d, R%d, %d \n", rt, rs, immediate_value_unsign );
		break;
		
		//(LUI- Load upper immediate)
		case 0x3C000000:
		//get the address value of rs 
		rs = (0x03E00000 & instruction) >> 21;
		//printf("\n rs = %d", rs);
		
		//get the address value of rt
		rt = (0x001F0000 & instruction) >> 16;
		//printf("\n rt = %d", rt);
		
		//get the immediate value and sign extend it
		immediate_value_decode = (0x0000FFFF & instruction);
		immediate_sign_decode = (0x0000FFFF & instruction)>>15; 
			switch(immediate_sign_decode){
			case 0x00000001:
			immediate_value = (immediate_value_decode | 0Xffff0000);
			break;

			case 0x00000000: 
			immediate_value = (immediate_value_decode | 0X000000000);
			break;
			}
		printf("\n Lui R%d, %d", rt, immediate_value);
		break;
		
		//(XORI- Exclusive OR immediate)
		case 0x38000000:
		//get the address value of rs 
		rs = (0x03E00000 & instruction) >> 21;
		//printf("\n rs = %d", rs);
		
		//get the address value of rt
		rt = (0x001F0000 & instruction) >> 16;
		//printf("\n rt = %d", rt);
		
		immediate_value_unsign = (0x0000FFFF & instruction);
		printf("\n Xori R%d, R%d, %d ", rt, rs, immediate_value_unsign );
		break;
		
		
		//(SW- Store word)
		case 0xAC000000:
		//get the address value of rs 
		rs = (0x03E00000 & instruction) >> 21;
		//printf("\n rs = %d", rs);
		
		//get the address value of rt
		rt = (0x001F0000 & instruction) >> 16;
		//printf("\n rt = %d", rt);
		
		//get the immediate value and sign extend it
		immediate_value_decode = (0x0000FFFF & instruction);
		immediate_sign_decode = (0x0000FFFF & instruction)>>15; 
			switch(immediate_sign_decode){
			case 0x00000001:
			immediate_value = (immediate_value_decode | 0Xffff0000);
			break;

			case 0x00000000: 
			immediate_value = (immediate_value_decode | 0X000000000);
			break;
			}
		printf("\n Sw R%d, %d(R%d) ", rt, immediate_value, rs);
		break;
		
		
		//--(LW- Load Word)
		case 0x8C000000:
		//get the address value of rs 
		rs = (0x03E00000 & instruction) >> 21;
		//printf("\n rs = %d", rs);
		
		//get the address value of rt
		rt = (0x001F0000 & instruction) >> 16;
		//printf("\n rt = %d", rt);
		
		//get the immediate value and sign extend it
		immediate_value_decode = (0x0000FFFF & instruction);
		immediate_sign_decode = (0x0000FFFF & instruction)>>15; 
			switch(immediate_sign_decode){
			case 0x00000001:
			immediate_value = (immediate_value_decode | 0Xffff0000);
				break;
				
			case 0x00000000: 
			immediate_value = (immediate_value_decode | 0X000000000);
				break;
			}
		printf("\n Lw R%d, %d(R%d)", rt, immediate_value , rs);
		break;
		
	}		
		
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	int i;
	for(i=0; i<PROGRAM_SIZE; i++){
	printf("\n____________________________________________________________________________________________________________________________\n");
	WB();
	MEM();
	EX();
	ID();
	IF();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;}
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"
#include "mu-cache.h"

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
	printf("forwarding <val>\t-- enable forwarding with 1, disable with 0 for <val>\n");
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
	int load_stall = 0;
	for (i = 0; i < num_cycles; i++) {
		if(CACHE_MISS_FLAG == 1)
		{
			if(load_stall < 100) //100 cycles to laod from memory
			{
				load_stall++;
				CYCLE_COUNT++;
			}
			else //memory loaded
			{
				CACHE_MISS_FLAG = 0;	
			}
		}
		else
		{
			if (RUN_FLAG == FALSE) {
				printf("Simulation Stopped.\n\n");
				break;
			}
			cycle();
		}
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
		case 'F':
		case 'f':
			//enable forwarding
			if(scanf("%d", &ENABLE_FORWARDING) != 1) {
				break;
			}
			ENABLE_FORWARDING == 0 ? printf("FORWARDING IS OFF\n") : printf("FORWARDING IS ON\n");
			break;
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
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
		case 'C':
		case 'c':
			cache_miss_rate();
		default:
			printf("Invalid Command.\n");
			break;
	}
}

void cache_miss_rate()
{
	double miss_rate = ((double) cache_misses / ((double)cache_hits + (double)cache_misses) * 100);
	double hit_rate = 100 - miss_rate;
	
	printf("-------------------------------------\n");
	printf("Dumping Cache Statistics\n");
	printf("-------------------------------------\n");
	printf("Number of Cache hits: %u\n", cache_hits);
	printf("Number of Cache misses: %u\n", cache_misses);
	printf("Cache hit rate: %0.2f\n", hit_rate);
	printf("Cache miss rate: %0.2f\n", miss_rate);
	printf("-------------------------------------\n");

	printf("Cache Contenets\n");
	printf("Block\tValid\tTag\tWord 1\t\tWord 2\t\tWord 3\t\tWord 4\n");
	
	for(int i = 0; i < NUM_CACHE_BLOCKS; i ++)
	{
		//finish this thingy
		printf("[%d]\t%d\t%x\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n", i, L1Cache.blocks[i].valid, L1Cache.blocks[i].tag, L1Cache.blocks[i].words[0], L1Cache.blocks[i].words[1],L1Cache.blocks[i].words[2],L1Cache.blocks[i].words[3]);
	}
	
	printf("-------------------------------------\n");
	
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
	INSTRUCTION_COUNT = -4;
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
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */
	NEXT_STATE = CURRENT_STATE;
	if(STALL_COUNT > 0)
		STALL_COUNT--; //Decrementing stall
	WB();
	MEM();
	if(CACHE_MISS_FLAG == 1) //skip cycles if stalling
	{
		int load_stall = 0;
		if(load_stall < 100) //100 cycles to laod from memory
		{
			load_stall++;
			CYCLE_COUNT++;
		}
		else //memory loaded
		{
			CACHE_MISS_FLAG = 0;	
		}
	}
	EX();
	if(FLUSH_FLAG == 1)
	{
		STALL_COUNT = 1; //stalling ID stage
		ID();
		STALL_COUNT = 0;
		CURRENT_STATE.PC = EX_MEM.ALUOutput; //changing PC
		IF();
		FLUSH_FLAG--; //<-- may just set to zero here to avoid any seg faults
	}
	else
	{
		ID();
		IF();
	}
}

//reading from cache
uint32_t cache_reads(uint32_t addr)
{
	//uint32_t byte_offset;
	uint32_t word_offset = (addr & 0xC) >> 2;
	uint32_t index = (addr & 0xF0) >> 4;
	uint32_t word = L1Cache.blocks[index].words[word_offset];
	uint32_t tag = (addr & 0XFFFFFF00) >> 8;
	//byte_offset = addr & 0x3;
	if((L1Cache.blocks[index].valid != 1) || (L1Cache.blocks[index].tag != tag)){			
		//read a word
		L1Cache.blocks[index].tag = tag;
		L1Cache.blocks[index].words[0] = mem_read_32(addr); 
		L1Cache.blocks[index].words[1] = mem_read_32(addr + 0x4);
		L1Cache.blocks[index].words[2] = mem_read_32(addr + 0x8);
		L1Cache.blocks[index].words[3] = mem_read_32(addr + 0xC);
		word = L1Cache.blocks[index].words[word_offset];
		//increment cache miss?
		CACHE_MISS_FLAG = 1;
		cache_misses++;
	} else{
		//increment cache hit
		cache_hits++;
	}
	printf("Read from cache: %x\n",word);				
	return word;
}

//writing to cache with address and new data
void cache_writes(uint32_t addr, uint32_t new)
{
    uint32_t word_offset = (addr & 0x0000000C) >> 2;
    uint32_t index = (addr & 0x000000F0) >> 4;
    uint32_t tag = (addr & 0xFFFFFF00) >> 8;
    
    if(L1Cache.blocks[index].tag != tag || L1Cache.blocks[index].valid != 1){
        L1Cache.blocks[index].tag = tag;
        L1Cache.blocks[index].words[0] = mem_read_32((addr & 0xFFFFFFF0));
        L1Cache.blocks[index].words[1] = mem_read_32((addr & 0xFFFFFFF0) + 0x04);
        L1Cache.blocks[index].words[2] = mem_read_32((addr & 0xFFFFFFF0) + 0x08);
        L1Cache.blocks[index].words[3] = mem_read_32((addr & 0xFFFFFFF0) + 0x0C);
        CACHE_MISS_FLAG = 1; 
        L1Cache.blocks[index].valid = 1;
        cache_misses++;
    }else{
        cache_hits++;
    }

    L1Cache.blocks[index].words[word_offset] = new;
    
    mem_write_32((addr & 0xFFFFFFF0), L1Cache.blocks[index].words[0]);
    mem_write_32(((addr & 0xFFFFFFF0) + 0x04), L1Cache.blocks[index].words[1]);
    mem_write_32(((addr & 0xFFFFFFF0) + 0x08), L1Cache.blocks[index].words[2]);
    mem_write_32(((addr & 0xFFFFFFF0) + 0x0C), L1Cache.blocks[index].words[3]);
    printf("Wrote to cache: %x\n", new);
}

//writing back to registers, increment instruction count at this stage
/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB() //may need to add more insturctions here, will look later
{
	//if bubble happening then skip stage
	if(MEM_WB.stage_stalled == 1)
		return; 
	
	uint32_t opcode = (MEM_WB.IR & 0xFC000000) >> 26;
	
	//printf("opcode = %x\n", opcode);
	
	if(opcode == 0) //r type
	{
		//r type command
		uint32_t funct = (MEM_WB.IR & 0x0000003F);// instruction function field	
		uint32_t rd = (MEM_WB.IR & 0x0000F800) >> 11;//destination reg

		rtypeWB(funct, rd);
	}
	else //i or j type (but not jump or branching)
	{
		uint32_t rt = (MEM_WB.IR & 0x001F0000) >> 16; //finding rt register
		
		switch(opcode)
		{
			case 0b001000: { //add immediate
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			}
			case 0b001001: { //add immediate unsigned
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;	
			}
			case 0b001100: { //and immediate
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;	
			}
			case 0b001101: { //ORI 
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			}
			case 0b001110: { //Xori 
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			}
			case 0b001111: { //load upper immediate
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput<<16;
				break;
			}
			case 0b001010: { //SLTI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			}
			case 0b100000: { //Loading byte 
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				break;	
			}
			case 0b100001:{ //Loading halfword
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				break;	
			}
			case 0b100011: { //Load word, 32 bits
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				break;	
			}
			case 0b000011: { //JAL
				NEXT_STATE.REGS[31] = MEM_WB.PC + 4; 	
			}
			default: {
				//not handled yet	
			}
		}
	}
	CURRENT_STATE = NEXT_STATE;
	
	INSTRUCTION_COUNT++; //increasing instriction count after WB stage, don't know if end is good idea but c'est la vie
}

//writing back r type commands, most to just next state destination reg
void rtypeWB(uint32_t funct, uint32_t rd)
{
	switch(funct)
	{
		case 0b100000: { //ADD, pulled from brenden's alu ops because I didn't want to write out all the codes again
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b100001: { //ADDIU
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
		}
		case 0b100010: { //SUB
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b100011: { //SUBU
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b100100: { //AND
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b100101: { //OR
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b100110: { //XOR
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b100111: { //NOR
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b101010: { //SLT
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b011000: { //MULT
			NEXT_STATE.HI = MEM_WB.HI;
			NEXT_STATE.LO = MEM_WB.LO;
			break;
		}
		case 0b011001: { //MULTU
			NEXT_STATE.HI = MEM_WB.HI;
			NEXT_STATE.LO = MEM_WB.LO;
			break;
		}
		case 0b010000: { //MFHI
			NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
			break;
		}
		case 0b010010: { //MFLO
			NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
			break;
		}
		case 0b010001: { //MTHI
			NEXT_STATE.HI = MEM_WB.ALUOutput;
			break;
		}
		case 0b010011: { //MTLO
			NEXT_STATE.LO = MEM_WB.ALUOutput;
			break;
		}
		case 0b011010: { //DIV
			NEXT_STATE.HI = MEM_WB.HI;
			NEXT_STATE.LO = MEM_WB.LO;
			break;
		}
		case 0b011011: { //DIVU
			NEXT_STATE.HI = MEM_WB.HI;
			NEXT_STATE.LO = MEM_WB.LO;
			break;
		}
		case 0b000000: { //SLL
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b000010: { //SRL
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b000011: { //SRA
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			break;
		}
		case 0b001001: { //JALR, may need to change in future
			//NEXT_STATE.REGS[rd] = MEM_WB.LMD;
			break;	
		}
		case 0x0000000C: { //syscall
			if(NEXT_STATE.REGS[2] == 0xA)
			{
				RUN_FLAG = FALSE;	
				MEM_WB.ALUOutput = 0x0;
				break;
			}
			printf("SYSCALL\n");
			break;
		}
		default: {
			//not handled yet			
		}
	}
}

//memory accessed
/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	MEM_WB.stage_stalled = EX_MEM.stage_stalled;
	MEM_WB.REG_RD_VALUE = EX_MEM.REG_RD_VALUE;
	MEM_WB.REG_RS_VALUE = EX_MEM.REG_RS_VALUE;
	MEM_WB.REG_RT_VALUE = EX_MEM.REG_RT_VALUE;
	//if stalled then only pass forward stall
	if(EX_MEM.stage_stalled == 1)
	{
		printf("MEM stage stalled\n"); //just for debugging
		MEM_WB.stage_stalled = 1;
		MEM_WB.REG_RD_VALUE = 0;
		MEM_WB.REG_RS_VALUE = 0;
		MEM_WB.REG_RT_VALUE = 0;
		MEM_WB.LO = 0;
		MEM_WB.HI = 0;
		return;
	}
	
	if(EX_MEM.Forwarding_Type==3 || EX_MEM.Forwarding_Type==7) {
	}
	if(EX_MEM.Forwarding_Type==8) {
		EX_MEM.B=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
		MEM_WB.B=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
		EX_MEM.LMD=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
		MEM_WB.LMD=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
	}

	if(EX_MEM.Forwarding_Type==12) {
		EX_MEM.B=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
		MEM_WB.B=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
		EX_MEM.LMD=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
		MEM_WB.LMD=NEXT_STATE.REGS[EX_MEM.REG_RT_VALUE];
	}
	//forwarding the values from pipeline regsisters
	MEM_WB.IR = EX_MEM.IR; //instruction
	MEM_WB.ALUOutput = EX_MEM.ALUOutput; //forwarding output before manpulating
	MEM_WB.ALUOutputLow = EX_MEM.ALUOutputLow;
	MEM_WB.PC = EX_MEM.PC; //program counter
	MEM_WB.HI = EX_MEM.HI;
	MEM_WB.LO = EX_MEM.LO;
	
	uint32_t opcode = (MEM_WB.IR & 0xFC000000) >> 26; ///getting opcode
		
	if(opcode == 0x00000000) //opcode is zero, r-type command, most likely syscall 
	{
		opcode = MEM_WB.IR & 0x0000003F; //finding function	field
		if(opcode == 0x0C)
		{
			//SYS call, nothing to do with memory access	
		}
	}
	else //other two types of commmands (stuff that actually memory access)
	{
		switch(opcode)
		{
			//loading instructions
			case 0b100000: { //Loading byte of 8 bits, cache read then get from mem if needed
				//reading in cache then putting into MEM_WB.LMD for safe keeping
				uint32_t temp_data = cache_reads(EX_MEM.ALUOutput);
				MEM_WB.LMD = ((temp_data & 0x000000FF) & 0x80) > 0 ? (temp_data | 0xFFFFFF00) : (temp_data & 0x000000FF);
				break;
			}
			case 0b100001:{ //Loading halfword, ditto as above
				uint32_t temp_data = cache_reads(EX_MEM.ALUOutput);
				MEM_WB.LMD = ((temp_data & 0x0000FFFF) & 0x8000) > 0 ? (temp_data | 0xFFFF0000) : (temp_data & 0x0000FFFF);
				break;
			}
			case 0b100011: { //Load word, 32 bits, ditto as above
				MEM_WB.LMD = cache_reads(EX_MEM.ALUOutput);
				break;
			}
			case 0b101000:{ //storing byte, cache write
				uint32_t temp_data = cache_reads(EX_MEM.ALUOutput);
				temp_data = (temp_data & 0xFFFFFF00) | (EX_MEM.B & 0x000000FF);
				cache_writes(EX_MEM.ALUOutput, EX_MEM.B);
			}
			case 0b101001: { //storing halfword, cache write
				uint32_t temp_data = cache_reads(EX_MEM.ALUOutput);
				temp_data = (temp_data & 0xFFFF0000) | (EX_MEM.B & 0x0000FFFF);
				cache_writes(EX_MEM.ALUOutput, EX_MEM.B);
			}
			case 0b101011: { //store word, cache write
				cache_writes(EX_MEM.ALUOutput, EX_MEM.B);
			}
		}
	}
}

int i = 1;

//instruction executed
/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	

	// printf("Execution #%i\n", i++);
	if(ID_EX.stage_stalled == 1)
	{
		//execute stage stalled
		EX_MEM.stage_stalled = 1;
		EX_MEM.imm = 0;
		EX_MEM.IR = 0;
		EX_MEM.A = 0;
		EX_MEM.B = 0;
		EX_MEM.HI = 0;
		EX_MEM.LO = 0;
		EX_MEM.REG_RD_VALUE = 0;
		EX_MEM.REG_RS_VALUE = 0;
		EX_MEM.REG_RT_VALUE = 0;
		EX_MEM.PC = 0;
		EX_MEM.ALUOutput = 0;
		printf("EX stalled\n");
	}
	if(ID_EX.stage_stalled == 0)
	{	
		printf("[0x%08X]\t", ID_EX.PC);
		uint32_t instr = (mem_read_32(ID_EX.PC)); //reading in address from mem
		print_instruction(instr);
		EX_MEM.stage_stalled =ID_EX.stage_stalled;
		EX_MEM.Forwarding_Type = ID_EX.Forwarding_Type;
		EX_MEM.IR = ID_EX.IR;
		EX_MEM.A = ID_EX.A;
		EX_MEM.B = ID_EX.B;
		EX_MEM.HI = 0;
		EX_MEM.LO = 0;
		EX_MEM.REG_RD_VALUE = ID_EX.REG_RD_VALUE;
		EX_MEM.REG_RS_VALUE = ID_EX.REG_RS_VALUE;
		EX_MEM.REG_RT_VALUE = ID_EX.REG_RT_VALUE;
		EX_MEM.PC = ID_EX.PC;
		EX_MEM.imm =ID_EX.imm;
		EX_MEM.ALUOutput = 0;
		
		if(ID_EX.Forwarding_Type == 1 || ID_EX.Forwarding_Type == 5) {
			ID_EX.A = MEM_WB.ALUOutput;
			EX_MEM.A = ID_EX.A;
		}
		else if(ID_EX.Forwarding_Type == 2 || ID_EX.Forwarding_Type == 6) {
			ID_EX.B = MEM_WB.ALUOutput;
			EX_MEM.B = ID_EX.B;
		}
		else if(ID_EX.Forwarding_Type == 3) {
			ID_EX.A = NEXT_STATE.REGS[ID_EX.REG_RS_VALUE];
			EX_MEM.A = ID_EX.A;
		}
		else if(ID_EX.Forwarding_Type == 4) {
			ID_EX.B = NEXT_STATE.REGS[ID_EX.REG_RT_VALUE];
			EX_MEM.B = ID_EX.B;
		}
		else if(ID_EX.Forwarding_Type == 7) {
			ID_EX.A = MEM_WB.LMD;
		}
		else if(ID_EX.Forwarding_Type == 8) {
			ID_EX.B = MEM_WB.LMD;
			EX_MEM.B = ID_EX.B;
		}
		else if(ID_EX.Forwarding_Type == 9|| ID_EX.Forwarding_Type == 11) {
			ID_EX.A = MEM_WB.ALUOutput;
			EX_MEM.A = ID_EX.A;
			ID_EX.B = NEXT_STATE.REGS[ID_EX.REG_RT_VALUE];
			EX_MEM.B = ID_EX.B;
		}
		else if(ID_EX.Forwarding_Type == 10) {
			ID_EX.A = NEXT_STATE.REGS[ID_EX.REG_RD_VALUE];
			EX_MEM.A = ID_EX.A;
		}
		else if(ID_EX.Forwarding_Type == 12) {
			ID_EX.A = NEXT_STATE.REGS[ID_EX.REG_RS_VALUE];
			EX_MEM.A = ID_EX.A;
		}
		else if(ID_EX.Forwarding_Type == 13) {
			ID_EX.A = NEXT_STATE.REGS[ID_EX.REG_RS_VALUE];
			EX_MEM.A = ID_EX.A;
			ID_EX.B = MEM_WB.ALUOutput;
			EX_MEM.B = ID_EX.B;
		}
		else if(ID_EX.Forwarding_Type == 14) {
			ID_EX.A = NEXT_STATE.REGS[ID_EX.REG_RS_VALUE];
			EX_MEM.A = ID_EX.A;
			ID_EX.B = MEM_WB.ALUOutput;
			EX_MEM.B = ID_EX.B;
		}
		
		if(load_store(ID_EX.IR>>26, ID_EX.IR & 0x0000003F)==3) {		
			EX_MEM.ALUOutput = ID_EX.A;
		}
		else if(reg_jump(ID_EX.IR>>26, ID_EX.IR & 0x3F)) {
			branch_jump(ID_EX.IR>>26);
		}
		else if(load_store(ID_EX.IR>>26, ID_EX.IR & 0x0000003F)) {
			printf("0x%x\n", ID_EX.A);
			if(EX_MEM.imm >> 15) //sign extend
			{
				EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm; 	
			}
			EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;
		}
		else if(reg_imm(ID_EX.IR>>26)) {
			EX_MEM.ALUOutput = ALUOperationI();
		}
		else if(reg_reg(ID_EX.IR>>26, ID_EX.IR & 0x0000003F)) { //reg-reg
			EX_MEM.ALUOutput = ALUOperationR();
		}
		else if(ID_EX.IR != 0xC) {
			// printf("ERROR EX\n");
		}
	}
}

uint32_t ALUOperationI() {
	switch(ID_EX.IR >> 26) {
		case 0b001000: { //ADDI
			if(ID_EX.imm >> 15) {
				ID_EX.imm = 0xFFFF0000 | ID_EX.imm; //sign extend	
			}
			uint8_t bit30_carry = (((ID_EX.imm >> 30) & 0x1) + (0x1 & (ID_EX.A >> 30))) >> 1;
			uint8_t bit31_carry = (((ID_EX.imm >> 31) & 0x1) + (0x1 & (ID_EX.A >> 31))) >> 1;
			if(bit30_carry == bit31_carry)
				return ID_EX.A + ID_EX.imm;
			break;
		}
		case 0b001001: { //ADDIU, unsigned so not carry bits
			//printf("EX_MEM.A = %X\n", EX_MEM.A);
			//printf("EX_MEM.imm = %X\n", EX_MEM.imm);
			//printf("result = %X\n", EX_MEM.A + EX_MEM.imm);
			return EX_MEM.A + EX_MEM.imm;
			break;
		}
		case 0b001100: { //ANDI
			return ID_EX.A & ID_EX.imm;
			break;
		}
		case 0b001101: { //ORI
			return ID_EX.A | ID_EX.imm;
			break;
		}
		case 0b001110: { //XORI
			return ID_EX.A ^ ID_EX.imm;
			break;
		}
		case 0b001010: { //SLTI
			if((int32_t) ID_EX.A < (int16_t) ID_EX.imm) {
				return 1;
			} 
			else {
				return 0;
			}
		}
		default:
			return 0;
	}
	return 0;
}

uint32_t ALUOperationR() {
	switch(ID_EX.IR & 0x0000003F) {
		case 0b100000: { //ADD
			return ID_EX.A + ID_EX.B;
			break;
		}
		case 0b100001: { //ADDU
			return ID_EX.A + ID_EX.B;
			break;
		}
		case 0b100010: { //SUB
			return ID_EX.A - ID_EX.B;
			break;
		}
		case 0b100011: { //SUBU
			return ID_EX.A - ID_EX.B;
			break;
		}
		case 0b100100: { //AND
			return ID_EX.A & ID_EX.B;
			break;
		}
		case 0b100101: { //OR
			return ID_EX.A | ID_EX.B;
			break;
		}
		case 0b100110: { //XOR
			return ID_EX.A ^ ID_EX.B;
			break;
		}
		case 0b100111: { //NOR
			return ~(ID_EX.A | ID_EX.B);
			break;
		}
		case 0b101010: { //SLT
			if((int32_t) ID_EX.A < (int32_t) ID_EX.B) {
				return 1;
			} else {
				return 0;
			}
		}
		case 0b011000: { //MULT
			if(EX_MEM.A >> 31)
			{
				EX_MEM.A = 0xFFFFFFFF00000000 | EX_MEM.A; //sign extend with 1's
			}
			if(EX_MEM.B >> 31)
			{
				EX_MEM.B = 0xFFFFFFFF00000000 | EX_MEM.B; //sign extend with 1's
			}
			int64_t result = EX_MEM.A * EX_MEM.B;
			EX_MEM.LO = result;
			EX_MEM.HI = (result) >> 32;
			return;
		}
		case 0b011001: { //MULTU
			uint64_t result = (uint32_t) EX_MEM.A * (uint32_t) EX_MEM.B;
			EX_MEM.LO = result;
			EX_MEM.HI = (result) >> 32;
			return;
		}
		case 0b011010: { //DIV
			EX_MEM.LO = EX_MEM.A / EX_MEM.B;
			EX_MEM.HI = EX_MEM.A % EX_MEM.B;
			//EX_MEM.ALUOutputLow = (int32_t) ID_EX.A / (int32_t) ID_EX.B;
			return;
		}
		case 0b011011: { //DIVU
			EX_MEM.LO = EX_MEM.A / EX_MEM.B;
			EX_MEM.HI = EX_MEM.A % EX_MEM.B;
			return 0;
		}
		case 0b000000: { //SLL
			return ID_EX.B << ID_EX.sham_t;
		}
		case 0b000010: { //SRL
			return ID_EX.B >> ID_EX.sham_t;
		}
		case 0b000011: { //SRA
			return (ID_EX.B >> ID_EX.sham_t) | (ID_EX.B & 0x80000000);
		}
		case 0b001000: { //JR
			FLUSH_FLAG = 1;
			CURRENT_STATE.PC = CURRENT_STATE.REGS[31];
			return ID_EX.A;
		}
		case 0b001001: { //JALR
			FLUSH_FLAG = 1;
			EX_MEM.LMD = CURRENT_STATE.PC + 4;
			return ID_EX.A;
		}
		case 0b010000: { //MFHI
			break;			
		}
		case 0b010010: { //MFLO
			break;			
		}
		case 0b010011: {
			EX_MEM.ALUOutput = EX_MEM.A;	
		}
		default: {
			return 0;
		}
	}
	return 0;
}

//returns 1 for load 2 for store 3 if accessing HI or LO registers and 0 if niether
int load_store(uint32_t opcode, uint32_t regreg) {
	switch(opcode) {
		case 0b100011:
		case 0b100000:
		case 0b100001:
		case 0b001111:
			return 1;
		case 0b101011:
		case 0b101000: //store byte
		case 0b101001:
			return 2;
		case 0b000000: {
			switch(regreg) {
				case 0b010000:
				case 0b010001:
				case 0b010010:
				case 0b010011:
					return 3;
				default:
					return 0;
			}
		}
		default:
			return 0;
	}
}

//brnach and jump instrucions function
int branch_jump(uint32_t opcode)
{
	printf("%d\n", opcode);
	switch(opcode) {
		case 0b000100: {//BEQ
			if(EX_MEM.A == EX_MEM.B) //branch is taken
			{
				/*if(EX_MEM.imm >> 15) //negative immediate so sign extend
				{
					   EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm;
				}*/
				
				int32_t offset = EX_MEM.imm << 2;
				EX_MEM.ALUOutput = EX_MEM.PC + offset; //finding outcome of branch
				FLUSH_FLAG = 1; //flush flag enabled
			}
			// else
				//no branch
			break;
		}
		case 0b000101: {//BNE
			if(EX_MEM.A != EX_MEM.B) //branch is taken
			{
				/*if(EX_MEM.imm >> 15) //negative immediate so sign extend
				{
					   EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm;
				}*/

				int32_t offset = EX_MEM.imm << 2;
				EX_MEM.ALUOutput = EX_MEM.PC + offset; //finding outcome of branch
				FLUSH_FLAG = 1; //flush flag enabled
			}
			// else
				//no branch
			break;
		}
		case 0b000110: {//BLEZ
			if(EX_MEM.A >> 31 || EX_MEM.A==0) //branch is taken
			{
				/*if(EX_MEM.imm >> 15) //negative immediate so sign extend
				{
					   EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm;
				}*/
				
				int32_t offset = EX_MEM.imm << 2;
				EX_MEM.ALUOutput = EX_MEM.PC + offset; //finding outcome of branch
				FLUSH_FLAG = 1; //flush flag enabled
			}
			// else
				//no branch
			break;
		}
		case 0b000111: {//BGTZ
			if(!(EX_MEM.A >> 31)) //branch is taken
			{
				/*if(EX_MEM.imm >> 15) //negative immediate so sign extend
				{
					   EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm;
				}*/
				
				int32_t offset = EX_MEM.imm << 2;
				EX_MEM.ALUOutput = EX_MEM.PC + offset; //finding outcome of branch
				FLUSH_FLAG = 1; //flush flag enabled
			}
			//else
				//no branch
			break;
		}
		case 0b000001: { //special branch instructions
		
			if(EX_MEM.REG_RT_VALUE == 00001) //BGEZ
			{
				//printf("Called BGEZ\n");
				if(!(EX_MEM.A >> 15) || EX_MEM.A == 0) //taking branch
				{
					/*if(EX_MEM.imm >> 15) //negative immediate so sign extend
					{
					   EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm;
					}*/
				
					int32_t offset = EX_MEM.imm << 2;
					EX_MEM.ALUOutput = EX_MEM.PC + offset; //finding outcome of branch
					FLUSH_FLAG = 1; //flush flag enabled
				}
				// else
					//no branch
				//break; 
			}
			if(EX_MEM.REG_RT_VALUE == 00000) //BLTZ
			{
				if((EX_MEM.A >> 15)) //taking branch
				{
					/*if(EX_MEM.imm >> 15) //negative immediate so sign extend
					{
					   EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm;
					}*/
					
					int32_t offset = EX_MEM.imm << 2;
					EX_MEM.ALUOutput = EX_MEM.PC + offset; //finding outcome of branch
					FLUSH_FLAG = 1; //flush flag enabled
				}
				//else
					//no branch
				//break; 
			}
			break;
		}
		case 0b000010: { //J
			FLUSH_FLAG = 1;
			EX_MEM.ALUOutput = ((EX_MEM.IR) & 0x03FFFFFF)<<2;
			break;
		}
		case 0b000011: { //JAL
			FLUSH_FLAG = 1;
			//printf("EX_MEM IR = %X\n EX_MEM.imm = %X\n", ((EX_MEM.IR) & 0x03FFFFFF<< 2), EX_MEM.imm << 2);
			EX_MEM.ALUOutput = (((EX_MEM.IR) & 0x03FFFFFF) << 2);
			break;
		}
		case 0b000000: { 
			switch(EX_MEM.IR & 0x3F) {
				case 0b001000: { //JR
					FLUSH_FLAG = 1;
					EX_MEM.ALUOutput = ID_EX.A;
					//NEXT_STATE.PC = EX_MEM.A;
					break;
				}
				case 0b001001: { //JALR
					FLUSH_FLAG = 1;
					EX_MEM.ALUOutput = NEXT_STATE.REGS[EX_MEM.REG_RS_VALUE];
					EX_MEM.ALUOutput = ID_EX.A;
					EX_MEM.LMD = CURRENT_STATE.PC + 4;
					printf("Forwarding Type: %d\n", EX_MEM.Forwarding_Type);
					printf("EX_MEM.A: 0x%x\n", EX_MEM.A);
					printf("EX_MEM.B: 0x%x\n", EX_MEM.B);
					printf("EX_MEM.REG_RT_VALUE: 0x%x\n", EX_MEM.REG_RT_VALUE);
					printf("EX_MEM.REG_RS_VALUE: 0x%x\n", EX_MEM.REG_RS_VALUE);
					printf("EX_MEM.REG_RD_VALUE: 0x%x\n", EX_MEM.REG_RD_VALUE);
					printf("EX_MEM.CorrectValue: 0x%x\n", NEXT_STATE.REGS[EX_MEM.REG_RS_VALUE]);
					printf("EX_MEM.ALUOutput: 0x%x\n", EX_MEM.ALUOutput);
					//printf("EX_MEM IR = %X\n EX_MEM.imm = %X\n", ((EX_MEM.IR) & 0x03FFFFFF<< 2), EX_MEM.imm << 2);
					break;
				}
			}
		}
	}
	//printf("Test\n");
	return 0;
}

int reg_imm(uint32_t opcode) {
	switch(opcode) {
		case 0b001000:
		case 0b001001:
		case 0b001100:
		case 0b001101:
		case 0b001110:
		case 0b001010:
			return 1;
		default:
			return 0;
	}
}

int reg_jump(uint32_t opcode, uint32_t instruction) {
	switch(opcode) {
		case 0b000100:
		case 0b000101:
		case 0b000110:
		case 0b000111:
		case 0b000001:
		case 0b000010:
		case 0b000011:
			return 1;
		case 0b000000:
			switch(instruction) {
				case(0b001000):
				case(0b001001):
					return 2;
			    default:
					return 0;
			}
		default:
			return 0;
	}
}

int reg_reg(uint32_t opcode, uint32_t instruction) {
	if(opcode == 0b000000) {
		switch(instruction) {
			case 0b100000:
			case 0b100001:
			case 0b100010:
			case 0b100011:
			case 0b011000:
			case 0b011001:
			case 0b011010:
			case 0b011011:
			case 0b100100:
			case 0b100101:
			case 0b100110:
			case 0b100111:
			case 0b101010:
			case 0b000000:
			case 0b000010:
			case 0b000011:
				return 1;
			default:
				return 0;
		}
	}
	return 0;
}

//This is where the instruction is actually determined by the bit fields
/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{		
	if(STALL_COUNT > 0)
	{
		//ID stage stalled for jump/branch outcome to be found first
		ID_EX.stage_stalled = 1;
		IF_ID.IR = ID_EX.IR;
		printf("ID stalling\n");
		return;
	}
	if(STALL_COUNT == 0)
	{
		ID_EX.PC = IF_ID.PC;
		ID_EX.IR = IF_ID.IR; //transfering instruction
		ID_EX.imm = IF_ID.IR & 0x00008000 ? IF_ID.IR | 0xFFFF0000 : IF_ID.IR & 0x0000FFFF; //immediate
		ID_EX.A = CURRENT_STATE.REGS[(IF_ID.IR>>21) & 0x1F]; //rs reg
		ID_EX.B = CURRENT_STATE.REGS[(IF_ID.IR>>16) & 0x1F]; //rt reg
		ID_EX.PC = IF_ID.PC; //pc transfer
		ID_EX.REG_RS_VALUE = (IF_ID.IR>>21) & 0x1F;
		ID_EX.REG_RT_VALUE = (IF_ID.IR>>16) & 0x1F;
		ID_EX.REG_RD_VALUE = (IF_ID.IR>>11) & 0x1F;
		ID_EX.sham_t = (IF_ID.IR>>6) & 0x1F;
		ID_EX.stage_stalled = 0;
		ID_EX.Forwarding_Type = 0;
		if(STALL_COUNT==0)
		{
			dataHazardDetection();
			
			uint8_t opcode = (IF_ID.IR & 0xFC000000) >> 26;
			
			if(opcode == 0)
			{
				opcode = IF_ID.IR & 0x0000003F; //funct field
				
				switch(opcode)	
				{
					case 0b001000: { //jr
						ID_EX.A = NEXT_STATE.REGS[(IF_ID.IR & 0x03E00000) >> 21];
						
						IF_ID.stage_stalled = 1;
						break;
					}
					case 0b001001: { //jalr
						ID_EX.A = NEXT_STATE.REGS[(IF_ID.IR & 0x03E00000) >> 21];
						//ID_EX.REG_RD_VALUE = rd
						IF_ID.stage_stalled = 1;
					}
				}
			}
		}
		if(STALL_COUNT > 0)
		{
			ID_EX.stage_stalled = 1;
			ID_EX.IR = 0xFFFFFFFF;
			ID_EX.REG_RD_VALUE = -1;
			ID_EX.REG_RT_VALUE = -1;
			ID_EX.REG_RS_VALUE = -1;
			printf("Stall at ID stage at %x\n", CURRENT_STATE.PC);
			return;
		}
		if(STALL_COUNT != 0)
		{
			printf("data hazard in ID, so stalled\n");
			ID_EX.stage_stalled = 1;
		}
	}
}

//function to detect data hazard in pipeline
void dataHazardDetection()
{
	int enabled_forwarding = ENABLE_FORWARDING;
	// //if accessing memory in exe to mem or mem to wb, turn off forwarding
	// if(EX_MEM.MEM_ACCESS_FLAG == 1 || MEM_WB.MEM_ACCESS_FLAG == 1)
	// 	enabled_forwarding = 0; //turn off enable forwarding
	if (reg_jump(ID_EX.IR>>26, ID_EX.IR & 0x0000003F)==2){
		return;
	} 
	else if(reg_imm(EX_MEM.IR>>26) && (EX_MEM.REG_RT_VALUE == ID_EX.REG_RS_VALUE) && (MEM_WB.REG_RD_VALUE != 0) && (MEM_WB.REG_RD_VALUE == ID_EX.REG_RT_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 11;
		else
			STALL_COUNT = 2;
	}
	else if(reg_imm(EX_MEM.IR>>26) && (EX_MEM.REG_RT_VALUE == ID_EX.REG_RS_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 5;
		else
			STALL_COUNT = 2;
	}
	else if(reg_imm(EX_MEM.IR>>26) && (EX_MEM.REG_RT_VALUE == ID_EX.REG_RT_VALUE) && ((MEM_WB.REG_RD_VALUE != 0)) && (MEM_WB.REG_RD_VALUE == ID_EX.REG_RS_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 13;
		else
			STALL_COUNT = 2;
	}
	else if(load_store(ID_EX.IR>>26, ID_EX.IR & 0x0000003F) && (ID_EX.REG_RT_VALUE == EX_MEM.REG_RT_VALUE) && (ID_EX.REG_RS_VALUE == MEM_WB.REG_RT_VALUE)) {
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 14;
		else
			STALL_COUNT = 2;
	}
	else if(reg_imm(EX_MEM.IR>>26) && (EX_MEM.REG_RT_VALUE == ID_EX.REG_RT_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 6;
		else
			STALL_COUNT = 2;
	}
	else if(load_store(EX_MEM.IR>>26, EX_MEM.IR & 0x0000003F) && (EX_MEM.REG_RT_VALUE == ID_EX.REG_RS_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 7;
		else
			STALL_COUNT = 2;
	}
	else if(load_store(EX_MEM.IR>>26, EX_MEM.IR & 0x0000003F) && (EX_MEM.REG_RT_VALUE == ID_EX.REG_RT_VALUE) && load_store(MEM_WB.IR>>26, MEM_WB.IR & 0x0000003F) && (MEM_WB.REG_RT_VALUE == ID_EX.REG_RS_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 12;
		else
			STALL_COUNT = 2;
	}
	else if(load_store(EX_MEM.IR>>26, EX_MEM.IR & 0x0000003F) && (EX_MEM.REG_RT_VALUE == ID_EX.REG_RT_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 8;
		else
			STALL_COUNT = 2;
	}
	else if(load_store(MEM_WB.IR>>26, MEM_WB.IR & 0x0000003F) == 3 && MEM_WB.REG_RD_VALUE != 0 && MEM_WB.REG_RD_VALUE == ID_EX.REG_RS_VALUE) {
		if(enabled_forwarding == 1)
		{
			ID_EX.Forwarding_Type = 10;
		}
	}
	else if(((EX_MEM.REG_RD_VALUE != 0)) && (EX_MEM.REG_RD_VALUE == ID_EX.REG_RS_VALUE) && (reg_imm(MEM_WB.IR>>26) && (MEM_WB.REG_RT_VALUE == ID_EX.REG_RT_VALUE)))
	{
		if(enabled_forwarding == 1)
		{
			ID_EX.Forwarding_Type = 9;
		}
		else
			STALL_COUNT = 2;
	}
	else if(((EX_MEM.REG_RD_VALUE != 0)) && (EX_MEM.REG_RD_VALUE == ID_EX.REG_RS_VALUE))
	{
		if(enabled_forwarding == 1)
		{
			ID_EX.Forwarding_Type = 1;
		}
		else
			STALL_COUNT = 2;
	}
	else if(((EX_MEM.REG_RD_VALUE != 0)) && (EX_MEM.REG_RD_VALUE == ID_EX.REG_RT_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 2;
		else
			STALL_COUNT = 2;
	}
	else if(((MEM_WB.REG_RD_VALUE != 0)) && (MEM_WB.REG_RD_VALUE == ID_EX.REG_RS_VALUE))
	{
		if(enabled_forwarding == 1)
		{
			ID_EX.Forwarding_Type = 3;
		}
		else
			STALL_COUNT = 1;
	}
	else if(((MEM_WB.REG_RD_VALUE != 0)) && (MEM_WB.REG_RD_VALUE == ID_EX.REG_RT_VALUE))
	{
		if(enabled_forwarding == 1)
			ID_EX.Forwarding_Type = 4;
		else
			STALL_COUNT = 1;
	}
	
	if(EX_MEM.JUMP_BRANCH_FLAG == 1)
		STALL_COUNT = 1;
		
		
	//for debugging
	// printf("%d\n", reg_imm(EX_MEM.IR>>26));
	// printf("%d|%d|%d\n", ID_EX.REG_RS_VALUE, ID_EX.REG_RT_VALUE, ID_EX.REG_RD_VALUE);
	// printf("%d|%d|%d\n", EX_MEM.REG_RS_VALUE, EX_MEM.REG_RT_VALUE, EX_MEM.REG_RD_VALUE);			
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	if(STALL_COUNT == 0)
	{
		if(IF_ID.IR == 0x0000000C)
			return; 
		IF_ID.IR = mem_read_32(CURRENT_STATE.PC);
		IF_ID.PC = CURRENT_STATE.PC;
		NEXT_STATE.PC = CURRENT_STATE.PC + sizeof(uint32_t); //incrementing program counter by four for next state
	}
	else
		printf("Stalling at IF stage\n");
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	cache_misses = 0;
	cache_hits = 0;
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
//from lab one
void print_program(){
	/*IMPLEMENT THIS*/
	uint32_t instr;
	uint32_t address;
	
	for(int i = 0; i < PROGRAM_SIZE; i++)
	{
			address = MEM_TEXT_BEGIN + (i*4);
			printf("[0x%08X]\t", address);
			instr = (mem_read_32(address)); //reading in address from mem
			print_instruction(instr);
	}

}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
	//ID_RF part of the lab
	printf("Current PC: %08X\n\n", CURRENT_STATE.PC); //may need to be one the one from the pipeline regs
	
	printf("IF/ID %08X ", IF_ID.IR); print_instruction(IF_ID.IR);
	printf("IF/ID.PC %08X\n\n", IF_ID.PC);
	
	//EX part
	printf("ID/EX.IR %08X ", ID_EX.IR); print_instruction(ID_EX.IR);
	printf("ID/EX.A %08X\n", ID_EX.A);
	printf("ID/EX.B %08X\n", ID_EX.B);
	printf("ID/EX.imm %08X\n\n", ID_EX.imm);
	
	//MEM
	printf("EX/MEM.IR %08X ", EX_MEM.IR);  print_instruction(EX_MEM.IR);
	printf("EX/MEM.A %08X\n", EX_MEM.A);
	printf("EX/MEM.B %08X\n", EX_MEM.B);
	printf("EX/MEM.ALUOutput %08X\n\n", EX_MEM.ALUOutput);
	
	//WB
	printf("MEM/WB.IR %08X ", MEM_WB.IR);  print_instruction(MEM_WB.IR);
	printf("MEM/WB.ALUOutput %08X\n", MEM_WB.ALUOutput);
	printf("MEM/WB.LMD %08X\n\n", MEM_WB.LMD);
	
}

unsigned createMask(int start, int end) {
	uint32_t mask = 0xFFFFFFFF;
	mask = (mask>>start)<<start;
	mask = (mask<<(31-end))>>(31-end);
	return mask;
}

uint32_t applyMask(uint32_t mask, uint instruction){
	return instruction & mask;
}

//from lab one
void print_instruction(uint32_t instr)
{
	//From lab 1
	uint32_t instruction = instr; //reading in address from mem
	
	if(instruction == 0)
	{
		printf("No instruction yet\n");
		return;
	}
	
	//creating bit massk
	unsigned opcode_mask = createMask(26,31); //last six bits mask, opcode
	unsigned rs_mask = createMask(21,25);
	unsigned rt_mask = createMask(16,20);
	unsigned imm_mask = createMask(0,15);	
	unsigned base_mask = createMask(21,25);
	unsigned offset_mask = createMask(0,15);
	unsigned target_mask = createMask(0,26);	
	unsigned sa_mask = createMask(6,10);
	unsigned branch_mask = createMask(16,20);	
	unsigned func_mask = createMask(0,5);
	unsigned rd_mask = createMask(11,15);

	//applying masks to get parts of command	
	unsigned opcode = applyMask(opcode_mask, instruction);
	unsigned rs = applyMask(rs_mask, instruction)>>21;
	unsigned rt = applyMask(rt_mask, instruction)>>16;
	unsigned immediate = applyMask(imm_mask, instruction);
	unsigned base = applyMask(base_mask, instruction)>>21;
	unsigned offset = applyMask(offset_mask, instruction);
	unsigned target = applyMask(target_mask, instruction);
	unsigned sa = applyMask(sa_mask, instruction)>>6;
	unsigned branch = applyMask(branch_mask, instruction)>>16;
	unsigned func = applyMask(func_mask, instruction);
	unsigned rd = applyMask(rd_mask, instruction)>>11;
	
	//printf("opcode = %08X      ", opcode);
	
	switch(opcode)
	{
		case 0xFC000000:
		{
			printf("Stall\n");
			break;
		}
		case 0x20000000: //add ADDI
		{
			printf("ADDI ");
			printf("$%X $%X 0x%04X\n", rt, rs, immediate); 
			break;
		}	
		case 0x24000000: //ADDIU
		{
			printf("ADDIU ");
			printf("$%X $%X 0x%04x\n", rt, rs, immediate); 
			break;
		}	
		case 0x30000000: //ANDI
		{
			printf("ANDI ");
			printf("$%X $%X 0x%04X\n", rs, rt, immediate); 
			break;
		}
		case 0x34000000: //ORI
		{
			printf("ORI ");
			printf("$%X $%X, 0x%04x\n", rs, rt, immediate); 
			break;
		}
		case 0x38000000: //XORI
		{
			printf("XORI ");
			printf("$%X $%X 0x%04X\n", rs, rt, immediate);
			break; 
		}
		case 0x28000000: //STLI set on less than immediate
		{
			printf("STLI ");
			printf("$%X $%X 0x%X04\n", rt, rs, immediate); 
			break;
		}
		case 0x8C000000: //Load Word - for now on is load/store instructions mostly
		{
			printf("LW ");
			printf("$%X, %X($%X)\n", rt, offset, base); 
			break;
		}
		case 0x80000000: //Load Byte LB
		{
			printf("LB ");
			printf("$%X, %X($%X)\n", rt, offset, base); 
			break;
		}
		case 0x84000000: //Load halfword
		{
			printf("LH ");
			printf("$%X, %X($%X)\n", rt, offset, base); 
			break;
		}
		case 0x3C000000: //LUI Load Upper Immediate, was 0F
		{
			printf("LUI ");
			printf("$%X 0x%04X\n", rt, immediate); 
			break;
		}
		case 0xAC000000: //SW Store Word
		{
			printf("SW ");
			printf("$%X %X($%X)\n", rt, offset, base); 
			break;
		}
		case 0xA0000000: //SB Store Byte 
		{
			printf("SB ");
			printf("$%X 0x%04X $%X\n", rt, offset, base); 
			break;
		}
		case 0xA4000000: //SH Store Halfword 
		{
			printf("SH ");
			printf("$%X 0x%04X $%X\n", rt, offset, base); 
			break;
		}
		case 0x10000000: //BEQ Branch if equal - start of branching instructions
		{
			printf("BEQ ");
			printf("$%X $%X 0x%04x\n", rs, rt, offset); 
			break;
		}
		case 0x14000000: //BNE Branch on Not Equal
		{
			printf("BNE ");
			printf("$%X $%X 0x%04x\n", rs, rt, offset); 
			break;
		}
		case 0x18000000: //BLEZ Brnach on Less than or equal to zero 
		{
			printf("BLEZ ");
			printf("$%X 0x%04X\n", rs, offset); 
			break;
		}
		case 0x4000000: //special branch cases
		{	
			switch(branch)
			{
				case 0x00: //BLTZ Brnach on Less than zero
				{
					printf("BLTZ ");
					printf("$%X 0x%04X\n", rs, offset); 
					break;
				}
				case 0x01: // BGEZ Branch on greater than zero
				{
					printf("BGEZ ");
					printf("$%X 0x%04X\n", rs, offset); 
					break;
				}
				default:
					printf("ERROR");
			}
			break;	
			
		}
		case 0x1C000000: //BGTZ Branch on Greater than Zero
		{
			printf("BGTZ ");
			printf("$%X 0x%04X\n", rs, offset); 
			break;
		}
		case 0x08000000: //Jump J (bum bum bummmm bum, RIP Eddie VanHalen)
		{
			printf("J ");
			printf("0x%04X\n", target);
			break;
		}
		case 0x0C000000: //JAL Jump and Link
		{
			printf("JAL ");
			printf("0x%04X\n", (instr & 0xF0000000) | (target));
			break;
		}
		case 0x00000000: //special case when first six bits are 000000, function operations
		{
			switch(func)
			{
				case 0x20: //ADD
				{
					printf("ADD ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x21: //ADDU
				{
					printf("ADDU ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x22: //SUB
				{
					printf("SUB ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x23: //SUBU
				{
					printf("SUBU ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x18: //MULT
				{
					printf("MULT ");
					printf("$%X $%X\n", rs, rt); 
					break;
				}
				case 0x19: //MULTU
				{
					printf("MULTU ");
					printf("$%X $%X\n", rs, rt); 
					break;
				}
				case 0x1A: //DIV
				{
					printf("DIV ");
					printf("$%X $%X\n", rs, rt); 
					break;
				}
				case 0x1B: //DIVU
				{
					printf("DIVU ");
					printf("$%X $%X\n", rs, rt); 
					break;	
				}
				case 0x24: //AND
				{
					printf("AND ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x25: //OR
				{
					printf("OR ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x26: //XOR
				{
					printf("XOR ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x27: //NOR
				{
					printf("NOR ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x2A: //SLT Set on less than
				{
					printf("SLT ");
					printf("$%X $%X $%X\n", rd, rs, rt);
					break;
				}
				case 0x00: //SLL Shift Left Logical NEED TO CHECK FORMAT
				{
					printf("SLL ");
					printf("$%X $%X %X\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				}
				case 0x02: //SRL Shift Right Logical CHECK FORMAT
				{
					printf("SRL ");
					printf("$%X $%X %X\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				}
				case 0x03: //SRA Shift Right Arithmetic
				{
					printf("SRA ");
					printf("$%X $%X %X\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				} 
				case 0x10: //MFHI Move from HI
				{
					printf("MFHI ");
					printf("$%X\n", rd); //only the rd register
					break;
				}
				case 0x12: //MFLO Move from LO
				{
					printf("MFLO ");
					printf("$%X\n", rd); //only the rd register
					break;
				}
				case 0x11: //MTHI Move to HI
				{
					printf("MTHI ");
					printf("$%X\n", rs); //only the rs register
					break;
				}
				case 0x13: //MTLO Move to LO
				{
					printf("MTLO ");
					printf("$%X\n", rs); //only the rs register
					break;
				}
				case 0x08: //JR Jump Register
				{
					printf("JR ");
					printf("$%X\n", rs); //only the rs register
					break;
				}
				case 0x09: //JALR Jump and Link Register CHECK FORMAT
				{
					printf("JALR ");
					/*if(rd == 0x1F) //if rd is all one's (or 31) then not given
					{
						printf("$%X\n", rs);
					}*/
					//else //rd is given
						printf("$%X $%X\n", rd, rs);
					break;
				}
				case 0x0C: //SYSCALL
				{
					printf("SYSCALL\n");
					break;
				}
				default:
					printf("ERROR2\n");
			}

			
			break;
		}			
		default:
		{
			printf("Command not found...\n");
			break;
		}
			
	}	
}

	
/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc != 2) {
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
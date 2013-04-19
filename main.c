#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_MAX 100
#define CODE_GEN_MAX_BLOCKS 1000


#define ADRESS 		0
#define SIZE 			1
#define NB_EXEC 		2
#define NB_TRANS 		3
#define LAST_EXEC 	4
#define VALIDE 		5
#define TRACE_ROWS 	6

char filename[16];
unsigned int nb_exec, nb_tran,local_nb_tran, nb_flush;
unsigned int Read_Adress,Read_Size;
unsigned int trace[CODE_GEN_MAX_BLOCKS+1][TRACE_ROWS];
unsigned int trace_size;
unsigned int last_trace_size = 0;
unsigned int tb_hit = 0, global_tb_hit = 0;
unsigned int nb_max_tb;
int sort_row;


/* ------------ Needed for qsort() ------------ */
int cmp ( const void *pa, const void *pb ) {
    const int (*a)[5] = pa;
    const int (*b)[5] = pb;
    if ( (*a)[sort_row] < (*b)[sort_row] ) return +1;
    if ( (*a)[sort_row] > (*b)[sort_row] ) return -1;
    return 0;
}

/* ------------ Dump actual trace[][] content into file ------------ */	
void Dump_Trace(char *filename)
{
	FILE *f_trace;
	int i,Sum_Exec=0,Sum_Trans=0;
	int Deviation,Esperance,nb_pos_dev;
	long Variance;

	f_trace = fopen(filename, "w");
      	if (f_trace == NULL) {
         	printf("Couldn't open trace file for writing.\n");
         	exit(EXIT_FAILURE);
      		}
	for(i=0;i < trace_size;i++) 
   	{
   		Sum_Exec+=trace[i][NB_EXEC];
   		Sum_Trans+=trace[i][NB_TRANS];
   	}
   Esperance = (trace_size>0?Sum_Exec/trace_size:0);
   Variance = 0;
   nb_pos_dev = 0;
	fprintf(f_trace,"  i  |  Adress  | Size | Nb Ex | Nb Tr |  Dev  |  Date  | Valide\n"); 
   for(i=0;i < trace_size;i++) 
   	{
   		Deviation = trace[i][NB_EXEC] - Esperance;
   		if (Deviation > 0) nb_pos_dev++;
   		fprintf(f_trace,"%04u | %08x | %04u | %05u | %05u | %+5d | %5d | %u\n",
   					i,
   					trace[i][ADRESS],
   					trace[i][SIZE],
   					trace[i][NB_EXEC],
   					trace[i][NB_TRANS],
   					Deviation,
   					trace[i][LAST_EXEC],
   					trace[i][VALIDE]);
   		Variance += (Deviation * Deviation);
   	}
   Variance = (trace_size > 0 ? Variance / trace_size : 0);
   
   fprintf(f_trace,"\n-------------------------------------------------------\n");
   
   fprintf(f_trace,"\nSum NbExec = %u\nTotal Exec = %u\nTotal Tran = %u\nEsperance  = %u\nVariance   = %d\nPos values = %u",
   				 Sum_Exec,
   				 nb_exec,
   				 Sum_Trans,
   				 Esperance, 
   				 Variance, 
   				 nb_pos_dev);
   fclose(f_trace);
   printf("\nTrace file recorded to %s\n",filename);
}

/* ------------ Used to erase trace[][] ------------ */
void Trace_Init(int start)
{
	int i,j;
	for (i=0;i<start;i++) trace[i][VALIDE] = 1;  // tb not flushed are valid (until retranslation)
   for(i=start;i<nb_max_tb;i++)			// flush all remaining trace data
    for(j=0;j<TRACE_ROWS;j++) 
      trace[i][j]=0;
}

/* ------------ Lookup for tb in trace[][] using adress of 1st instruction ------------ */
int Lookup_tb(unsigned int Adress, int alloc)
{  
	unsigned int i = 0;
	while((i<trace_size) && (trace[i][ADRESS]!=Adress) && (i<nb_max_tb)) i++;
	if (i>nb_max_tb)
		{
			printf("Trace index Overflow! (i=%d ; loc_nb_tr=%d)\n",i,local_nb_tran);
			snprintf(filename, sizeof(char) * 16, "Trace_Log.dat");
			Dump_Trace(filename); 	
			exit(EXIT_FAILURE);
		}
	if ( (Adress!=trace[i][ADRESS]) && (alloc)) trace_size++;
	return i;
}


/* ------------ Run simulation of cache policy by walking log file ------------ */
void Run(char mode, FILE *f,unsigned int loop_exec, int quota)
{
	FILE *fdat;
	int i;
	int next_line_is_adress=0;
	int nb_lines=0;
	static unsigned int last_tb_exec;
	static int tb_flushed =0;
   char line[LINE_MAX];
   char tmp[LINE_MAX];
   char RLU_RFU = '_';
   float ratio;

	if (tb_flushed) 
	{
		ratio = (float)(nb_exec-last_tb_exec)/nb_max_tb;
		last_tb_exec=nb_exec;
		tb_flushed=0;
		fdat = fopen("exec_ratio.dat", "a");
      if (fdat == NULL) {
         printf("Couldn't open ratio file for writing.\n");
        	exit(EXIT_FAILURE);
      	}
		fprintf(fdat, "%d, %f\n", nb_flush, ratio);
    	fclose(fdat);
    	if (mode == '1') Trace_Init(0);
    	local_nb_tran = 0;
	}

	while ((nb_lines < loop_exec) || !loop_exec)
   {
   	if (fgets(line,LINE_MAX,f)==NULL) {printf("\nEnd of trace file, Exiting... \n");exit(EXIT_SUCCESS);}
   	if (next_line_is_adress) 
   	{
   		next_line_is_adress=0;
   		sscanf(line,"%x",&Read_Adress);
   		printf("\rTranslation @ 0x%010x",Read_Adress);
   	}
   	else 
   	{
   		switch(line[0]) {
      case 'I': next_line_is_adress = 1;														// In asm, next line is @
					 nb_tran++;
					 local_nb_tran++;
					 if ( (local_nb_tran >= (float)nb_max_tb*(1-(float)1/quota)) && ((mode == '2') || (mode == '3')) )
					 {
					  	if (mode == '2') 
					  	 	{sort_row = 4; RLU_RFU = 'R';} 										// LRU
					 	else 
					 	 	{sort_row = 2; RLU_RFU = 'F';} 										// mode == 3 : LFU
					 	last_trace_size = trace_size;
						nb_flush++;
						tb_flushed = 1;						
						qsort(trace, trace_size, TRACE_ROWS * sizeof(unsigned int), cmp);
						snprintf(filename, sizeof(char) * 16, "Trace_L%cU.dat",RLU_RFU);
					   Dump_Trace(filename);
						trace_size = nb_max_tb/quota;								// Quota !??
						Trace_Init(trace_size);
						printf("\nL%cU cache policy flush here!\nCache hit since last flush = %u\n",RLU_RFU,tb_hit);						
						printf("\nStat:\n");					 	
					 	printf("nb_exec                  = %u\n",nb_exec);
					 	printf("nb_trans                 = %u\n",nb_tran);
						printf("nb_flush                 = %u\n",nb_flush);
	   			 	printf("exec since last flush    = %u\n",nb_exec-last_tb_exec);
					 	ratio = (float)(nb_exec-last_tb_exec)/(nb_max_tb-(float)nb_max_tb/quota);
					 	printf("exec/trans ratio         = %f\n",ratio);
					 	ratio = ((float)tb_hit / (nb_exec-last_tb_exec));
					 	printf("cache_hit/nb_exec ratio  = %f\n",ratio);
						fdat = fopen("hit_ratio.dat", "a");
      					if (fdat == NULL) {
         					printf("Couldn't open ratio file for writing.\n");
        						exit(EXIT_FAILURE);
      						}
						fprintf(fdat, "%d, %f\n", nb_flush, ratio);
    					fclose(fdat);
    					global_tb_hit+=tb_hit;
    					printf("Total cache_hit          = %u\n",global_tb_hit);
						tb_hit = 0;
						ratio = ((float)global_tb_hit/nb_exec);
					 	printf("Global cache_hit ratio   = %f\n",ratio);
						return;
					}	 		
					 		
					break;
      case 'O': sscanf(line+11,"%u",&Read_Size);											// Out asm [size=%]
      			 //printf("[size = %5u]",Read_Size);
					 i = Lookup_tb(Read_Adress,1);
					 //if ((trace[i][SIZE]!=0) && (trace[i][SIZE]!=Read_Size))
					 //  {printf("Warning: Attemp to overwrite bloc @ 0x%x (Index = %u ; Size = %u) \n",Read_Adress,i,trace[i][SIZE]);}
      			 trace[i][ADRESS] = Read_Adress;
      			 trace[i][SIZE] = Read_Size;
      			 trace[i][NB_EXEC] = 0;
      			 trace[i][NB_TRANS]++;
      			 trace[i][VALIDE] = 0;
      			break;
      case 'T': sscanf(line+22,"%x",&Read_Adress);											// Trace 
      			 printf("\rExecution   @ 0x%010x",Read_Adress);
      			 i = Lookup_tb(Read_Adress,0);
					 if ((i<last_trace_size) && (trace[i][VALIDE])) tb_hit++;
      			 trace[i][NB_EXEC]++;
      			 trace[i][LAST_EXEC] = nb_exec;
					 nb_exec++;
					break;     
		case 'F': if (mode == '1') 																// Qemu basic cache policy
					{printf(line); 																	// tb_flush >> must return 
   				 snprintf(filename, sizeof(char) * 16, "Trace_%u.dat", nb_flush);
					 Dump_Trace(filename);
					 nb_flush++;
					 tb_flushed=1;					 
					 printf("\nStat:\n");
					 printf("nb_exec  = %u\n",nb_exec);
					 printf("nb_trans = %u\n",nb_tran);
					 printf("nb_flush = %u\n",nb_flush);
	   			 printf("exec since last flush = %u\n",nb_exec-last_tb_exec);
					 ratio=(float)(nb_exec-last_tb_exec)/nb_max_tb;
					 printf("exec ratio = %f\n",ratio);	
					return;
					} 
		//case 'm': printf(line);  										// modifying code
	
      	}
     	}
	nb_lines++;   
   }

}

/* ------------ Analyse extracted trace[][] data ------------ */
void Analyse_Data()
{
	char read_char;
	getchar();
	printf("Choose analyse method:\n");
	printf("1 - Sort Trace by Most Recently Exec\n");
	printf("2 - Sort Trace by Most Exec\n");
	printf("0 - Return\n");
	read_char = getchar();
	switch(read_char) { 
		case '1': sort_row = 4;break;
		case '2': sort_row = 2;break;
		default : if (read_char!='0') {printf("Unknown sort criteria\n");} return;
		}
	qsort(trace, trace_size, TRACE_ROWS * sizeof(unsigned int), cmp);
	printf("Sort terminated of %u data by %s of execution\n",trace_size,(sort_row==2 ? "number":"date"));
	sprintf(filename,"Sort.dat");
	Dump_Trace(filename);
}

/* ------------ Function used to get simulation mode ------------ */
char Simulation_Mode(char Sim_mode)
{
	char read_char;
	printf("Choose the simulation mode:\n");
	printf("1- Qemu Basic Cache Policy\n");
	printf("2- Simulate LRU Cache Policy\n");
	printf("3- Simulate LFU Cache Policy\n");
	printf("0- Return\n");
	do {read_char = getchar();} while((read_char!='0')  && (read_char!='1')  && (read_char!='2')  && (read_char!='3'));
	if (read_char == '0') read_char = Sim_mode;
	 switch(read_char) {
		case '1': printf("Simulation mode : Qemu Basic Policy\n");break;	 	
		case '2': printf("Simulation mode : LRU Cache Policy\n");break;
		case '3': printf("Simulation mode : LFU Cache Policy\n");break;
	 	}
	return read_char;
}

/* ------------------------ Main program function ------------------------ */
int main(int argc, char **argv)
{
	unsigned int loop_exec;
	int quota = 5;
	FILE *f;
	char read_char;
	char Sim_mode = '1';
	if (argc<2) 
	{
		printf("Should pass trace file as argument!\n");
		return -1; 
	}

		
   f=fopen(&argv[1][0],"r");
   if(f == NULL)
    {
        printf("specified file not found! Exiting...\n");
        return -1;
    }
    
   Trace_Init(0);
   nb_max_tb = CODE_GEN_MAX_BLOCKS;
   
   remove("exec_ratio.dat");
   remove("hit_ratio.dat");

	system( "clear" );
	printf("\n *** Qemu Translation Cache trace tool *** TIMA LAB - March 2013 ***\n\n");    

	while(1) {	

	printf("\nChoose a command:\n");
	printf("1 - Run Cache policy simulation\n");
	printf("2 - Modify number of steps into loop\n");
	printf("3 - Modify Quota of simulated cache policy\n");
	printf("4 - Modify Trace Size\n");
	printf("5 - Change Simulated cache policy\n");	
	printf("6 - Dump Trace Data\n");
	printf("7 - Analyse Trace Data\n");
	printf("8 - plot hit ratio\n");	
	printf("0 - Exit\n");  

	do {read_char = getchar();} while((read_char !='0') && 
												 (read_char !='1') && 
												 (read_char !='2') && 
												 (read_char !='3') && 
												 (read_char !='4') && 
												 (read_char !='5') && 
												 (read_char !='6') && 
												 (read_char !='7') && 
												 (read_char !='8'));
   switch(read_char) {
   	case '0': return;   	// exit program
   	case '1': Run(Sim_mode, f,loop_exec,quota); 
   					break;
   	case '2': printf("Actual loop_exec=%d, Enter new value(0 for continuous loop) : ",loop_exec);scanf("%u",&loop_exec);
   					break;
   	case '3': do {printf("Actual Quota=1/%d, Enter new value : 1/",quota);scanf("%d",&quota);} while(quota < 1);
   					break;
   	case '4': do {printf("Actual Trace size = %d, Enter new value : ",nb_max_tb);scanf("%d",&nb_max_tb);} while((nb_max_tb>1000) || (nb_max_tb<10));
   					break;			
   	case '5': Sim_mode = Simulation_Mode(Sim_mode);
   					break;
   	case '6': if (trace_size) {sprintf(filename,"DumpTrace.dat"); Dump_Trace(filename);} 
   					else {printf("No trace data available!\n");} break;
   	case '7': if (trace_size) {Analyse_Data();}
   					else {printf("No trace data available!\n");} break;
   	case '8': system("gnuplot script_hit.plt");
   					break;	
   	default:  break;
   	}
	}
   fclose(f);
   exit(EXIT_SUCCESS);
}
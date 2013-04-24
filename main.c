#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_MAX 100
#define CODE_GEN_MAX_BLOCKS 1024


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
unsigned int trace[CODE_GEN_MAX_BLOCKS][TRACE_ROWS];
unsigned int trace_size;
unsigned int last_trace_size = 0;
unsigned int tb_hit = 0, global_tb_hit = 0;
unsigned int nb_max_tb;
int sort_row;
char Sim_mode = '1';


/* ------------ Needed for qsort() ------------ */
int cmp ( const void *pa, const void *pb ) {
    const int (*a)[5] = pa;
    const int (*b)[5] = pb;
    if (( (*a)[sort_row] < (*b)[sort_row] ) && ((Sim_mode=='2') || (Sim_mode=='3'))) return +1;
    if (( (*a)[sort_row] < (*b)[sort_row] ) && ((Sim_mode=='4') || (Sim_mode=='5'))) return -1;
    if (( (*a)[sort_row] > (*b)[sort_row] ) && ((Sim_mode=='2') || (Sim_mode=='3'))) return -1;
    if (( (*a)[sort_row] > (*b)[sort_row] ) && ((Sim_mode=='4') || (Sim_mode=='5'))) return +1;
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
int Lookup_tb(unsigned int Adress)
{  
	unsigned int i = 0;
	while((i<trace_size) && (trace[i][ADRESS]!=Adress) && (i<nb_max_tb)) i++;
	snprintf(filename, sizeof(char) * 16, "Trace_Log.dat");
	if (i>nb_max_tb)
		{
			printf("Trace index Overflow! (i=%d ; loc_nb_tr=%d)\n",i,local_nb_tran);
			Dump_Trace(filename);
			exit(EXIT_FAILURE);
		}
	if (Adress!=trace[i][ADRESS]) {
			trace_size++;
			trace[i][ADRESS] = Read_Adress;
			}
	return i;
}


/* ------------ Run simulation of cache policy by walking log file ------------ */
void Run(FILE *f,unsigned int loop_exec, int quota)
{
	FILE *fdat;
	int i;
	int next_line_is_adress = 0;
	static unsigned int last_tb_exec;
	static int tb_flushed = 0;
   char line[LINE_MAX];
   char tmp[LINE_MAX];
   char R_F = '_';
	char L_M = '_';
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
		fprintf(fdat, "%d, %f\n", nb_exec, ratio);
    	fclose(fdat);
    	if (Sim_mode == '1') Trace_Init(0);
    	local_nb_tran = 0;
	}

	while (1)
   {
   	if ((nb_exec > loop_exec) && loop_exec) 
   		{printf("\nEnd of %d executions, please modify loop_exec to continue...\n",nb_exec);return;}
   	if (fgets(line,LINE_MAX,f)==NULL) 
   		{printf("\nEnd of trace file, Exiting... \n");exit(EXIT_SUCCESS);}
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
					 if ( (local_nb_tran >= ((32-quota) * nb_max_tb)/32) && (Sim_mode != '1') )
					 {
					 	switch(Sim_mode) 
					 		{
					 			case '2':  sort_row = 4; L_M = 'L';	R_F = 'R'; break;
					 			case '3':  sort_row = 2; L_M = 'L';	R_F = 'F'; break;
					 			case '4':  sort_row = 4; L_M = 'M';	R_F = 'R'; break;
					 			case '5':  sort_row = 2; L_M = 'M';	R_F = 'F'; break;
					 			default: printf("error in cache policy\n");exit(EXIT_FAILURE);
							}
					 	last_trace_size = trace_size;
						nb_flush++;
						tb_flushed = 1;						
						qsort(trace, trace_size, TRACE_ROWS * sizeof(unsigned int), cmp);
						snprintf(filename, sizeof(char) * 16, "Trace_%c%cU.dat",L_M ,R_F);
					   Dump_Trace(filename);
						trace_size = nb_max_tb/quota;
						Trace_Init(trace_size);
						printf("\n%c%cU (Quota=%d/32)cache policy flush here!\nCache hit since last flush = %u\n"
								 ,L_M, R_F,quota,tb_hit);						
						printf("\nStat:\n");					 	
					 	printf("nb_exec                  = %u\n",nb_exec);
					 	printf("nb_trans                 = %u\n",nb_tran);
						printf("nb_flush                 = %u\n",nb_flush);
	   			 	printf("exec since last flush    = %u\n",nb_exec-last_tb_exec);
					 	ratio = (float)(nb_exec-last_tb_exec)/(nb_max_tb-nb_max_tb*32/quota);
					 	printf("exec/trans ratio         = %f\n",ratio);
					 	ratio = ((float)tb_hit / (nb_exec-last_tb_exec));
					 	printf("cache_hit/nb_exec ratio  = %f\n",ratio);
						fdat = fopen("hit_ratio.dat", "a");
      					if (fdat == NULL) {
         					printf("Couldn't open ratio file for writing.\n");
        						exit(EXIT_FAILURE);
      						}
						fprintf(fdat, "%d, %f\n", nb_exec, ratio);
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
					 i = Lookup_tb(Read_Adress);
      			 
      			 trace[i][SIZE] = Read_Size;
      			 trace[i][NB_EXEC] = 0;
      			 trace[i][NB_TRANS]++;
      			 trace[i][VALIDE] = 0;
      			break;
      case 'T': sscanf(line+22,"%x",&Read_Adress);											// Trace 
      			 printf("\rExecution   @ 0x%010x",Read_Adress);
      			 i = Lookup_tb(Read_Adress);
					 if ((i<last_trace_size) && (trace[i][VALIDE])) tb_hit++;
      			 trace[i][NB_EXEC]++;
      			 trace[i][LAST_EXEC] = nb_exec;
					 nb_exec++;
					break;     
		case 'F': if (Sim_mode == '1') 																// Qemu basic cache policy
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
      	}
     	}
     	
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
	printf("4- Simulate MRU Cache Policy\n");
	printf("5- Simulate MFU Cache Policy\n");
	printf("0- Return\n");
	do {read_char = getchar();} while((read_char!='1')  && 
												 (read_char!='2')  && 
												 (read_char!='3')  && 
												 (read_char!='4')  &&
												 (read_char!='5')  &&
												 (read_char!='0'));
	if (read_char == '0') read_char = Sim_mode;
	 switch(read_char) {
		case '1': printf("Simulation mode : Qemu Basic Policy\n");break;	 	
		case '2': printf("Simulation mode : LRU Cache Policy\n");break;
		case '3': printf("Simulation mode : LFU Cache Policy\n");break;
		case '4': printf("Simulation mode : MRU Cache Policy\n");break;
		case '5': printf("Simulation mode : MFU Cache Policy\n");break;
	 	}
	return read_char;
}

/* ------------------------ Main program function ------------------------ */
int main(int argc, char **argv)
{
	unsigned int loop_exec = 500000;
	int quota = 8;
	FILE *f;
	char read_char;

	if (argc<2) 
	{
		printf("Should pass trace file as argument!\n");
		exit(EXIT_FAILURE);
	}


   f=fopen(&argv[1][0],"r");
   if(f == NULL)
    {
        printf("specified file not found! Exiting...\n");
        exit(EXIT_FAILURE);
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
	printf("2 - Modify number of executions into loop\n");
	printf("3 - Modify Quota of simulated cache policy\n");
	printf("4 - Modify Trace Size\n");
	printf("5 - Change Simulated cache policy\n");	
	printf("6 - Dump Trace Data\n");
	printf("7 - Analyse Trace Data\n");
	printf("8 - Plot hit ratio\n");
	printf("9 - Restart simulation\n");
	printf("0 - Exit\n");  

	do {read_char = getchar();} while((read_char !='1') && 
												 (read_char !='2') && 
												 (read_char !='3') && 
												 (read_char !='4') && 
												 (read_char !='5') && 
												 (read_char !='6') && 
												 (read_char !='7') &&
												 (read_char !='8') && 
												 (read_char !='9') &&
												 (read_char !='0'));
   switch(read_char) {
   	case '0': return;   	// exit program
   	case '1': Run(f,loop_exec,quota); 
   					break;
   	case '2': printf("Actual number of translations is %d, Enter new value (0 for continuous loop) : ",loop_exec);
   				 scanf("%u",&loop_exec);
   				 break;
   	case '3': do {printf("Actual Quota=%d/32, Enter new value : ",quota);scanf("%d/32",&quota);} while((quota != 1) && 
   																																		(quota != 2) &&
   																																		(quota != 4) &&
   																																		(quota != 8) &&
   																																		(quota != 16));
   				 break;
   	case '4': do {printf("Actual Trace size = %d, Enter new value : ",nb_max_tb);scanf("%d",&nb_max_tb);} 
   						while((nb_max_tb>1024) || (nb_max_tb<16));
   				 break;			
   	case '5': Sim_mode = Simulation_Mode(Sim_mode);
   				 break;
   	case '6': if (trace_size) {sprintf(filename,"DumpTrace.dat"); Dump_Trace(filename);} 
   				 else {printf("No trace data available!\n");} break;
   	case '7': if (trace_size) {Analyse_Data();}
   					else {printf("No trace data available!\n");} break;
   	case '8': if (!system("gnuplot script_hit.plt")) printf("\nPlot recorded to hit_out.png"); 
   				 else printf("\nPlot error, verify that source file (hit_ratio.dat) is available\n");
   				 break;
   	case '9': remove("exec_ratio.dat");
   			    remove("hit_ratio.dat");
   			    system("clear");
   			    nb_exec =0;
   			    nb_tran = 0;
   			    local_nb_tran = 0;
   			    nb_flush = 0;
   			    global_tb_hit = 0;
   			    trace_size = 0;
   			    last_trace_size = 0;
   			    rewind(f); 
   			    break;
   	default:  break;
   	}
	}
   fclose(f);
   exit(EXIT_SUCCESS);
}
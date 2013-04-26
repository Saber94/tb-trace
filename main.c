#include "main.h"

char filename[F_LENGTH];
unsigned int nb_exec;
unsigned int local_nb_exec;
unsigned int nb_tran;
unsigned int local_nb_tran;
unsigned int nb_flush;
unsigned int Read_Adress,Read_Size;
unsigned int trace[2][CODE_GEN_MAX_BLOCKS][TRACE_ROWS];
unsigned int cold_size;
unsigned int cold_size_max;
unsigned int tb_hit = 0;
unsigned int global_tb_hit = 0;
unsigned int last_nb_exec;
unsigned int last_nb_tran;
int tb_flushed = 0;
int sort_row;
char Sim_mode = BASIC_MODE;


/* ------------ Needed for qsort() ------------ */
int cmp ( const void *pa, const void *pb ) {
    const int (*a)[TRACE_ROWS] = pa;
    const int (*b)[TRACE_ROWS] = pb;
    if ( (*a)[sort_row] < (*b)[sort_row] ) return +1;
    if ( (*a)[sort_row] > (*b)[sort_row] ) return -1;
    return 0;
}

/* ------------ Dump actual trace[COLD][][] content into file ------------ */	
void Dump_Trace(char *filename)
{
	FILE *f_trace;
	int i,Sum_Exec=0,Sum_Trans=0;
	int Deviation,Esperance,nb_pos_dev;
	unsigned int Variance;

	f_trace = fopen(filename, "w");
      	if (f_trace == NULL) {
         	printf("Couldn't open trace file for writing.\n");
         	exit(EXIT_FAILURE);
      		}
	for(i=0;i < cold_size;i++) 
   	{
   		Sum_Exec += trace[COLD][i][NB_EXEC];
   		Sum_Trans += trace[COLD][i][NB_TRANS];
   	}
   Esperance = (cold_size>0?Sum_Exec/cold_size:0);
   Variance = 0;
   nb_pos_dev = 0;
	fprintf(f_trace,"  i  |  Adress  | Size | Nb Ex | Nb Tr |   Dev   |  Date  | Valide\n"); 
   for(i=0;i < cold_size;i++)
   	{
   		Deviation = trace[COLD][i][NB_EXEC] - Esperance;
   		if (Deviation > 0) nb_pos_dev++;
   		fprintf(f_trace,"%04u | %08x | %04u | %05u | %05u | %+7d | %6d |   %u\n",
   					i,
   					trace[COLD][i][ADRESS],
   					trace[COLD][i][SIZE],
   					trace[COLD][i][NB_EXEC],
   					trace[COLD][i][NB_TRANS],
   					Deviation,
   					trace[COLD][i][LAST_EXEC],
   					trace[COLD][i][VALIDE]);
   		Variance += (Deviation * Deviation);
   	}
   Variance = (cold_size > 0 ? Variance / cold_size : 0);
   
   fprintf(f_trace,"\n----------------------------------------------------------------\n");
   
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

/* ------------ Used to erase trace[][][] ------------ */
void Trace_flush(int start,int spot)
{
	int i,j;
	for (i=0;i<start;i++) 
	 {
	 	trace[spot][i][VALIDE] = 1;  // tb not flushed are valid (until retranslation)
	 }
   for(i=start;i<cold_size_max;i++)			// flush all remaining trace data
    {
    	for(j=0;j<TRACE_ROWS;j++) 
       {trace[spot][i][j]=0;}
    }
}

/* ------------ Lookup for tb in trace[][] using adress of 1st instruction ------------ */
int Lookup_tb(unsigned int Adress)
{
	unsigned int i = 0;
	while((i<cold_size) && (trace[COLD][i][ADRESS]!=Adress))
	 { i++;
		if (i>cold_size_max-1)
		{
		return -1;
		}
	 }
	if ((trace[COLD][i][ADRESS] == 0) && (cold_size < cold_size_max-1))
	 {
		cold_size++;
		trace[COLD][i][ADRESS] = Read_Adress;
		if ((i >= cold_size_max ) || (cold_size >= cold_size_max))
		{printf("\n i = %d ; cold_size = %d",i,cold_size);
		 exit(EXIT_FAILURE);
		 }
	 }
	return i;
}


void cold_flush(int quota)
{
	FILE *fdat;
   float ratio;
   char R_F;
	switch(Sim_mode)
	 {
		case LRU_MODE: sort_row = 4;	R_F = 'R'; break;
		case LFU_MODE: sort_row = 2;	R_F = 'F'; break;
		default :		R_F = '_';
	 }
   nb_flush++;
	tb_flushed = 1;
	local_nb_tran = nb_tran - last_nb_tran;
	local_nb_exec = nb_exec - last_nb_exec;
	snprintf(filename, sizeof(char) * F_LENGTH, "trace/Trace_%u.dat", nb_flush);
	Dump_Trace(filename);
	qsort(trace, cold_size, TRACE_ROWS * sizeof(unsigned int), cmp);
	cold_size = (cold_size_max * quota)/NB_SEG;
	Trace_flush(cold_size,0);
	printf("\nL%cU (Quota=%d/32)cache policy flush here!", R_F,quota);
	printf("\n ------------- Local Stat -------------\n");
	printf("nb exec                  = %u\n",local_nb_exec);
	printf("nb trans                 = %u\n",local_nb_tran);
	printf("nb cache hit             = %u\n",tb_hit);
	ratio = ((float)tb_hit / local_nb_exec);
	printf("cache_hit ratio          = %f\n",ratio);
	ratio = (float)(local_nb_exec)/(local_nb_tran);
	printf("exec ratio               = %f\n",ratio);
	printf("\n ------------- Global Stat -------------\n");
	printf("nb flush                 = %u\n",nb_flush);
	printf("nb exec                  = %u\n",nb_exec);
	printf("nb trans                 = %u\n",nb_tran);
	global_tb_hit += tb_hit;
	printf("nb cache hit             = %u\n",global_tb_hit);
	ratio = ((float)global_tb_hit/nb_exec);
	printf("cache_hit ratio          = %f\n",ratio);
	fdat = fopen("hit_ratio.dat", "a");
	if (fdat == NULL) 
	{
	printf("Couldn't open hit_ratio file for writing.\n");
   exit(EXIT_FAILURE);
   }
   ratio = ((float)tb_hit / local_nb_exec);
	fprintf(fdat, "%d, %f\n", nb_exec, ratio);
   fclose(fdat);
	last_nb_exec = nb_exec;
	last_nb_tran = nb_tran;
	tb_hit = 0;
}

/* ------------ Run simulation of cache policy by walking log file ------------ */
void Run(FILE *f,unsigned int max_exec, int quota, int threshold)
{
	FILE *fdat;
	int i;
	int next_line_is_adress = 0;
   char line[LINE_MAX];

   float ratio;

	if (tb_flushed)
	{
		ratio = (float)local_nb_exec/cold_size_max;
		tb_flushed = 0;
		fdat = fopen("exec_ratio.dat", "a");
      if (fdat == NULL) {
         printf("Couldn't open ratio file for writing.\n");
        	exit(EXIT_FAILURE);
      	}
		fprintf(fdat, "%d, %f\n", nb_exec, ratio);
    	fclose(fdat);
    	if (Sim_mode == '1') Trace_flush(0,0);
	}

	while (1)
   {
   	if (fgets(line,LINE_MAX,f)==NULL)
   		{system("gnuplot script_hit.plt");
   		printf("\nEnd of trace file, Exiting... \n");
   		exit(EXIT_SUCCESS);
   		}
   	if ((nb_exec > max_exec) && max_exec) 
   		{printf("\nEnd of %d executions, please modify max_exec to continue...\n",nb_exec);
   		return;
   		}
   	if (next_line_is_adress) 
   		{next_line_is_adress=0;
   		sscanf(line,"%x",&Read_Adress);
   		printf("\rTranslation @ 0x%010x",Read_Adress);
   		}
   	else 
   		{switch(line[0]) {
      case 'I': next_line_is_adress = 1;														// In asm, next line is @
					 nb_tran++;
					 if ( (((nb_tran - last_nb_tran) % threshold) == 0) && (Sim_mode != '1') )
					 {
						cold_flush(quota);
						return;
					 }
					break;
      case 'O': sscanf(line+11,"%u",&Read_Size);											// Out asm [size=%]
					 i = Lookup_tb(Read_Adress);
					 if (i == -1)
					 	{
					 	 printf("\ncache overflow..\n");
						 exit(EXIT_FAILURE);
						}
      			 trace[COLD][i][SIZE] = Read_Size;
      			 trace[COLD][i][NB_EXEC] = 0;
      			 trace[COLD][i][NB_TRANS]++;
      			 trace[COLD][i][VALIDE] = 0;
      			break;
      case 'T': sscanf(line+22,"%x",&Read_Adress);											// Trace 
      			 printf("\rExecution   @ 0x%010x",Read_Adress);
      			 i = Lookup_tb(Read_Adress);
      			 if (i == -1)
      			  {
      			   cold_flush(quota);
      			   system("sleep 3");
      			   i = Lookup_tb(Read_Adress);
      			   if (trace[COLD][i][ADRESS] == 0)
      			    {nb_tran++;
      			    trace[COLD][i][ADRESS] = Read_Adress;
      			    trace[COLD][i][NB_EXEC] = 0;
      			 	 trace[COLD][i][NB_TRANS]++;
      			 	 trace[COLD][i][VALIDE] = 0;
      			 	 }
      			  }
					 if (trace[COLD][i][VALIDE])
					 	{
					 		tb_hit++;
					 	}
      			 trace[COLD][i][NB_EXEC]++;
      			 trace[COLD][i][LAST_EXEC] = nb_exec;
					 nb_exec++;
					break;     
		case 'F': if (Sim_mode == '1') 															// Qemu basic cache policy
					{printf(line); 																	// tb_flush >> must return 
					 cold_flush(0);
					return;
					} 
      		}
     		}
   }
}

/* ------------ Function used to get simulation mode ------------ */
char Simulation_Mode(char Sim_mode)
{
	char read_char;
	printf("Choose the simulation mode:\n");
	printf("1- Qemu Basic Cache Policy\n");
	printf("2- Simulate LRU Cache Policy\n");
	printf("3- Simulate LFU Cache Policy\n");
	printf("4- Simulate MQ Cache Policy\n");
	printf("0- Return\n");
	do {read_char = getchar();} while((read_char <'0') || (read_char >'4'));
	switch(read_char) {
	 	case '0': read_char = Sim_mode;
		case BASIC_MODE: printf("Simulation mode : Qemu Basic Policy\n");break;	 	
		case LRU_MODE	: printf("Simulation mode : LRU Cache Policy\n");break;
		case LFU_MODE	: printf("Simulation mode : LFU Cache Policy\n");break;
		case MQ_MODE	: printf("Simulation mode : MQ  Cache Policy\n");break;
	 	}
	return read_char;
}

/* ------------------------ Main program function ------------------------ */
int main(int argc, char **argv)
{
	unsigned int max_exec = 500000;
	int quota = 8;
	int threshold;
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

   Trace_flush(0,0);
   Trace_flush(0,1);   
   cold_size_max = CODE_GEN_MAX_BLOCKS;
   threshold = cold_size_max - (cold_size_max*quota)/NB_SEG;

   remove("exec_ratio.dat");
   remove("hit_ratio.dat");
   system("rm ./trace/*.dat");

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
	printf("7 - Plot hit ratio\n");
	printf("0 - Exit\n");

	do {read_char = getchar();} while((read_char <'0') || (read_char >'7'));
   switch(read_char) {
   	case '0': return;   	// exit program
   	case '1': Run(f,max_exec,quota,threshold);
   					break;
   	case '2': printf("Actual number of translations is %d, Enter new value (0 for continuous loop) : ",max_exec);
   				 scanf("%u",&max_exec);
   				 break;
   	case '3': do {printf("Actual Quota=%d/32, Enter new value : ",quota);
   					  scanf("%d/32",&quota);
   					  threshold = cold_size_max - (cold_size_max*quota)/NB_SEG; 
   					  } while((quota < 1) || (quota > 16));
   				 break;
   	case '4': do {printf("Actual Trace size = %d, Enter new value : ",cold_size_max);
   					  scanf("%d",&cold_size_max);
   					  threshold = cold_size_max - (cold_size_max*quota)/NB_SEG;
   					  } while((cold_size_max>CODE_GEN_MAX_BLOCKS) || (cold_size_max%NB_SEG));
   				 break;
   	case '5': Sim_mode = Simulation_Mode(Sim_mode);
   				 break;
   	case '6': if (cold_size) 
   					{sprintf(filename,"DumpTrace.dat"); Dump_Trace(filename);} 
   				 else 
   				 	{printf("No trace data available!\n");} 
   				 break;
   	case '7': if (!system("gnuplot script_hit.plt")) printf("\nPlot recorded to hit_out.png"); 
   				 else printf("\nPlot error, verify that source file (hit_ratio.dat) is available\n");
   				 break;
   	default:  break;
   	}
	}
   fclose(f);
   exit(EXIT_SUCCESS);
}
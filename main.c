#include "main.h"


unsigned int nb_exec = 0;
unsigned int nb_tran = 0;
unsigned int nb_cold_flush = 0;
unsigned int nb_hot_flush = 0;
unsigned int trace[2][CODE_GEN_MAX_BLOCKS][TRACE_ROWS];
//unsigned int nb_inv_tb;
//unsigned int local_nb_inv_tb;
unsigned int cold_size = 0;
unsigned int hot_size = 0;
unsigned int tb_hit = 0;
unsigned int global_tb_hit = 0;

int sort_row;

char filename[F_LENGTH];

/* ------------ Needed for qsort() ------------ */
int cmp ( const void *pa, const void *pb ) {
    const int (*a)[TRACE_ROWS-1] = pa;
    const int (*b)[TRACE_ROWS-1] = pb;
    if ( (*a)[sort_row] < (*b)[sort_row] ) return +1;
    if ( (*a)[sort_row] > (*b)[sort_row] ) return -1;
    return 0;
}

/* ------------ Dump actual trace[][][] content into file ------------ */	
void Dump_Cache(int spot, char *filename,unsigned int size)
{
	FILE *f_trace;
	int i,Sum_Exec=0,Sum_Trans=0;
	int Deviation,Esperance,nb_pos_dev;
	unsigned int Variance;

	f_trace = fopen(filename, "w");
   if (f_trace == NULL)
    {
		printf("Couldn't open trace file for writing.\n");
		exit(EXIT_FAILURE);
	 }
	for(i=0;i < size;i++)
   	{
   	Sum_Exec += trace[spot][i][NB_EXEC];
   	Sum_Trans += trace[spot][i][NB_TRANS];
   	}
   Esperance = (size>0?Sum_Exec/size:0);
   Variance = 0;
   nb_pos_dev = 0;
	fprintf(f_trace,"  i  |  Adress  | Nb Ex | Nb Tr |   Dev   |  Date  | Valide\n"); 
   for(i=0;i < size;i++)
   	{
   		Deviation = trace[spot][i][NB_EXEC] - Esperance;
   		if (Deviation > 0) nb_pos_dev++;
   		fprintf(f_trace,"%04u | %08x | %05u | %05u | %+7d | %6d |   %u\n",
   					i,
   					trace[spot][i][ADRESS],
   					trace[spot][i][NB_EXEC],
   					trace[spot][i][NB_TRANS],
   					Deviation,
   					trace[spot][i][LAST_EXEC],
   					trace[spot][i][VALIDE]);
   		Variance += (Deviation * Deviation);
   	}
   Variance = (size > 0 ? Variance / size : 0);
   
   fprintf(f_trace,"\n----------------------------------------------------------\n");
   
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
void Cache_flush(int spot,int start,unsigned int size_max)
{
	int i,j;
	for (i=0;i<start;i++) 
	 {
	 	trace[spot][i][VALIDE] = 1;  				// tb not flushed are valid (until retranslation)
	 }
   for(i=start;i<size_max;i++)				// flush all remaining trace data
    {
    	for(j=0;j<TRACE_ROWS;j++) 
       {trace[spot][i][j]=0;}
    }
}

/* ------------ Lookup for tb in trace[][][] using adress of 1st instruction ------------ */
int Lookup_tb(int spot, unsigned int Adress, unsigned int size, unsigned int size_max, int* key)
{
	unsigned int i = 0;

	while((i<size) && (trace[spot][i][ADRESS]!=Adress))
	 { i++;
		if (i>=size_max)
			{return 2;}								// Cache Full
	 }
	*key = i;
	if (trace[spot][i][ADRESS] == Adress)
		{return 1;}	 								// found
	return 0;										// not found
}


void Display_stat()
{
	FILE *fdat;
   float ratio;
   unsigned int local_nb_exec;
	unsigned int local_nb_tran;
	static unsigned int last_nb_exec;
	static unsigned int last_nb_tran;

   nb_cold_flush++;
	local_nb_tran = nb_tran - last_nb_tran;
	local_nb_exec = nb_exec - last_nb_exec;

	printf("\n ------------- Local Stat -------------\n");
	printf("nb exec                  = %u\n",local_nb_exec);
	printf("nb trans                 = %u\n",local_nb_tran);
	printf("nb cache hit             = %u\n",tb_hit);
	ratio = (float)tb_hit / local_nb_exec;
	printf("cache_hit ratio          = %f\n",ratio);
	ratio = (float)local_nb_exec/local_nb_tran;
	printf("exec ratio               = %f\n",ratio);
	printf("\n ------------- Global Stat -------------\n");
	printf("nb coldspot flush        = %u\n",nb_cold_flush);
	printf("nb hotspot flush         = %u\n",nb_hot_flush);	
	printf("nb exec                  = %u\n",nb_exec);
	printf("nb trans                 = %u\n",nb_tran);
	global_tb_hit += tb_hit;
	printf("nb cache hit             = %u\n",global_tb_hit);
	ratio = (float)global_tb_hit/nb_exec;
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

/* ------------ Exec instruction --------*/
void Exec(int spot,unsigned int i, unsigned int Read_Adress)
{
	trace[spot][i][ADRESS] = Read_Adress;
  	trace[spot][i][NB_EXEC]++;
  	trace[spot][i][LAST_EXEC] = nb_exec;
  	trace[spot][i][VALIDE] = 0;
}

/* ------------ Run simulation of cache policy by walking log file ------------ */
void Run(FILE *f,unsigned int max_exec, int quota, int threshold,int Sim_mode, unsigned int size_max)
{
	int i,j;
	int found_cold;
	int found_hot;
	unsigned int Read_Adress;
	unsigned int tmp;
	static unsigned int adresses[CACHE_MAX_BLOCKS];
	static unsigned int adr_size;
   char line[LINE_MAX];
   char smode[4];
   static int cold_cache_flush;
   
   switch(Sim_mode)
	 	{
	 	case MQ_MODE:	sort_row = NB_EXEC;
	 						snprintf(smode,4,"MQ");
							break;
		case LRU_MODE: sort_row = LAST_EXEC;	
							snprintf(smode,4,"LRU");
							break;
		case LFU_MODE: sort_row = NB_EXEC;	
							snprintf(smode,4,"LFU");
							break;
	 	}

	while (1) {
   if (fgets(line,LINE_MAX,f)==NULL)
   	{
   		system("gnuplot script_hit.plt");
   		printf("\nEnd of trace file, Exiting... \n");
   		exit(EXIT_SUCCESS);
   	}

  	if ((nb_exec > max_exec) && max_exec)
  		{
  			printf("\nEnd of %d executions, please modify max_exec to continue...\n",nb_exec);
  			return;
  		}
  
  	if (line[0]=='T')
  		{
  			if (cold_cache_flush)
  				{
  					Cache_flush(COLD,cold_size,size_max);
  					cold_cache_flush = 0;
      	   	if (Lookup_tb(COLD,Read_Adress,cold_size,size_max,&i) == 0) 	// Cannot fail, but may be not found! (cache miss)
      	   	 {
      	   		trace[COLD][i][NB_TRANS]++;
      	   		nb_tran++;
      	   	 }
					Exec(COLD, i, Read_Adress);  					
					nb_exec++;
  				}
  			sscanf(line+22,"%x",&Read_Adress);
  			printf("\rt=%6d / size=%d %d/",nb_tran,cold_size,hot_size);
     		printf("Execution   @ 0x%010x",Read_Adress);
     		if ((Sim_mode == MQ_MODE) && (Lookup_tb(HOT,Read_Adress,hot_size,size_max,&i) == 1))
     	  		{
     	   		tb_hit++;
     	   		Exec(HOT, i, Read_Adress);
  		  		}
     		else 
     			{
      			found_cold = Lookup_tb(COLD,Read_Adress,cold_size,size_max,&i);
      			switch(found_cold) {
      	 case 2:															// if cold cache is full, flush it
				qsort(trace, cold_size, TRACE_ROWS * sizeof(unsigned int), cmp);
				printf("\n%s",smode);
				printf(" (Quota=%d/32)cache policy flush here!",quota);
				snprintf(filename, sizeof(char) * F_LENGTH, "trace/Trace_%u.dat", nb_cold_flush);
				Dump_Cache(COLD,filename,cold_size);
				if (Sim_mode != MQ_MODE)
					{cold_size = (size_max * quota)/NB_SEG;}
				else
					{
						if (adr_size>(size_max - (size_max * quota)/NB_SEG)) {adr_size = 0;}
					 	for(i=0;i<(cold_size * quota)/NB_SEG;i++)
					 		{
					 			adresses[adr_size] = trace[COLD][i][ADRESS];adr_size++;
/*						 		 j=0;
					 		 while((j<adr_size) && (adresses[j]!=tmp)) {j++;}
					 		 if (adresses[j] != tmp)
					 			 {
					 			   printf(" not found 0x%x : 0x%x (%d:%d)\n",tmp,adresses[j],adr_size,j);
					 			   adresses[j] = tmp;
					 			   adr_size++;
					 			 } else {printf(" found 0x%x : 0x%x (%d:%d)\n",tmp,adresses[j],adr_size,j);}*/
					 		}
					 	cold_size = 0;
					}
      	 	Display_stat();
      	 	cold_cache_flush = 1;
      	 	return;
      	   break;
			 case 1:
			 	if ((trace[COLD][i][VALIDE]) && (Sim_mode != MQ_MODE))			// if TB is translated before last flush
			 		{tb_hit++;}
			 	Exec(COLD, i, Read_Adress);	
			 	break;
			 case 0:																				// if new allocation needed
      	  	
				if (Sim_mode != MQ_MODE)
					{
      	  			Exec(COLD, i, Read_Adress);
      	  			trace[COLD][i][NB_TRANS]++;
  			 			cold_size++;
			 		}
			 	else
			 		{																				// decide where to translate the new TB
			 			j=0;
						while((j<=adr_size) && (adresses[j]!=Read_Adress)) {j++;} 
					 	if (adresses[j] == Read_Adress)
					 		{
					 		 if (hot_size < size_max) 
					 		 {
					 		 	Exec(HOT, hot_size, Read_Adress);
					 		 	trace[HOT][hot_size][NB_TRANS]++;
      						hot_size++;
      					 }
      					 else 
      					 {
      					 	Cache_flush(HOT,0,size_max);
      					 	hot_size=0;
      					 	nb_hot_flush++;
      					 }
					 		}
					 	else 
					 		{
					 			Exec(COLD, i, Read_Adress);
					 			trace[COLD][i][NB_TRANS]++;
  			 				 	cold_size++;
			 				}
			 		}
  			 	nb_tran++;
			   break;
									 		}
			 }
			nb_exec++;
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
	 	case '0': read_char = Sim_mode;break;
		case BASIC_MODE: printf("Simulation mode : Qemu Basic Policy\n");break;	 	
		case LRU_MODE	: printf("Simulation mode : LRU Cache Policy\n");break;
		case LFU_MODE	: printf("Simulation mode : LFU Cache Policy\n");break;
		case MQ_MODE	: printf("Simulation mode : MQ Cache Policy\n");break;
	 	}
	return read_char;
}

/* ------------------------ Main program function ------------------------ */
int main(int argc, char **argv)
{
	unsigned int size_max;
	unsigned int max_exec = 0;
	int quota = 4;
	int threshold;
	FILE *f;
	char read_char;
	char Sim_mode = LRU_MODE;

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

   size_max = CODE_GEN_MAX_BLOCKS;
   Cache_flush(COLD,0,size_max);
   Cache_flush(COLD,0,size_max);   

   threshold = size_max - (size_max*quota)/NB_SEG;

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
	printf("6 - Dump Hotspot cache\n");
	printf("7 - Dump Coldspot cache\n");	
	printf("8 - Plot hit ratio\n");
	printf("0 - Exit\n");

	do {read_char = getchar();} while((read_char <'0') || (read_char >'8'));
   switch(read_char) {
   	case '0': return;   	// exit program
   	case '1': Run(f,max_exec,quota,threshold,Sim_mode,size_max);
   					break;
   	case '2': printf("Actual number of translations is %d, Enter new value (0 for continuous loop) : ",max_exec);
   				 scanf("%u",&max_exec);
   				 break;
   	case '3': do {printf("Actual Quota=%d/32, Enter new value : ",quota);
   					  scanf("%d/32",&quota);
   					  threshold = size_max - (size_max*quota)/NB_SEG;
   					  } while((quota < 1) || (quota > 16));
   				 break;
   	case '4': do {printf("Actual Trace size = %d, Enter new value : ",size_max);
   					  scanf("%d",&size_max);
   					  threshold = size_max - (size_max*quota)/NB_SEG;
   					  } while((size_max>CODE_GEN_MAX_BLOCKS) || (size_max%NB_SEG));
   				 break;
   	case '5': Sim_mode = Simulation_Mode(Sim_mode);
   				 break;
   	case '6': sprintf(filename,"HotspotTrace.dat"); Dump_Cache(HOT,filename,hot_size);
   				 break;
   	case '7': sprintf(filename,"ColdspotTrace.dat"); Dump_Cache(COLD,filename,cold_size);
   				 break;
   	case '8': if (!system("gnuplot script_hit.plt")) printf("\nPlot recorded to hit_out.png"); 
   				 else printf("\nPlot error, verify that source file (hit_ratio.dat) is available\n");
   				 break;
   	default:  break;
   	}
	}
   fclose(f);
   exit(EXIT_SUCCESS);
}
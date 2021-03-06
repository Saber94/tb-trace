#include "main.h"


unsigned int nb_exec = 0;
unsigned int nb_tran = 0;
static unsigned int last_nb_exec;
static unsigned int last_nb_tran;
unsigned int nb_cold_flush = 0;
unsigned int nb_hot_flush = 0;
unsigned int trace[2][CODE_GEN_MAX_BLOCKS][TRACE_ROWS];
unsigned int adresses[CACHE_MAX_BLOCKS];
unsigned int cold_size = 0;
unsigned int hot_size = 0;
unsigned int adr_size;
unsigned int hotspot_hit = 0;
unsigned int total_hit = 0;
unsigned int global_tb_hit = 0;
unsigned int inv_count = 0;
unsigned int hot_size_max;
unsigned int size_max = CODE_GEN_MAX_BLOCKS/2;
unsigned int size_total = CODE_GEN_MAX_BLOCKS/2;
unsigned int Fth = 16;

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
   /* --- Trace file generation --- */ 
	fprintf(f_trace,"  i  |  Adress  | Nb Ex | Nb Tr |   Dev   |  Date   | Valide\n"); 
   for(i=0;i < size;i++)
   	{
   		Deviation = trace[spot][i][NB_EXEC] - Esperance;
   		if (Deviation > 0) nb_pos_dev++;
   		fprintf(f_trace,"%04u | %08x | %05u | %05u | %+7d | %7d |   %u\n",
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
   
   /* --- Stats at the end of trace file --- */
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
void Cache_flush(int spot,int start, unsigned int size)
{
	int i,j;
	for (i=0;i<start;i++) 
	 {
	 	trace[spot][i][VALIDE] = 1;  				// tb not flushed are valid (until retranslation)
	 }
   for(i=start;i<size;i++)				// flush all remaining trace data
    {
    	for(j=0;j<TRACE_ROWS;j++) 
       {trace[spot][i][j]=0;}
    }
}

/* ------------ Lookup for tb in trace[][][] using address of 1st instruction ------------ */
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


void Display_stat() /* --- Called when CSA flush occurs --- */
{
	FILE *fdat;
   float ratio;
   unsigned int local_nb_exec;
	unsigned int local_nb_tran;

	if (nb_tran != last_nb_tran)
	{
   nb_cold_flush++;
	local_nb_tran = nb_tran - last_nb_tran;
	local_nb_exec = nb_exec - last_nb_exec;

	printf("\n ------------- Local Stat -------------\n");
	printf("nb exec                  = %u\n",local_nb_exec);
	printf("nb trans                 = %u\n",local_nb_tran);
	printf("hotspot cache hit        = %u\n",hotspot_hit);
	ratio = (float)hotspot_hit / local_nb_exec;
	printf("hotspot hit ratio        = %f\n",ratio);
	printf("total cache hit          = %u\n",total_hit);

	printf("\n ------------- Global Stat -------------\n");
	printf("nb coldspot flush        = %u\n",nb_cold_flush);
	printf("nb hotspot flush         = %u\n",nb_hot_flush);	
	printf("nb exec                  = %u\n",nb_exec);
	printf("nb trans                 = %u\n",nb_tran);
	ratio = (float)nb_exec/nb_tran;
	printf("reuse ratio              = %f\n",ratio);
	printf("nb tb invalidation       = %u\n",inv_count);
	global_tb_hit += hotspot_hit;
	printf("total cache hit          = %u\n",global_tb_hit);
	ratio = (float)global_tb_hit/nb_exec;
	printf("cache_hit ratio          = %f\n",ratio);
	fdat = fopen("hit_ratio.dat", "a");
	if (fdat == NULL)
	 {
		printf("Couldn't open hit_ratio file for writing.\n");
   	exit(EXIT_FAILURE);
    }
   ratio = ((float)hotspot_hit / local_nb_exec);
	fprintf(fdat, "%d, %f\n", nb_exec, ratio);
   fclose(fdat);
	last_nb_exec = nb_exec;
	last_nb_tran = nb_tran;
	total_hit = 0;
	hotspot_hit = 0;
	}
}

/* ------------------- Execute tb ------------------- */
inline void Execute(int spot,unsigned int i)
{
  	trace[spot][i][NB_EXEC]++;
  	trace[spot][i][LAST_EXEC] = nb_exec;
}

/* -------------------  tb ------------------- */
inline void Translate(unsigned int Read_Adress, int i, unsigned int *quota)
{
	int j = 0;
	while((j<hot_size_max/2) && (adresses[j]!=Read_Adress)) {j++;} 
	if (adresses[j] == Read_Adress)			/* --- decide where to translate the new TB --- */
		{
			 if (hot_size >= hot_size_max) 	/* --- HSA overflow -> flush HSA --- */
				 {
      		 	snprintf(filename, sizeof(char) * F_LENGTH, "trace/Trace_H%u.dat", nb_hot_flush);
      		 	Dump_Cache(HOT,filename,hot_size);
      		 	Cache_flush(HOT, 0, hot_size);
      		 	hot_size = 0;
      		 	(*quota)= 2;
      		 	hot_size_max = ((size_total * (*quota))/NB_SEG);
					size_max = size_total - hot_size_max;	
      		 	
      		 	nb_hot_flush++;
      		 }
			trace[HOT][hot_size][ADRESS] = Read_Adress;
			trace[HOT][hot_size][NB_TRANS]++;
			trace[HOT][hot_size][VALIDE] = 1;
			Execute(HOT, hot_size);
      	hot_size++;
		}
	else
		{
			trace[COLD][i][ADRESS] = Read_Adress;
			trace[COLD][i][VALIDE] = 1;
		 	Execute(COLD, i);
		 	trace[COLD][i][NB_TRANS]++;
  			cold_size++;
		}
}


/* ------------ Run simulation of cache policy by walking log file ------------ */
void Run(FILE *f,unsigned int max_exec, unsigned int *quota, unsigned int max_quota, int Sim_mode)
{
	unsigned int i,j;
	unsigned int found_cold;
	unsigned int found_hot;
	unsigned int nb_adr;
	unsigned int Read_Adress;
	unsigned int tmp;

   char line[LINE_MAX];
   char smode[7];
   static int cold_cache_flush;

   switch(Sim_mode)
	 	{
	 	case BASIC_MODE:	snprintf(smode,7,"FLUSH");
							break;	 		
	 	case MQ_MODE:	snprintf(smode,7,"A-2Q");
							break;
		case LRU_MODE: sort_row = LAST_EXEC;
							snprintf(smode,7,"A-LRU");
							break;
		case LFU_MODE: sort_row = NB_EXEC;
							snprintf(smode,7,"A-LFU");
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
  			Display_stat();
  			printf("\nEnd of %d executions, please modify max_exec to continue...\n",nb_exec);
  			return;
  		}
  if (cold_cache_flush)
  		{
  			Cache_flush(COLD, cold_size, size_max);
  			cold_cache_flush = 0;
      	if (Lookup_tb(COLD,Read_Adress,cold_size,size_max,&i) == 0) 	/* Cannot fail, but may be not found! (cache miss) */
      	 {
      	 	trace[COLD][i][ADRESS] = Read_Adress;
      		trace[COLD][i][NB_TRANS]++;
      		trace[COLD][i][VALIDE] = 1;
      		nb_tran++;
      	 }
			Execute(COLD, i);
			nb_exec++;
  		}
  
  	if (line[0]=='T')
  		{
  			sscanf(line+22,"%x",&Read_Adress);
  			printf("\rTranslations=%6d / Sizes=(Coldspot: %4d/%d) (Hotspot: %4d/%d) / ", nb_tran, cold_size, size_max, hot_size, hot_size_max);
     		printf("Execution   @ 0x%010x",Read_Adress);
     		if ((Sim_mode == MQ_MODE) && (Lookup_tb(HOT,Read_Adress,hot_size,hot_size_max,&i) == 1) && (trace[HOT][i][VALIDE]))
     	  		{
     	  			hotspot_hit++;
     	   		total_hit++;
     	   		Execute(HOT, i);
  		  		}
     		else 
     			{
      			found_cold = Lookup_tb(COLD,Read_Adress,cold_size,size_max,&i);
      			switch(found_cold) {
      	 case 2:															// if cold cache is full, flush it
				snprintf(filename, sizeof(char) * F_LENGTH, "trace/Trace_C%u.dat", nb_cold_flush);
				if (Sim_mode != MQ_MODE)
					{
						if ((Sim_mode == LRU_MODE) || (Sim_mode == LFU_MODE))
							{	
								qsort(trace, cold_size, TRACE_ROWS * sizeof(unsigned int), cmp);
								Dump_Cache(COLD,filename,cold_size);
								cold_size = (size_max * (*quota))/NB_SEG;				// cold size is reduced to hotspot items, the remaining items are evicted..
								printf("\n%s (Quota=%d/32)", smode, (*quota));
							}
							else 
							{ 	
								Dump_Cache(COLD,filename,cold_size);
								printf("\n%s",smode);
								cold_size = 0;
							}
								printf(" cache policy flush here!");
					}
				else
					{
						Dump_Cache(COLD,filename,cold_size);
   					nb_adr = 0;

					 	for(i=0;i<cold_size;i++)
					 		{ if(trace[COLD][i][NB_EXEC] > Fth) 
					 		 {
					 		 	nb_adr++;
					 			adresses[adr_size] = trace[COLD][i][ADRESS];
					 			adr_size++;
								if (adr_size >=hot_size_max/2)
									{
										adr_size = 0;
									}
									
							 }
					 		}
					 	printf("\n%s",smode);
						printf(" (nb added @ = %d )( Quota = %u)cache policy flush here!",nb_adr,(*quota));
					 	cold_size = 0;
					 	if( ((*quota)<max_quota)  && ((((float)hot_size_max/hot_size)<2 ) || (hot_size == 0)))
					 		{
					 			(*quota)++;
					 			hot_size_max = ((size_total * (*quota))/NB_SEG);
								size_max = size_total - hot_size_max;	
					 		}
					}
      	 	Display_stat();
      	 	cold_cache_flush = 1;
      	 	return;
      	   break;
			 case 1:
			 	if ((trace[COLD][i][VALIDE]) && (Sim_mode != MQ_MODE) && (i < (size_max * (*quota))/NB_SEG) && (nb_cold_flush>0))		
			 																						// if TB is translated before last flush
			 		{
			 			hotspot_hit++;
			 			total_hit++;
			 		}
			 	else
			 		{
			 			total_hit++;
			 		}
			 	Execute(COLD, i);	
			 	break;
			 case 0:																				// if new allocation needed
				if (Sim_mode != MQ_MODE)
					{
      	  			trace[COLD][i][ADRESS] = Read_Adress;
      	  			trace[COLD][i][NB_TRANS]++;
      	  			trace[COLD][i][VALIDE] = 1;
      	  			Execute(COLD, i);
  			 			cold_size++;
			 		}
			 	else
			 		{
			 			Translate(Read_Adress, i, quota);
			 		}
  			 	nb_tran++;
			   break;
									 		}
			 }
			nb_exec++;
     	}
	else if(line[0]=='I')
		{
			sscanf(line+6,"%x",&Read_Adress);
			if (Lookup_tb(HOT,Read_Adress,cold_size,hot_size_max,&i) == 1)
				{
					trace[HOT][i][VALIDE] = 0;
					inv_count++;
				}
			else if(Lookup_tb(COLD,Read_Adress,cold_size,size_max,&i) == 1)
				{
					trace[COLD][i][VALIDE] = 0;

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
	printf("2- Simulate A-LRU Cache Policy\n");
	printf("3- Simulate A-LFU Cache Policy\n");
	printf("4- Simulate A-2Q Cache Policy\n");
	printf("0- Return\n");
	do {read_char = getchar();} while((read_char <'0') || (read_char >'4'));
	switch(read_char) {
	 	case '0': read_char = Sim_mode;break;
		case BASIC_MODE: printf("Simulation mode : Qemu Basic Policy\n");break;
		case LRU_MODE	: printf("Simulation mode : A-LRU Cache Policy\n");break;
		case LFU_MODE	: printf("Simulation mode : A-LFU Cache Policy\n");break;
		case MQ_MODE	: printf("Simulation mode : A-2Q Cache Policy\n");break;
	 	}
	return read_char;
}

/* ------------ Display the main menu ------------ */
void Display_menu()	
{
	printf("1 - Run Cache policy simulation\n");
	printf("2 - Modify number of executions into loop\n");
	printf("3 - Modify Quota of simulated cache policy\n");
	printf("4 - Modify Trace Size\n");
	printf("5 - Modify Fth of A-2Q Algo\n");	
	printf("6 - Change Simulated Algo\n");
	printf("7 - Dump Coldspot cache\n");	
	printf("8 - Plot hit ratio\n");
	printf("9 - Display this menu\n");
	printf("0 - Exit\n");
}

/* ------------------------ Main program function ------------------------ */
int main(int argc, char **argv)
{

	unsigned int max_exec = 1000000;								/* initial execution limit */
	unsigned int quota = 0;
	unsigned int max_quota = 16;									/* initial quota = 16/32 = 1/2 */
	hot_size_max = ((CODE_GEN_MAX_BLOCKS * quota)/NB_SEG);
	FILE *f;
	char read_char;
	char Sim_mode = MQ_MODE;

	if (argc<2) 
	{
		printf("Should pass trace file as argument! Exiting...\n");
		exit(EXIT_FAILURE);
	}


   f=fopen(&argv[1][0],"r");
   if(f == NULL)
    {
        printf("specified file not found! Exiting...\n");
        exit(EXIT_FAILURE);
    }

	/* Init: Flush the entire cache content */
   Cache_flush(HOT,0,size_max);
   Cache_flush(COLD,0,size_max);   
	
	/* remove trace data of last simulations */
   remove("exec_ratio.dat");
   remove("hit_ratio.dat");
   system("rm ./trace/*.dat");

	/* clear screen */
	system( "clear" );
	printf("\n *** Qemu Translation Cache trace tool *** TIMA LAB - March 2013 ***\n\n");    

	Display_menu();	
	
	while(1) {

	printf("\nChoose a command:\n");


	do {read_char = getchar();} while((read_char <'0') || (read_char >'9'));
   switch(read_char) {
   	case '0': return;   	// leave loop & exit program
   	case '1': Run(f,max_exec,&quota,max_quota,Sim_mode);
   					break;
   	case '2': printf("Actual number of translations is %d, Enter new value (0 for continuous loop) : ",max_exec);
   				 scanf("%u",&max_exec);
   				 break;
   	case '3': do { 
   						printf("Actual Quota=%d/32, Enter new value : ",max_quota);
   					   scanf("%d/32",&max_quota);
   					 }
   					 	while((max_quota < 1) || (max_quota > 32));
   					hot_size_max = ((size_total * quota)/NB_SEG);
						if (Sim_mode == MQ_MODE)
						 {size_max = size_total - hot_size_max;}
						else 
						 {size_max = size_total;}
   				 break;
   	case '4': do {
   						printf("Actual Trace size = %d, Enter new value : ",size_total);
   					   scanf("%d",&size_total);
   					  	hot_size_max = ((size_total * quota)/NB_SEG);
							if (Sim_mode == MQ_MODE) 
								{size_max = size_total - hot_size_max;} 
							else 
								{size_max = size_total;}
   					 }
   					 	while((size_max>CODE_GEN_MAX_BLOCKS));
   				 break;
   	case '5': printf("Actual Fth value is %d, Enter new value : ",Fth);
   				 scanf("%u",&Fth);
   				 break;
   	case '6': Sim_mode = Simulation_Mode(Sim_mode);
   				 break;
   	case '7': sprintf(filename,"ColdspotTrace.dat"); Dump_Cache(COLD,filename,cold_size);
   				 break;
   	case '8': if (!system("gnuplot script_hit.plt")) printf("\nPlot recorded to hit_out.png"); 
   				 else printf("\nPlot error, verify that source file (hit_ratio.dat) is available\n");
   				 break;
   	default:  Display_menu();break;
   	}
	}
   fclose(f);
   exit(EXIT_SUCCESS);
}

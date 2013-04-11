#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_MAX 100
#define code_gen_max_blocks 1000

unsigned int nb_exec, nb_tran, nb_flush;
unsigned int Read_Adress,Read_Size;

unsigned int trace[code_gen_max_blocks][5];
unsigned int trace_size = 0;

int sort_row;

char Simulation_Mode()
{
	char read_char;
	printf("Choose the simulation mode:\n");
	printf("1- Qemu Basic Cache Policy\n");
	printf("2- Simulate LRU Cache Policy\n");
	printf("3- Simulate LFU Cache Policy\n");
	printf("0- Return\n");
	while((read_char!='0')  && (read_char!='1')  && (read_char!='2')  && (read_char!='3')) 
		read_char = getchar();
	return read_char;
}

void Trace_Init()
{
  int i,j;
  for(i=0;i<code_gen_max_blocks;i++)
    for(j=0;j<4;j++) 
      trace[i][j]=0;
}
	
void Log_Trace()
{
	FILE *f_trace;
	int i,Sum_Exec=0,Sum_Trans=0;
	int Deviation,Esperance,nb_pos_dev;
	long Variance;
	char filename[16];
   snprintf(filename, sizeof(char) * 16, "trace_%u.dat", nb_flush);
	f_trace = fopen(filename, "w");
      	if (f_trace == NULL) {
         	printf("Couldn't open trace file for writing.\n");
         	exit(EXIT_FAILURE);
      		}
	for(i=0;i < trace_size;i++) 
   	{
   		Sum_Exec+=trace[i][2];
   		Sum_Trans+=trace[i][3];
   	}   
   Esperance = (trace_size>0?Sum_Exec/trace_size:0);
   Variance = 0;
   nb_pos_dev = 0;
	fprintf(f_trace,"  i  |  Adress  | Size | Nb Ex | Nb Tr |  Dev  | Last Exec\n"); 
   for(i=0;i < trace_size;i++) 
   	{
   		Deviation = trace[i][2] - Esperance;
   		if (Deviation > 0) nb_pos_dev++;
   		fprintf(f_trace,"%04u | %08x | %04u | %05u | %05u | %+5d | %5d\n",i,trace[i][0],trace[i][1],trace[i][2],trace[i][3],Deviation,trace[i][4]);
   		Variance += (Deviation * Deviation);
   	}
   Variance = (trace_size > 0 ? Variance / trace_size : 0);
   
   printf("\n------------------------------------------------------\n");
   
   fprintf(f_trace,"\nSum NbExec = %u\nTotal Exec = %u\nTotal Tran = %u\nEsperance  = %u\nVariance   = %d\nPos values = %u",
   				 Sum_Exec,
   				 nb_exec,
   				 Sum_Trans,
   				 Esperance, 
   				 Variance, 
   				 nb_pos_dev);
   fclose(f_trace);
}
	
int Lookup_tb(unsigned int Adress)
{
	int i=0;
	while((i<trace_size) && (trace[i][0]!=Adress)) i++;
	if (Adress!=trace[i][0]) trace_size++;
	return i;
}

void Run(char mode, FILE *f,unsigned int loop_exec)
{
	FILE *fdat;
	int i;
	int next_line_is_adress=0;
	int nb_lines=0;
	static unsigned int last_tb_exec;
	static int tb_flushed =0;
   char line[LINE_MAX];
   char tmp[LINE_MAX];
   float ratio;

	if (tb_flushed) 
		{
			ratio = (float)(nb_exec-last_tb_exec)/code_gen_max_blocks;
			last_tb_exec=nb_exec;
			tb_flushed=0;
			fdat = fopen("ratio.dat", "a");
      	if (fdat == NULL) {
         	printf("Couldn't open ratio file for writing.\n");
         	exit(EXIT_FAILURE);
      		}
			fprintf(fdat, "%d, %f\n", nb_flush, ratio);
    		fclose(fdat);
    		trace_size = 0;
    		Trace_Init();
		}
	if (mode = '0') {
	while ((fgets(line,LINE_MAX,f)!= NULL) && ((nb_lines < loop_exec) || !loop_exec))
   {
   	if (next_line_is_adress) 
   	{
   		next_line_is_adress=0;
   		sscanf(line,"%x",&Read_Adress);
   		printf("Translation @ 0x%x ",Read_Adress);
   		}
   	else 
   	{
   		switch(line[0]) {
      case 'I': next_line_is_adress = 1;						// In asm, next line is @
					 nb_tran++;
					break;
      case 'O': sscanf(line+11,"%u",&Read_Size);			// Out asm [size=%]
      			 printf("[size = %3u]\n",Read_Size);
					 i = Lookup_tb(Read_Adress);
					 if ((trace[i][1]!=0) && (trace[i][1]!=Read_Size))
					   {
						printf("Warning: Attemp to overwrite bloc @ 0x%x (Index = %u ; Size = %u) \n",Read_Adress,i,trace[i][1]);
						//Log_Trace();
						//printf("Press any key to continue...\n");
						//getchar();    			 		
      			 	}
      			 trace[i][0] = Read_Adress;
      			 trace[i][1] = Read_Size;
      			 trace[i][2] = 0;
      			 trace[i][3]++;
      			break;
      case 'T': sscanf(line+22,"%x",&Read_Adress);				// Trace 
      			 printf("Execution   @ 0x%x\n",Read_Adress); 
      			 i = Lookup_tb(Read_Adress);
      			 trace[i][2]++;
      			 trace[i][4] = nb_exec;
					 nb_exec++;
					break;     
		case 'F': printf(line); 										// tb_flush >> must return 
					 Log_Trace();
					 nb_flush++;
					 tb_flushed=1;					 
					 printf("\nStat:\n");
					 printf("nb_exec  = %u\n",nb_exec);
					 printf("nb_trans = %u\n",nb_tran);
					 printf("nb_flush = %u\n",nb_flush);
	   			 printf("exec since last flush = %u\n",nb_exec-last_tb_exec);
					 ratio=(float)(nb_exec-last_tb_exec)/code_gen_max_blocks;
					 printf("exec ratio = %f\n",ratio);	
					return;
		case 'm': printf(line);  										// modifying code
			   	 //printf("Press any key to continue...\n");
					 //getchar();		
      	}
     	}

	nb_lines++;   
   }
   }
   else printf("Simulation not implemented yet!\n");
}


int cmp ( const void *pa, const void *pb ) {
    const int (*a)[5] = pa;
    const int (*b)[5] = pb;
    if ( (*a)[sort_row] < (*b)[sort_row] ) return +1;
    if ( (*a)[sort_row] > (*b)[sort_row] ) return -1;
    return 0;
}


void Analyse_Daty()
{
	char read_char;

	printf("Choose analyse method:\n");
	printf("1 - Sort Trace by Most Exec\n");
	printf("2 - Sort Trace by Most Recently Exec\n");
	printf("0 - Return\n");
	read_char = getchar();
	switch(read_char) { 
		case '1': sort_row = 2;break;
		case '2': sort_row = 4;break;
		default : printf("Unknown sort criteria\n");return;
		}
	qsort (trace, trace_size, 5*sizeof(unsigned int), cmp);
	Log_Trace(); // should spec sort file
	printf("Sort terminated of %u data\n",trace_size);
}

int main(int argc, char **argv)
{
	unsigned int loop_exec;
	FILE *f;
	char read_char;
	int Sim_mode = 0;

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
    
   Trace_Init(); 
   if (remove("ratio.dat") == -1)
   	perror("Error in deleting a file"); 

	/* Clear screen at startup */
	if (system( "clear" )) system( "cls" );
	printf("\n *** Qemu Translation Cache trace tool *** TIMA LAB - March 2013 ***\n\n");    

	printf("\nChoose the command to execute:\n");
	printf("1 - Run Cache policy simulation\n");
	printf("2 - Modify number of instructions into loop\n");
	printf("3 - Print tb trace table\n");
	printf("4 - Change Simulated cache policy\n");
	printf("5 - Analyse Trace Data\n");
	printf("0 - Exit\n");  

	while(1) {

	read_char = getchar();
   switch(read_char) {
   	case '0': return;   	
   	case '1': Run(Sim_mode, f,loop_exec); break;
   	case '2': printf("Actual loop_exec=%d, Enter new value: ",loop_exec);scanf("%u",&loop_exec);break;
   	case '3': Log_Trace();break;
   	case '4': Sim_mode = Simulation_Mode();break;
   	case '5': Analyse_Daty();break;
   	default:  break;
   	}
	}
   fclose(f);
   exit(EXIT_SUCCESS);
}
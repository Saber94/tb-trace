#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_MAX 100
#define code_gen_max_blocks 2000

int loop_exec,trace_size = 0;
unsigned long nb_exec=0, nb_tran=0, nb_flush=0,last_tb_exec;
unsigned int Read_Adress,Read_Size;
float ratio;
unsigned int trace[1000][4];

void Display_Stat()
{
	int i;
	extern float ratio;
   printf("\nStat:\n");
	printf("nb_exec  = %u\n",nb_exec);
	printf("nb_trans = %u\n",nb_tran);
	printf("nb_flush = %u\n",nb_flush);
	printf("exec since last flush = %u\n",nb_exec-last_tb_exec);
	ratio=(float)(nb_exec-last_tb_exec)/code_gen_max_blocks;
	printf("exec ratio = %f\n",ratio);	
}
	
void Log_Trace(int size)
{
	FILE *f_trace;
	int i,Sum_Exec=0,Sum_Trans=0;
	f_trace = fopen("trace.dat", "w");
      	if (f_trace == NULL) {
         	printf("I couldn't open results.dat for writing.\n");
         	exit(EXIT_FAILURE);
      		}
	fprintf(f_trace,"  i |   Adress | Size | Nb Exec | Nb Tran \n"); 
   for(i=0;i < (size<1000?size:1000);i++) 
   	{
   		fprintf(f_trace,"%3u | %8x | %4u |  %5u  | %5u \n",i,trace[i][0],trace[i][1],trace[i][2],trace[i][3]);
   		Sum_Exec+=trace[i][2];
   		Sum_Trans+=trace[i][3];
   	}
   fprintf(f_trace,"Total Exec = %u \nTotal Tran = %u\n", Sum_Exec,Sum_Trans);
   fclose(f_trace);
}
	
int Lookup_tb(unsigned int Adress)
{
	int i=0;
	while((i<trace_size) && (trace[i][0]!=Adress)) i++;
	if (Adress!=trace[i][0]) trace_size++;
	return i;
}

void display_menu()
{
  	printf("\nChoose the command to execute:\n");
	printf("1 - Read file\n");
	printf("2 - Display Stat\n");
	printf("3 - Modify number of instructions into loop\n");
	printf("4 - Print tb trace table\n");
	printf("0 - Exit\n");
	}

void Read_Qemu_Log(FILE *f)
{
	FILE *fdat;
	int i;
	int next_line_is_adress=0;
	int nb_lines=0;

	static int tb_flushed =0;
   char line[LINE_MAX];
   char tmp[LINE_MAX];

	if (tb_flushed) 
		{
			last_tb_exec=nb_exec;
			tb_flushed=0;
			fdat = fopen("results.dat", "a");
      	if (fdat == NULL) {
         	printf("I couldn't open results.dat for writing.\n");
         	exit(EXIT_FAILURE);
      		}
			fprintf(fdat, "%d, %f\n", nb_flush, ratio);
    		fclose(fdat);
		}

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
      case 'I': next_line_is_adress = 1;
					 nb_tran++;
					break;
      case 'O': sscanf(line+11,"%u",&Read_Size);	
      			 printf("[size = %3u]\n",Read_Size);
					 i = Lookup_tb(Read_Adress);
					 if ((trace[i][1]!=0) && (trace[i][1]!=Read_Size))
					   {
						printf("Attemp to overwrite bloc @ 0x%x (index=%u) \n",Read_Adress,i);
						Log_Trace(nb_tran);
						printf("Abnormal program termination!\n");
						exit(EXIT_FAILURE);      			 		
      			 	}
      			 trace[i][0] = Read_Adress;
      			 trace[i][1] = Read_Size;
      			 trace[i][3]++;
      			break;
      case 'T': sscanf(line+22,"%x",&Read_Adress);
      			 printf("Execution   @ 0x%x\n",Read_Adress); 
      			 i = Lookup_tb(Read_Adress);
      			 trace[i][2]++;
					 nb_exec++;
					break;     
		case 'F': printf(line); 			// tb_flush >> must return 
					 nb_flush++;
					 tb_flushed=1;
					 Log_Trace(nb_tran);
					 Display_Stat();
					return;
      	}
     	}

	nb_lines++;   
   }
}

int main(int argc, char **argv)
{

	FILE *f;
	char read_char;

	if (argc<2) 
	{
		printf("Should pass trace file as argument!\n");
		return -1; 
	}

		
   f=fopen(&argv[1][0],"r");
   if(f == NULL)
    {
        printf("src file not found! Exiting...\n");
        return -1;
    }
    
   if (remove("results.dat") == -1)
   perror("Error in deleting a file"); 

	/* Clear screen at startup */
	if (system( "clear" )) system( "cls" );
	printf("\n *** Qemu Translation Cache trace tool *** TIMA LAB - March 2013 ***\n\n");    
   
   display_menu();

while(1) {
   read_char=getchar();
   switch(read_char) {
   	case '1': Read_Qemu_Log(f); break;
   	case '2': Display_Stat();break;
   	case '3': printf("Actual loop_exec=%d, Enter new value: ",loop_exec);scanf("%u",&loop_exec);break;
   	case '4': Log_Trace(nb_tran);break;
   	case '0': exit(EXIT_SUCCESS);
   	default:  break;
   	}
}
   
   fclose(f);


}
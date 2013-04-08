#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_MAX 100
#define code_gen_max_blocks 2000

int loop_exec=1000;
unsigned long nb_exec=0, nb_tran=0, nb_flush=0,last_tb_exec;
float ratio;

int trace[1000][3];


void display_stat()
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
	printf("   Adress | Size | Nb Exec\n");
	for(i=0;i<1000;i++) printf(" %8x | %4u | %u \n",trace[i][0],trace[i][1],trace[i][2]);
	}
	
void Inc_tb_exec(int Adress)
{
	int i=0;
	while(trace[i++][0]!=Adress && i<1000); 
	if (i>999) printf("block @ %x not found ! \n",Adress);
	else 	trace[i-1][2]++;
}		

void display_menu()
{
  	printf("\nChoose the command to execute:\n");
	printf("1 - Read file\n");
	printf("2 - Display Stat\n");
	printf("3 - Modify number of instructions into loop\n");
	printf("0 - Exit\n");
	}

void read_log(FILE *f)
{
	FILE *fdat;
	int i;
	int next_line_is_adress=0;
	int nb_lines=0;
	int Read_Adress,Read_Size;
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
         	exit(0);
      		}
			fprintf(fdat, "%d, %f\n", nb_flush, ratio);
    		fclose(fdat);
		}

	while((fgets(line,LINE_MAX,f)!= NULL) && ((nb_lines < loop_exec) || !loop_exec))
   {
   	if (next_line_is_adress) 
   	{
   		next_line_is_adress=0;
   		sscanf(line,"%x",&Read_Adress);
   		printf("Translation @ 0x%x ",Read_Adress);
   		}
   	else 
   	{	switch(line[0]) {
      case 'I': next_line_is_adress=1;
			nb_tran++;	break;
      case 'O': sscanf(line+11,"%u",&Read_Size);	
      			 printf("[size = %3u]\n",Read_Size);
					 i = nb_tran % 1000;
      			 trace[i][0] = Read_Adress;
      			 trace[i][1] = Read_Size;
      			 trace[i][2] = 0;
      	break;
      case 'T': sscanf(line+22,"%x",&Read_Adress);	printf("Execution   @ 0x%x\n",Read_Adress); Inc_tb_exec(Read_Adress);
			nb_exec++;	break;     
		case 'F': printf(line); 			// tb_flush >> must return 
			nb_flush++;	tb_flushed=1; display_stat(); return;
      }}

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
   	case '1': read_log(f); break;
   	case '2': display_stat();break;
   	case '3': printf("Actual loop_exec=%d, Enter new value: ",loop_exec);scanf("%u",&loop_exec);break;
   	
   	case '0': exit(EXIT_SUCCESS);
   	default:  break;
   	}
}
   
   fclose(f);


}
   

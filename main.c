#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_MAX 100
#define code_gen_max_blocks 1000

int loop_exec=1000;
unsigned long nb_exec=0, nb_tran=0, nb_flush=0,last_tb_exec;
float ratio;


void display_stat()
{
	extern float ratio;
   fprintf(stdout,"\nStat:\n");
	fprintf(stdout,"nb_exec  = %u\n",nb_exec);
	fprintf(stdout,"nb_trans = %u\n",nb_tran);
	fprintf(stdout,"nb_flush = %u\n",nb_flush);
	fprintf(stdout,"exec since last flush = %u\n",nb_exec-last_tb_exec);
	ratio=(float)(nb_exec-last_tb_exec)/code_gen_max_blocks;
	fprintf(stdout,"exec ratio = %f\n",ratio);
	}

void display_menu()
{
  	fprintf(stdout,"\nChoose the command to execute:\n");
	fprintf(stdout,"1 - Read file\n");
	fprintf(stdout,"2 - Display Stat\n");
	fprintf(stdout,"3 - Modify number of instructions into loop\n");
	fprintf(stdout,"0 - Exit\n");
	}

void read_log(FILE *f)
{
	FILE *fdat;
	int next_line_is_adress=0;
	static int tb_flushed =0;
   char line[LINE_MAX];
   char tmp[LINE_MAX];
	int nb_lines=0;
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

	while((fgets(line,LINE_MAX,f)!= NULL) && (nb_lines < loop_exec))
   {
   	if (next_line_is_adress) 
   	{													// should convert adress to int
   		next_line_is_adress=0;
   		strncpy(tmp,line,18);
   		tmp[19]='\0';	   		
   		fprintf(stdout,"new translation at ");
   		puts(tmp);
   		}
   	else 
   	{	switch(line[0]) {
      case 'I': next_line_is_adress=1;
			nb_tran++;	break;
      case 'O': fprintf(stdout,line+5);		// should convert size to int
      	break;
      case 'T': fprintf(stdout,line+21); 		// Trace case..
			nb_exec++;	break;     
		case 'F': fprintf(stdout,line); 			// tb_flush >> must return 
			nb_flush++;	tb_flushed=1; void display_stat(); return;
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
		fprintf(stdout,"Should pass trace file as argument!\n");
		return -1; 
	}

		
   f=fopen(&argv[1][0],"r");
   if(f == NULL)
    {
        fprintf(stdout,"src file not found! Exiting...\n");
        return -1;
    }
    
   if (remove("results.dat") == -1)
   perror("Error in deleting a file"); 

	/* Clear screen at startup */
	if (system( "clear" )) system( "cls" );
	fprintf(stdout,"\n *** Qemu Translation Cache trace tool *** TIMA LAB - March 2013 ***\n\n");    
   
   display_menu();

while(1) {
   read_char=getchar();
   switch(read_char) {
   	case '1': read_log(f); break;
   	case '2': display_stat();break;
   	case '3': fprintf(stdout,"Actual loop_exec=%d, Enter new value: ",loop_exec);scanf("%u",&loop_exec);break;
   	
   	case '0': exit(EXIT_SUCCESS);
   	default:  break;
   	}
}
   
   fclose(f);


}
   

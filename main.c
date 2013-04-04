#include <stdio.h>
#include <string.h>

#define LINE_MAX 100
#define NB_EXEC 1000

unsigned long nb_exec=0, nb_tran=0, nb_flush=0,last_tb_exec;

void display_stat()
{
	fprintf(stdout,"nb_exec  = %u\n",nb_exec);
//	fprintf(stdout,"nb_trans = %u\n",nb_tran);
//	fprintf(stdout,"nb_flush = %u\n",nb_flush);
	fprintf(stdout,"exec since last flush = %u\n",nb_exec-last_tb_exec);
	fprintf(stdout,"exec ratio = %f\n",(float)(nb_exec-last_tb_exec)/NB_EXEC);
	}

void display_menu()
{
  	fprintf(stdout,"Choose the command to execute:\n");
	fprintf(stdout,"1 - Read file\n");
	fprintf(stdout,"2 - Display Stat\n");
	fprintf(stdout,"3 - Exit\n");
	}

void read_log(FILE *f,int length)
{
	int next_line_is_adress=0;
   char line[LINE_MAX];
   char tmp[LINE_MAX];
	int nb_lines=0;
	last_tb_exec=nb_exec;
	   while((fgets(line,LINE_MAX,f)!= NULL) || (nb_lines < length))
   {
   	if (next_line_is_adress) 
   	{												// should convert adress to int
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
		case 'F': fprintf(stdout,line); 		// tb_flush >> must return 
			nb_flush++;	return;
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
	

	/* Clear screen at startup */
	if (system( "clear" )) system( "cls" );
	fprintf(stdout,"\n *** Qemu Translation Cache trace tool *** TIMA LAB - March 2013 ***\n\n");
		
   f=fopen(&argv[1][0],"r");
   if(f == NULL)
    {
        fprintf(stdout,"src file not found! Exiting...\n");
        return -1;
    }
    
   display_menu();

while(1) {
   read_char=getchar();
   switch(read_char) {
   	case '1': read_log(f,NB_EXEC); break;
   	case '2': display_stat();
   	default:break;
   	}
}
   
   fclose(f);


}
   

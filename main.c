#include <stdio.h>
#include <string.h>

#define LINE_MAX 100

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
	   while((fgets(line,LINE_MAX,f)!= NULL) && (line[0]!='F'))//(nb_lines < length))
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
			break;
      case 'O': fprintf(stdout,line+5);		// should convert size to int
      	break;
      case 'T': fprintf(stdout,line+21); 		// Trace case..
			break;     
		case 'F':  fprintf(stdout,line); 
			break;
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
   	case '1': read_log(f,1000); break;
   	
   	default:break;
   	}
}
   
   fclose(f);


}
   

#include <stdio.h>

void main ( void )
{
    FILE *fptr;

    fptr = fopen("output.txt","w");

    if(fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }

    fprintf(fptr,"1\t2\t3\t4\r\n");

    fclose(fptr);
}

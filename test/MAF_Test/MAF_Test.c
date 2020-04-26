#include <stdio.h>

#include "maf.h"

int main ( void )
{
    int i = 0;

    double testArray[ 32 ] = { 1, 2, 10, 5, 15, 8, 12, 20, 10, 15, 16, 17, 18, 19, 20 };
    double tempVal1 = 0;
    double tempVal2 = 0;

    Maf_t test1;
    Maf_t test2;

    Maf_Init ( &test1, 5, 0 );
    Maf_Init ( &test2, 20, 0 );

    for ( i = 0; i < 15; ++i )
    {
        Maf_Iteration ( &test1, testArray[ i ] );
        Maf_Iteration ( &test2, testArray[ i ] );

        tempVal1 = Maf_GetOutput ( &test1 );
        tempVal2 = Maf_GetOutput ( &test2 );

        printf ( "%d-\t%.3f\t%.3f\t%.3f\r\n", i, testArray[ i ], tempVal1, tempVal2 );
    }

    return ( 1 );
}

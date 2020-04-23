#include <stdio.h>

#include "emaf.h"

int main ( void )
{
    int i = 0;

    double testArray[ 32 ] = { 1, 2, 10, 5, 15, 8, 12, 20, 10, 15, 16, 17, 18, 19, 20 };
    double tempVal1 = 0;
    double tempVal2 = 0;

    Emaf_t test1;
    Emaf_t test2;

    Emaf_Init ( &test1, 0.5, 0 );
    Emaf_Init ( &test2, 0.1, 0 );

    for ( i = 0; i < 15; ++i )
    {
        Emaf_Iteration ( &test1, testArray[ i ] );
        Emaf_Iteration ( &test2, testArray[ i ] );

        tempVal1 = Emaf_GetOutput ( &test1 );
        tempVal2 = Emaf_GetOutput ( &test2 );

        printf ( "%d-\t%.3f\t%.3f\t%.3f\r\n", i, testArray[ i ], tempVal1, tempVal2 );
    }

    return ( 1 );
}

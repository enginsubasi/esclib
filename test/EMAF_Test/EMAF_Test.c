#include <stdio.h>

#include "emaf.h"

int main ( void )
{
    int i = 0;

    float testArray[ 32 ] = { 1, 2, 10, 5, 15, 8, 12, 20, 10, 15, 16, 17, 18, 19, 20 };
    float tempVal1 = 0;
    float tempVal2 = 0;

    emaf_t test1;
    emaf_t test2;

    emafInit ( &test1, 0.5, 0 );
    emafInit ( &test2, 0.1, 0 );

    for ( i = 0; i < 15; ++i )
    {
        emafIteration ( &test1, testArray[ i ] );
        emafIteration ( &test2, testArray[ i ] );

        tempVal1 = emafGetOutput ( &test1 );
        tempVal2 = emafGetOutput ( &test2 );

        printf ( "%d-\t%.3f\t%.3f\t%.3f\r\n", i, testArray[ i ], tempVal1, tempVal2 );
    }

    return ( 1 );
}

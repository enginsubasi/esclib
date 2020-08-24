#include <stdio.h>

#include "pid.h"

int main ( void )
{
    int i = 0;

    float testArray[ 32 ] = { 1, 2, 80, 81, 100, 101, 119, 120, 121, 110, 100, 90, 80, 70, 10 };
    float tempVal1 = 0;
    float tempVal2 = 0;

    pidc_t test1;
    pidc_t test2;

    pidInit ( &test1, 0.1, 0.001, 0.001, 10, 0, 100, 0 );
    pidInit ( &test2, 0.5, 0.001, 0.01, 10, 0, 100, 0 );

    for ( i = 0; i < 15; ++i )
    {
        pidControl ( &test1, testArray[ i ] );
        pidControl ( &test2, testArray[ i ] );

        tempVal1 = pidGetOutput ( &test1 );
        tempVal2 = pidGetOutput ( &test2 );

        printf ( "%d-\t%.3f\t%.3f\t%.3f\r\n", i, testArray[ i ], tempVal1, tempVal2 );
    }

    return ( 1 );
}

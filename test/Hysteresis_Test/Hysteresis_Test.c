#include <stdio.h>

#include "hysteresis.h"

int main ( void )
{
    int i = 0;

    float testArray[ 32 ] = { 1, 2, 80, 81, 100, 101, 119, 120, 121, 110, 100, 90, 80, 70, 10 };
    float tempVal1 = 0;
    float tempVal2 = 0;

    hysteresis_t test1;
    hysteresis_t test2;

    hysteresisInit ( &test1, 110, 90 );
    hysteresisInit ( &test2, 120, 80 );

    for ( i = 0; i < 15; ++i )
    {
        hysteresisControl ( &test1, testArray[ i ] );
        hysteresisControl ( &test2, testArray[ i ] );

        tempVal1 = hysteresisGetOutput ( &test1 );
        tempVal2 = hysteresisGetOutput ( &test2 );

        printf ( "%d-\t%.3f\t%.3f\t%.3f\r\n", i, testArray[ i ], tempVal1, tempVal2 );
    }

    return ( 1 );
}

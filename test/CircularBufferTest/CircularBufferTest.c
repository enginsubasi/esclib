#include <stdio.h>

#include "circBuf.h"

#define BUFSIZE 10

uint32_t buf[ BUFSIZE ];

circBufu32_t cb;

uint32_t readTemp;

void printBuffer ( uint32_t* buf, uint32_t size )
{
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        printf ( "%d ", buf[ i ] );
    }
    printf ( "\r\n" );
}

int main ( void )
{
    int i = 0;
    uint8_t retValTemp = 0;

    circBufInitu32 ( &cb, buf, BUFSIZE, BB_STOP );
    circBufInitu32 ( &cb, buf, BUFSIZE, BB_OVERWRITE );
    circBufInitu32 ( &cb, buf, BUFSIZE, BB_OVERWRITE );

    printBuffer ( cb.buffer, cb.capacity );

    for ( i = 0; i < 5; ++i )
    {
        circBufAddu32 ( &cb, i*2 );
        printf ( "W-S: %d, W: %d, R: %d L: %d-\t\t", cb.status, cb.wp, cb.rp, circBufGetLengthu32 ( &cb ) );
        printBuffer ( cb.buffer, cb.capacity );
    }

    for ( i = 0; i < 15; ++i )
    {
        retValTemp = circBufReadu32 ( &cb, &readTemp );

        //printf ( "R: %d, D: %d\r\n", retValTemp, readTemp );
        printf ( "R-S: %d, W: %d, R: %d L: %d D: %d-\t\t", cb.status, cb.wp, cb.rp, circBufGetLengthu32 ( &cb ), readTemp );
        printBuffer ( cb.buffer, cb.capacity );
    }

    for ( i = 1; i < 15; ++i )
    {
        circBufAddu32 ( &cb, i*3 );
        printf ( "W-S: %d, W: %d, R: %d L: %d-\t\t", cb.status, cb.wp, cb.rp, circBufGetLengthu32 ( &cb ) );
        printBuffer ( cb.buffer, cb.capacity );
    }

    for ( i = 0; i < 15; ++i )
    {
        retValTemp = circBufReadu32 ( &cb, &readTemp );

        //printf ( "R: %d, D: %d\r\n", retValTemp, readTemp );
        printf ( "R-S: %d, W: %d, R: %d L: %d D: %d-\t\t", cb.status, cb.wp, cb.rp, circBufGetLengthu32 ( &cb ), readTemp );
        printBuffer ( cb.buffer, cb.capacity );
    }



    return ( 1 );
}

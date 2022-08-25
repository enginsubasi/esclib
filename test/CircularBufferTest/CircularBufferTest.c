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
    circBufInitu32 ( &cb, buf, BUFSIZE, BB_STOP );

    printBuffer ( cb.buffer, cb.capacity );

    for ( i = 0; i < 15; ++i )
    {
        circBufAddu32 ( &cb, i*2 );
        printf ( "S: %d, W: %d, R: %d-", cb.status, cb.wp, cb.rp );
        printBuffer ( cb.buffer, cb.capacity );
    }

    for ( i = 0; i < 15; ++i )
    {
        retValTemp = circBufReadu32 ( &cb, &readTemp );

        printf ( "R: %d, D: %d\r\n", retValTemp, readTemp );
        printf ( "%d-", cb.status );
        printBuffer ( cb.buffer, cb.capacity );
    }

    return ( 1 );
}

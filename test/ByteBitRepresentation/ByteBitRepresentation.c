#include <stdio.h>
#include <stdint.h>

struct bitDef
{
    uint8_t b0:1;
    uint8_t b1:1;
    uint8_t b2:1;
    uint8_t b3:1;
    uint8_t b4:1;
    uint8_t b5:1;
    uint8_t b6:1;
    uint8_t b7:1;
};

typedef union
{
    struct	bitDef bit;
    uint8_t	byte;
} byteBitDef;

int main ( void )
{
    byteBitDef testUnion;

    testUnion.bit.b5 = 1;
    testUnion.bit.b6 = 1;

    printf ( "Test\r\n" );

    printf ( "%u\r\n",testUnion.byte );

    printf ( "%d %d\r\n", sizeof ( testUnion ), sizeof ( testUnion.bit ) );

    return ( 0 );
}

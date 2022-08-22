#include <stdio.h>

#include "complex.h"

void complexPrint ( complex_t* cprm1 )
{
    printf ( "r:%.2f i:%.2f\r\n", cprm1->re, cprm1->im );
}

complex_t t1, t2, t3;
float r, a;

int main ( void )
{
    int i = 0;

    complexInit ( &t1, 3, 4 );
    complexInit ( &t2, 5, 12 );

    complexPrint ( &t1 );

    complexSum ( &t1, &t2, &t3 );

    complexPrint ( &t3 );

    complexToPolar ( &t2, &r, &a );

    printf ( "r: %.2f a: %.2f\r\n", r, a );

    return ( 1 );
}


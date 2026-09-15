#include <stdio.h>

int main ( void )
{
    float x = 37.370f;

    void *vtof = &x;

    printf ( "%f", *( float * ) vtof );

    return ( 0 );
}

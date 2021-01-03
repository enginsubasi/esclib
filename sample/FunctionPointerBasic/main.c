#include <stdio.h>

int func1 ( int arg1, int arg2 );
int func2 ( int arg1, int arg2 );

void PassPtr ( int x,int ( *pt2Func )( int, int ) )
{
    int sonuc = ( *pt2Func )( 12, 3 );
    printf ( "%d\n", sonuc + x );
}

int main ( void )
{
    // void function pointer
    void *fp;

    // assign func1 address to fp
    fp = func1;

    // run function
    PassPtr ( 4, fp );

    // assign func2 address to fp
    fp = func2;

    // // run function
    PassPtr ( 4, fp );

    // check outputs
    // change stg. see effect

    return ( 1 );
}

int func1 ( int arg1, int arg2 )
{
    return ( ( ++arg1 ) + arg2 );
}

int func2 ( int arg1, int arg2 )
{
    return ( ( ++arg1 ) * arg2 );
}



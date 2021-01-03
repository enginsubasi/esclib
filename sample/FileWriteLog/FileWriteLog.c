/**
  ******************************************************************************
  *
  * @file:      FileWriteLog.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi/esclib
  *
  * @version:   v 0.0.1
  * @cdate:     04/10/2018
  * @mdate:     04/10/2018
  * @history:   04/10/2018 Created
  *
  * @about:     File write example
  * @device:
  *
  * @content:
  *     FUNCTIONS:
  *
  * @notes:
  *     Opening Types:
  *         r : Opens an existing text file for reading purpose.
  *
  *         w : Opens a text file for writing. If it does not exist, then a new
  *     file is created. Here your program will start writing content from
  *     the beginning of the file.
  *
  *         a : Opens a text file for writing in appending mode. If it does not
  *     exist, then a new file is created. Here your program will start
  *     appending content in the existing file content.
  *
  *         r+: Opens a text file for both reading and writing.
  *
  *         w+: Opens a text file for both reading and writing. It first
  *     truncates the file to zero length if it exists, otherwise creates a file
  *     if it does not exist.
  *
  *         a+: Opens a text file for both reading and writing. It creates the
  *     file if it does not exist. The reading will start from the beginning but
  *     writing can only be appended.
  *
  ******************************************************************************
  */

#include <stdio.h>
#include <stdint.h>

int main ( void )
{
    uint32_t i;
    FILE *fp;

    fp = fopen ( "test.txt", "w" );

    fprintf ( fp, "Beginning of file...\n" );

    for ( i = 0 ; i < 10 ; ++i )
    {
        fprintf ( fp, "%d\n",i );
    }

    fprintf ( fp, "End of file...\n" );

    fclose ( fp );

    return ( 0 );
}

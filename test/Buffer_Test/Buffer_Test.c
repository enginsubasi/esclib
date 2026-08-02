/*
 * Covers circBuf, both widths.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure. CircularBufferTest already exists
 * and prints; this covers what that one does not reach, which is the whole u8
 * half of the module, both overflow behaviours and the status reporting.
 */

#include <stddef.h>
#include <stdio.h>

#include "circBuf.h"

static uint32_t failures = 0;

static void check ( const char* what, uint8_t condition )
{
    if ( condition == TRUE )
    {
        printf ( "  PASS  %s\n", what );
    }
    else
    {
        printf ( "  FAIL  %s\n", what );
        ++failures;
    }
}

/* --------------------------------------------------------------- u32 init */

static void initCase ( void )
{
    circBufu32_t driver;
    circBufu8_t driver8;
    uint32_t storage[ 4 ];
    uint8_t storage8[ 4 ];

    printf ( "circBufInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( circBufInitu32 ( NULL, storage, 4u, BB_STOP ) == FALSE ) );
    check ( "a NULL buffer is rejected",
            ( uint8_t ) ( circBufInitu32 ( &driver, NULL, 4u, BB_STOP ) == FALSE ) );
    check ( "a zero capacity is rejected",
            ( uint8_t ) ( circBufInitu32 ( &driver, storage, 0u, BB_STOP ) == FALSE ) );
    check ( "a valid init succeeds",
            circBufInitu32 ( &driver, storage, 4u, BB_STOP ) );
    check ( "and the buffer starts empty",
            ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_EMPTY ) );
    check ( "with a length of zero",
            ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 0u ) );

    check ( "the u8 form rejects a NULL driver",
            ( uint8_t ) ( circBufInitu8 ( NULL, storage8, 4u, BB_STOP ) == FALSE ) );
    check ( "the u8 form rejects a NULL buffer",
            ( uint8_t ) ( circBufInitu8 ( &driver8, NULL, 4u, BB_STOP ) == FALSE ) );
    check ( "the u8 form rejects a zero capacity",
            ( uint8_t ) ( circBufInitu8 ( &driver8, storage8, 0u, BB_STOP ) == FALSE ) );
    check ( "and accepts a valid one",
            circBufInitu8 ( &driver8, storage8, 4u, BB_STOP ) );
    check ( "starting empty",
            ( uint8_t ) ( circBufGetStatusu8 ( &driver8 ) == BS_EMPTY ) );
}

/* ------------------------------------------------------ fill, drain, u32 */

static void fillDrainCase ( void )
{
    circBufu32_t driver;
    uint32_t storage[ 4 ];
    uint32_t value = 0;

    printf ( "circBuf fill and drain, BB_STOP\n" );

    check ( "Init", circBufInitu32 ( &driver, storage, 4u, BB_STOP ) );

    check ( "the first add succeeds", circBufAddu32 ( &driver, 10u ) );
    check ( "the buffer is no longer empty",
            ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_NOTEMPTY ) );
    check ( "and holds one word", ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 1u ) );

    check ( "second add", circBufAddu32 ( &driver, 20u ) );
    check ( "third add", circBufAddu32 ( &driver, 30u ) );
    check ( "the fourth add fills it", circBufAddu32 ( &driver, 40u ) );
    check ( "the status says so",
            ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_FULL ) );
    check ( "and the length is the capacity",
            ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 4u ) );

    /* BB_STOP refuses rather than dropping the oldest word. */
    check ( "a fifth add is refused",
            ( uint8_t ) ( circBufAddu32 ( &driver, 50u ) == FALSE ) );
    check ( "and the length did not change",
            ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 4u ) );

    check ( "the first read succeeds", circBufReadu32 ( &driver, &value ) );
    check ( "and returns the oldest word", ( uint8_t ) ( value == 10u ) );
    check ( "the buffer is no longer full",
            ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_NOTEMPTY ) );
    check ( "and one shorter", ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 3u ) );

    check ( "second read", circBufReadu32 ( &driver, &value ) );
    check ( "in order", ( uint8_t ) ( value == 20u ) );
    check ( "third read", circBufReadu32 ( &driver, &value ) );
    check ( "still in order", ( uint8_t ) ( value == 30u ) );
    check ( "fourth read", circBufReadu32 ( &driver, &value ) );
    check ( "the last word", ( uint8_t ) ( value == 40u ) );

    check ( "the buffer is empty again",
            ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_EMPTY ) );
    check ( "with a length of zero",
            ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 0u ) );

    value = 0xDEADu;
    check ( "reading an empty buffer reports failure",
            ( uint8_t ) ( circBufReadu32 ( &driver, &value ) == FALSE ) );
    check ( "and writes zero rather than leaving the caller's variable",
            ( uint8_t ) ( value == 0u ) );

    /* And it works again after being drained, which is where a stale pointer shows. */
    check ( "it accepts a new word after being drained",
            circBufAddu32 ( &driver, 99u ) );
    check ( "read it back", circBufReadu32 ( &driver, &value ) );
    check ( "with the right value", ( uint8_t ) ( value == 99u ) );
}

/* -------------------------------------------------------------- overwrite */

static void overwriteCase ( void )
{
    circBufu32_t driver;
    uint32_t storage[ 3 ];
    uint32_t value = 0;

    printf ( "circBuf BB_OVERWRITE\n" );

    check ( "Init", circBufInitu32 ( &driver, storage, 3u, BB_OVERWRITE ) );

    ( void ) circBufAddu32 ( &driver, 1u );
    ( void ) circBufAddu32 ( &driver, 2u );
    ( void ) circBufAddu32 ( &driver, 3u );

    check ( "the buffer is full", ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_FULL ) );

    /* Overwriting reports success, unlike BB_STOP, and drops the oldest word. */
    check ( "an add past full succeeds", circBufAddu32 ( &driver, 4u ) );
    check ( "and the buffer is still full",
            ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_FULL ) );
    check ( "with the capacity as its length",
            ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 3u ) );

    /*
     * The oldest word is the one that goes. What is left must still come out
     * oldest first, which is the part a wrong read pointer would break.
     */
    check ( "read one", circBufReadu32 ( &driver, &value ) );
    check ( "the dropped word is gone and 2 is now oldest",
            ( uint8_t ) ( value == 2u ) );
    check ( "read two", circBufReadu32 ( &driver, &value ) );
    check ( "then 3", ( uint8_t ) ( value == 3u ) );
    check ( "read three", circBufReadu32 ( &driver, &value ) );
    check ( "then the word that overwrote", ( uint8_t ) ( value == 4u ) );
    check ( "and the buffer is empty",
            ( uint8_t ) ( circBufGetStatusu32 ( &driver ) == BS_EMPTY ) );

    /* Wrapping many times must not drift the pointers. */
    check ( "Init again", circBufInitu32 ( &driver, storage, 3u, BB_OVERWRITE ) );

    {
        uint32_t i = 0;

        for ( i = 0; i < 100u; ++i )
        {
            ( void ) circBufAddu32 ( &driver, i );
        }
    }

    check ( "after a hundred writes the length is still the capacity",
            ( uint8_t ) ( circBufGetLengthu32 ( &driver ) == 3u ) );
    check ( "read one", circBufReadu32 ( &driver, &value ) );
    check ( "and the three newest words survived, oldest first",
            ( uint8_t ) ( value == 97u ) );
    check ( "read two", circBufReadu32 ( &driver, &value ) );
    check ( "second newest", ( uint8_t ) ( value == 98u ) );
    check ( "read three", circBufReadu32 ( &driver, &value ) );
    check ( "newest", ( uint8_t ) ( value == 99u ) );
}

/* -------------------------------------------------------------- the u8 half */

static void u8Case ( void )
{
    circBufu8_t driver;
    uint8_t storage[ 3 ];
    uint8_t value = 0;
    uint32_t i = 0;

    printf ( "circBuf u8\n" );

    check ( "Init with BB_STOP", circBufInitu8 ( &driver, storage, 3u, BB_STOP ) );

    check ( "add", circBufAddu8 ( &driver, 0x11u ) );
    check ( "length is one", ( uint8_t ) ( circBufGetLengthu8 ( &driver ) == 1u ) );
    check ( "status is not empty",
            ( uint8_t ) ( circBufGetStatusu8 ( &driver ) == BS_NOTEMPTY ) );

    check ( "add", circBufAddu8 ( &driver, 0x22u ) );
    check ( "add fills it", circBufAddu8 ( &driver, 0x33u ) );
    check ( "status is full", ( uint8_t ) ( circBufGetStatusu8 ( &driver ) == BS_FULL ) );
    check ( "an add past full is refused under BB_STOP",
            ( uint8_t ) ( circBufAddu8 ( &driver, 0x44u ) == FALSE ) );

    check ( "read", circBufReadu8 ( &driver, &value ) );
    check ( "oldest first", ( uint8_t ) ( value == 0x11u ) );
    check ( "read", circBufReadu8 ( &driver, &value ) );
    check ( "then the next", ( uint8_t ) ( value == 0x22u ) );
    check ( "read", circBufReadu8 ( &driver, &value ) );
    check ( "then the last", ( uint8_t ) ( value == 0x33u ) );
    check ( "empty again", ( uint8_t ) ( circBufGetStatusu8 ( &driver ) == BS_EMPTY ) );

    value = 0xAAu;
    check ( "reading empty reports failure",
            ( uint8_t ) ( circBufReadu8 ( &driver, &value ) == FALSE ) );
    check ( "and writes zero", ( uint8_t ) ( value == 0u ) );

    /* A byte buffer is the one most likely to see 0xFF, so check it round trips. */
    check ( "Init with BB_OVERWRITE", circBufInitu8 ( &driver, storage, 3u, BB_OVERWRITE ) );
    ( void ) circBufAddu8 ( &driver, 0x00u );
    ( void ) circBufAddu8 ( &driver, 0xFFu );
    check ( "read", circBufReadu8 ( &driver, &value ) );
    check ( "a zero byte survives", ( uint8_t ) ( value == 0x00u ) );
    check ( "read", circBufReadu8 ( &driver, &value ) );
    check ( "and so does 0xFF", ( uint8_t ) ( value == 0xFFu ) );

    check ( "Init once more", circBufInitu8 ( &driver, storage, 3u, BB_OVERWRITE ) );

    for ( i = 0; i < 50u; ++i )
    {
        ( void ) circBufAddu8 ( &driver, ( uint8_t ) i );
    }

    check ( "the u8 form wraps without drifting either",
            ( uint8_t ) ( circBufGetLengthu8 ( &driver ) == 3u ) );
    check ( "read", circBufReadu8 ( &driver, &value ) );
    check ( "and keeps the newest three", ( uint8_t ) ( value == 47u ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    fillDrainCase ( );
    printf ( "\n" );
    overwriteCase ( );
    printf ( "\n" );
    u8Case ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%u check(s) failed\n", ( unsigned ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}

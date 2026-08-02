/*
 * Covers sort and search, both the functions that were already here and the
 * ones added on 02/08/2026.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure. These two modules had no test at
 * all before this file, and they are among the files the July 2026 audit
 * changed most.
 */

#include <stdio.h>

#include "sort.h"
#include "search.h"

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

/* ------------------------------------------------------------- fixtures */

#define LEN     8u
#define BIGLEN  64u

static const float    srcF[ LEN ]  = { 5.0f, -2.0f, 9.5f, 0.0f, 5.0f, -7.25f, 3.0f, 1.0f };
static const float    wantF[ LEN ] = { -7.25f, -2.0f, 0.0f, 1.0f, 3.0f, 5.0f, 5.0f, 9.5f };

static const uint32_t srcU[ LEN ]  = { 5u, 40u, 9u, 0u, 5u, 4000000000u, 3u, 1u };
static const uint32_t wantU[ LEN ] = { 0u, 1u, 3u, 5u, 5u, 9u, 40u, 4000000000u };

static const int32_t  srcI[ LEN ]  = { 5, -2, 9, 0, 5, -2147483647 - 1, 3, 2147483647 };
static const int32_t  wantI[ LEN ] = { -2147483647 - 1, -2, 0, 3, 5, 5, 9, 2147483647 };

static uint8_t sameF ( const float* const a, const float* const b, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( a[ i ] != b[ i ] )
        {
            retVal = FALSE;
            break;
        }
    }

    return ( retVal );
}

static uint8_t sameU ( const uint32_t* const a, const uint32_t* const b, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( a[ i ] != b[ i ] )
        {
            retVal = FALSE;
            break;
        }
    }

    return ( retVal );
}

static uint8_t sameI ( const int32_t* const a, const int32_t* const b, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( a[ i ] != b[ i ] )
        {
            retVal = FALSE;
            break;
        }
    }

    return ( retVal );
}

static void copyF ( float* dst, const float* const src, uint32_t length )
{
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        dst[ i ] = src[ i ];
    }
}

static void copyU ( uint32_t* dst, const uint32_t* const src, uint32_t length )
{
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        dst[ i ] = src[ i ];
    }
}

static void copyI ( int32_t* dst, const int32_t* const src, uint32_t length )
{
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        dst[ i ] = src[ i ];
    }
}

/* ------------------------------------------------- the four sorts agree */

static void sortAgreementCase ( void )
{
    float work[ LEN ];
    uint32_t workU[ LEN ];
    int32_t workI[ LEN ];

    printf ( "every sort produces the same ascending order\n" );

    copyF ( work, srcF, LEN );
    sortSelection ( work, LEN );
    check ( "sortSelection", sameF ( work, wantF, LEN ) );

    copyF ( work, srcF, LEN );
    sortBubble ( work, LEN );
    check ( "sortBubble", sameF ( work, wantF, LEN ) );

    copyF ( work, srcF, LEN );
    sortInsertion ( work, LEN );
    check ( "sortInsertion", sameF ( work, wantF, LEN ) );

    copyF ( work, srcF, LEN );
    sortHeap ( work, LEN );
    check ( "sortHeap", sameF ( work, wantF, LEN ) );

    copyU ( workU, srcU, LEN );
    sortSelectionu32 ( workU, LEN );
    check ( "sortSelectionu32", sameU ( workU, wantU, LEN ) );

    copyU ( workU, srcU, LEN );
    sortBubbleu32 ( workU, LEN );
    check ( "sortBubbleu32", sameU ( workU, wantU, LEN ) );

    copyU ( workU, srcU, LEN );
    sortInsertionu32 ( workU, LEN );
    check ( "sortInsertionu32", sameU ( workU, wantU, LEN ) );

    copyU ( workU, srcU, LEN );
    sortHeapu32 ( workU, LEN );
    check ( "sortHeapu32, including a value past INT32_MAX", sameU ( workU, wantU, LEN ) );

    copyI ( workI, srcI, LEN );
    sortSelectioni32 ( workI, LEN );
    check ( "sortSelectioni32", sameI ( workI, wantI, LEN ) );

    copyI ( workI, srcI, LEN );
    sortBubblei32 ( workI, LEN );
    check ( "sortBubblei32", sameI ( workI, wantI, LEN ) );

    copyI ( workI, srcI, LEN );
    sortInsertioni32 ( workI, LEN );
    check ( "sortInsertioni32", sameI ( workI, wantI, LEN ) );

    copyI ( workI, srcI, LEN );
    sortHeapi32 ( workI, LEN );
    check ( "sortHeapi32, including both ends of the range", sameI ( workI, wantI, LEN ) );
}

/* ------------------------------------------------------- degenerate input */

static void sortDegenerateCase ( void )
{
    uint32_t work[ 3 ] = { 7u, 7u, 7u };
    uint32_t two[ 2 ] = { 9u, 4u };
    uint32_t three[ 3 ] = { 3u, 1u, 2u };

    printf ( "degenerate lengths\n" );

    /*
     * A zero length must not touch the buffer. Passing a real buffer and
     * checking it afterwards is the only way to see a walk past the end that
     * a length of zero used to cause here: length - 1 wrapped to 0xFFFFFFFF.
     */
    sortHeapu32 ( work, 0u );
    check ( "length 0 leaves the buffer alone",
            ( uint8_t ) ( ( work[ 0 ] == 7u ) && ( work[ 1 ] == 7u ) && ( work[ 2 ] == 7u ) ) );

    sortHeapu32 ( work, 1u );
    check ( "length 1 leaves the buffer alone", ( uint8_t ) ( work[ 0 ] == 7u ) );

    sortSelectionu32 ( work, 0u );
    sortBubbleu32 ( work, 0u );
    sortInsertionu32 ( work, 0u );
    check ( "the other three survive length 0 too",
            ( uint8_t ) ( ( work[ 0 ] == 7u ) && ( work[ 2 ] == 7u ) ) );

    sortHeapu32 ( work, 3u );
    check ( "all equal elements stay put",
            ( uint8_t ) ( ( work[ 0 ] == 7u ) && ( work[ 1 ] == 7u ) && ( work[ 2 ] == 7u ) ) );

    sortHeapu32 ( two, 2u );
    check ( "length 2", ( uint8_t ) ( ( two[ 0 ] == 4u ) && ( two[ 1 ] == 9u ) ) );

    sortHeapu32 ( three, 3u );
    check ( "length 3",
            ( uint8_t ) ( ( three[ 0 ] == 1u ) && ( three[ 1 ] == 2u ) && ( three[ 2 ] == 3u ) ) );
}

/* ------------------------------------------- heap sort over several levels */

static void sortHeapDepthCase ( void )
{
    int32_t work[ BIGLEN ];
    uint32_t i = 0;
    uint8_t ordered = TRUE;

    printf ( "heap sort over a deeper heap\n" );

    /* Reverse order is the shape that makes the build phase do the most work. */
    for ( i = 0; i < BIGLEN; ++i )
    {
        work[ i ] = ( int32_t ) ( BIGLEN - i );
    }

    sortHeapi32 ( work, BIGLEN );

    for ( i = 0; i < BIGLEN; ++i )
    {
        if ( work[ i ] != ( int32_t ) ( i + 1u ) )
        {
            ordered = FALSE;
            break;
        }
    }

    check ( "64 elements in reverse order come out ascending", ordered );
    check ( "and sortIsSortedi32 agrees", sortIsSortedi32 ( work, BIGLEN ) );
}

/* --------------------------------------------------- reverse and isSorted */

static void reverseAndIsSortedCase ( void )
{
    float work[ LEN ];
    uint32_t workU[ 4 ] = { 1u, 2u, 3u, 4u };
    uint8_t descending = TRUE;
    uint32_t i = 0;

    printf ( "sortReverse and sortIsSorted\n" );

    copyF ( work, srcF, LEN );
    sortHeap ( work, LEN );
    sortReverse ( work, LEN );

    for ( i = 1; i < LEN; ++i )
    {
        if ( work[ i - 1u ] < work[ i ] )
        {
            descending = FALSE;
            break;
        }
    }

    check ( "sort then reverse gives descending order", descending );
    check ( "the largest is first", ( uint8_t ) ( work[ 0 ] == 9.5f ) );
    check ( "the smallest is last", ( uint8_t ) ( work[ LEN - 1u ] == -7.25f ) );

    sortReverse ( work, LEN );
    check ( "reversing twice restores the sorted order", sameF ( work, wantF, LEN ) );

    check ( "an ascending array reports sorted", sortIsSortedu32 ( workU, 4u ) );
    workU[ 2 ] = 0u;
    check ( "one pair out of order reports unsorted",
            ( uint8_t ) ( sortIsSortedu32 ( workU, 4u ) == FALSE ) );

    check ( "an empty array reports sorted", sortIsSortedu32 ( workU, 0u ) );
    check ( "a single element reports sorted", sortIsSortedu32 ( workU, 1u ) );

    check ( "equal neighbours count as sorted",
            sortIsSortedi32 ( wantI, LEN ) );

    sortReverseu32 ( workU, 0u );
    sortReverseu32 ( workU, 1u );
    check ( "sortReverse survives lengths 0 and 1", ( uint8_t ) ( workU[ 0 ] == 1u ) );

    {
        int32_t workI[ 4 ] = { -7, -2, 0, 9 };

        sortReversei32 ( workI, 4u );
        check ( "sortReversei32 reverses across both signs",
                ( uint8_t ) ( ( workI[ 0 ] == 9 ) && ( workI[ 1 ] == 0 ) &&
                              ( workI[ 2 ] == -2 ) && ( workI[ 3 ] == -7 ) ) );
    }
}

/* -------------------------------------------------- linear and binary find */

static void findCase ( void )
{
    uint32_t idx = 0xAAAAAAAAu;
    float epsilon = 0.001f;

    printf ( "searchLinear and searchBinary\n" );

    check ( "searchLinearu32 finds a present item",
            searchLinearu32 ( srcU, LEN, 9u, &idx ) );
    check ( "and reports the right index", ( uint8_t ) ( idx == 2u ) );

    idx = 0xAAAAAAAAu;
    check ( "searchLinearu32 reports a missing item",
            ( uint8_t ) ( searchLinearu32 ( srcU, LEN, 12345u, &idx ) == FALSE ) );
    check ( "and leaves foundIndex alone", ( uint8_t ) ( idx == 0xAAAAAAAAu ) );

    check ( "a zero length reports nothing found",
            ( uint8_t ) ( searchLinearu32 ( srcU, 0u, 5u, &idx ) == FALSE ) );

    /*
     * searchLinear used to carry a stray semicolon that made it report a match
     * at index 0 whatever it was handed. A search for an item that is not in
     * the array is what catches that.
     */
    check ( "searchLinear reports a missing float",
            ( uint8_t ) ( searchLinear ( wantF, LEN, 123.0f, &idx, epsilon ) == FALSE ) );
    check ( "searchLinear finds a present float",
            searchLinear ( wantF, LEN, 3.0f, &idx, epsilon ) );
    check ( "at the right index", ( uint8_t ) ( idx == 4u ) );

    check ( "searchBinaryu32 finds the first element",
            searchBinaryu32 ( wantU, LEN, 0u, &idx ) );
    check ( "at index 0", ( uint8_t ) ( idx == 0u ) );

    check ( "searchBinaryu32 finds the last element",
            searchBinaryu32 ( wantU, LEN, 4000000000u, &idx ) );
    check ( "at the last index", ( uint8_t ) ( idx == ( LEN - 1u ) ) );

    check ( "searchBinaryu32 reports an absent item between two present ones",
            ( uint8_t ) ( searchBinaryu32 ( wantU, LEN, 7u, &idx ) == FALSE ) );

    /*
     * An item below every element used to drive the right bound below zero and
     * wrap it to 0xFFFFFFFF.
     */
    check ( "searchBinaryi32 reports an item below every element",
            ( uint8_t ) ( searchBinaryi32 ( wantI, LEN, -2147483647 - 1 + 1, &idx ) == FALSE ) );
    check ( "searchBinaryi32 reports an item above every element",
            ( uint8_t ) ( searchBinaryi32 ( wantI, LEN, 2147483646, &idx ) == FALSE ) );
    check ( "searchBinaryi32 with a zero length",
            ( uint8_t ) ( searchBinaryi32 ( wantI, 0u, 0, &idx ) == FALSE ) );

    check ( "searchLineari32 finds a negative item",
            searchLineari32 ( srcI, LEN, -2, &idx ) );
    check ( "at the right index", ( uint8_t ) ( idx == 1u ) );
    check ( "searchLineari32 finds INT32_MIN",
            searchLineari32 ( srcI, LEN, -2147483647 - 1, &idx ) );
    check ( "at its index", ( uint8_t ) ( idx == 5u ) );

    idx = 0xAAAAAAAAu;
    check ( "searchLineari32 reports a missing item",
            ( uint8_t ) ( searchLineari32 ( srcI, LEN, 12345, &idx ) == FALSE ) );
    check ( "and leaves foundIndex alone", ( uint8_t ) ( idx == 0xAAAAAAAAu ) );
}

/* --------------------------------------------------- lower and upper bound */

static void boundCase ( void )
{
    /* Three 20s so the two bounds have a run to bracket. */
    static const uint32_t tbl[ 7 ] = { 10u, 20u, 20u, 20u, 30u, 40u, 50u };

    printf ( "searchLowerBound and searchUpperBound\n" );

    check ( "lower bound of a value below the table is 0",
            ( uint8_t ) ( searchLowerBoundu32 ( tbl, 7u, 1u ) == 0u ) );
    check ( "upper bound of a value below the table is 0",
            ( uint8_t ) ( searchUpperBoundu32 ( tbl, 7u, 1u ) == 0u ) );

    check ( "lower bound of a value above the table is the length",
            ( uint8_t ) ( searchLowerBoundu32 ( tbl, 7u, 99u ) == 7u ) );
    check ( "upper bound of a value above the table is the length",
            ( uint8_t ) ( searchUpperBoundu32 ( tbl, 7u, 99u ) == 7u ) );

    check ( "lower bound lands on the first of a run",
            ( uint8_t ) ( searchLowerBoundu32 ( tbl, 7u, 20u ) == 1u ) );
    check ( "upper bound lands one past the last of a run",
            ( uint8_t ) ( searchUpperBoundu32 ( tbl, 7u, 20u ) == 4u ) );
    check ( "so their difference counts the duplicates",
            ( uint8_t ) ( ( searchUpperBoundu32 ( tbl, 7u, 20u ) -
                            searchLowerBoundu32 ( tbl, 7u, 20u ) ) == 3u ) );

    check ( "for an absent value the two bounds agree",
            ( uint8_t ) ( searchLowerBoundu32 ( tbl, 7u, 25u ) ==
                          searchUpperBoundu32 ( tbl, 7u, 25u ) ) );
    check ( "and that is the position an insert has to go",
            ( uint8_t ) ( searchLowerBoundu32 ( tbl, 7u, 25u ) == 4u ) );

    check ( "a zero length gives 0",
            ( uint8_t ) ( searchLowerBoundu32 ( tbl, 0u, 20u ) == 0u ) );

    check ( "the float bound works the same",
            ( uint8_t ) ( searchLowerBound ( wantF, LEN, 2.0f ) == 4u ) );
    check ( "and lands on the first of the two 5.0 entries",
            ( uint8_t ) ( searchLowerBound ( wantF, LEN, 5.0f ) == 5u ) );
    check ( "while the upper bound steps past both",
            ( uint8_t ) ( searchUpperBound ( wantF, LEN, 5.0f ) == 7u ) );

    check ( "the signed bound handles the bottom of the range",
            ( uint8_t ) ( searchLowerBoundi32 ( wantI, LEN, -2147483647 - 1 ) == 0u ) );
    check ( "and the top",
            ( uint8_t ) ( searchUpperBoundi32 ( wantI, LEN, 2147483647 ) == LEN ) );
}

/* ---------------------------------------------------------------- closest */

static void closestCase ( void )
{
    static const uint32_t tbl[ 4 ] = { 10u, 20u, 30u, 40u };
    static const uint32_t dup[ 5 ] = { 10u, 20u, 20u, 20u, 90u };
    static const int32_t  ends[ 2 ] = { -2147483647 - 1, 2147483647 };
    uint32_t idx = 0xAAAAAAAAu;

    printf ( "searchClosest\n" );

    check ( "an exact hit", searchClosestu32 ( tbl, 4u, 30u, &idx ) );
    check ( "at its own index", ( uint8_t ) ( idx == 2u ) );

    check ( "a value between two entries", searchClosestu32 ( tbl, 4u, 22u, &idx ) );
    check ( "picks the nearer one", ( uint8_t ) ( idx == 1u ) );

    check ( "a value nearer the upper entry", searchClosestu32 ( tbl, 4u, 28u, &idx ) );
    check ( "picks that one", ( uint8_t ) ( idx == 2u ) );

    check ( "a midpoint tie", searchClosestu32 ( tbl, 4u, 25u, &idx ) );
    check ( "goes to the lower index", ( uint8_t ) ( idx == 1u ) );

    check ( "a value below the table", searchClosestu32 ( tbl, 4u, 0u, &idx ) );
    check ( "clamps to the first entry", ( uint8_t ) ( idx == 0u ) );

    check ( "a value above the table", searchClosestu32 ( tbl, 4u, 4000000000u, &idx ) );
    check ( "clamps to the last entry", ( uint8_t ) ( idx == 3u ) );

    idx = 0xAAAAAAAAu;
    check ( "a zero length reports FALSE",
            ( uint8_t ) ( searchClosestu32 ( tbl, 0u, 20u, &idx ) == FALSE ) );
    check ( "and leaves foundIndex alone", ( uint8_t ) ( idx == 0xAAAAAAAAu ) );

    /*
     * With a run of equal values the answer must be the first of the run,
     * whichever side of the item the run sits on. Reporting the last of the run
     * would make the index depend on how the binary search walked the table
     * rather than on the table itself.
     */
    check ( "a value just above a run of equals", searchClosestu32 ( dup, 5u, 21u, &idx ) );
    check ( "reports the first of the run", ( uint8_t ) ( idx == 1u ) );

    check ( "a value far above a run that ends the table",
            searchClosestu32 ( dup, 4u, 999u, &idx ) );
    check ( "also reports the first of the run", ( uint8_t ) ( idx == 1u ) );

    check ( "a value just below a run of equals", searchClosestu32 ( dup, 5u, 19u, &idx ) );
    check ( "reports the first of the run too", ( uint8_t ) ( idx == 1u ) );

    /*
     * The pair that breaks the naive signed subtraction. The gap between
     * INT32_MIN and INT32_MAX is 4294967295, which overflows int32_t; computing
     * it that way typically yields -1, so the exact match at index 1 loses to
     * the far end at index 0.
     */
    check ( "the widest possible signed gap", searchClosesti32 ( ends, 2u, 2147483647, &idx ) );
    check ( "still picks the exact match", ( uint8_t ) ( idx == 1u ) );

    check ( "and from the other end", searchClosesti32 ( ends, 2u, -2147483647 - 1, &idx ) );
    check ( "picks the exact match there", ( uint8_t ) ( idx == 0u ) );

    check ( "a value one above INT32_MIN", searchClosesti32 ( ends, 2u, -2147483647, &idx ) );
    check ( "stays at the low end", ( uint8_t ) ( idx == 0u ) );

    check ( "the float table", searchClosest ( wantF, LEN, 2.9f, &idx ) );
    check ( "finds the nearest entry", ( uint8_t ) ( idx == 4u ) );
}

int main ( void )
{
    sortAgreementCase ( );
    printf ( "\n" );
    sortDegenerateCase ( );
    printf ( "\n" );
    sortHeapDepthCase ( );
    printf ( "\n" );
    reverseAndIsSortedCase ( );
    printf ( "\n" );
    findCase ( );
    printf ( "\n" );
    boundCase ( );
    printf ( "\n" );
    closestCase ( );

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

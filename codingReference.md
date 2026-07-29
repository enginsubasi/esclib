# Coding Reference

## if and switch Statements

### if
    if ( ( a > b ) || ( c == d ) )
    {
        // Code to be executed.
    }
    else if ( ( e > f ) && ( g == h ) )
    {
        // Code to be executed.
    }
    else
    {
        // Code to be executed.
    }
    
### switch
    switch ( expression )
    {
        case val1:
            // Code to be executed.
        break;
        
        case val2:
            // Code to be executed.
        break;
        
        default:
            // Code to be executed.
        break;
    }

## Loop Statements

### for
    for ( i = 0; i < DEF_SIZE; ++i )
    {
        // Code to be executed.
    }

    for ( i = 0; i < DEF_SIZE; ++i )
    {
        // Code to be executed.
        
        if ( a == b )
        {
            // Only one break expression is acceptable.
            break;
        }
    }

### while
    while ( loopControl == TRUE )
    {
        // Code to be executed.
    }

### do-while
    do
    {
        // Code to be executed.
    } while ( loopControl == TRUE );
    
## array

### Initialization of an array
    uint8_t ar [ SIZE_OF_AR ] = { 0, 0, 0 };

### Allowed Operations on arrays
    Index op. ar [ 2 ] = 5;

### Forbidden Operations on arrays
    Increment on the pointer ++ar;

## Doxygen Comments

### Function comment block
    /**
     * @brief   Adds one word to the circular buffer.
     * @param[in,out] driver  Buffer state.
     * @param[in]     data    Word to store.
     * @return  TRUE when the word was stored, FALSE when the buffer is
     *          full and the behaviour is BB_STOP.
     */
    uint8_t circBufAddu32 ( circBufu32_t* driver, uint32_t data )

### Rules
    A block opens with /** ; a /* block is invisible to Doxygen.
    The tag set is exactly @brief, @param[in], @param[out], @param[in,out], @return, @note.
    @brief is one sentence ending in a period.
    Every parameter carries an explicit direction.
    @return appears on non-void functions only, and never on a void one.
    @note only when it carries real information, otherwise omit it.
    static helpers are documented too, not only public functions.

### driver parameter direction
    xxxInit                                                     [out]
    xxxUpdate, xxxIteration, xxxControl, xxxReceive,
    xxxEvaluate, xxxAdd, xxxRead, xxxTimeoutCounter,
    xxxChange*                                                  [in,out]
    xxxGetValue, xxxGetOutput, xxxGetLength, xxxGetStatus       [in]

    Exception: bininpGetRisingValue is [in,out], not [in], because
    reading it clears the rising flag it reports.

### File banner
    /**
      ******************************************************************************
      *
      * @file      maf.c
      * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
      * @version   2.1.0
      * @date      26/04/2020
      *
      * @brief     Moving average filter.
      *
      * @par Device
      * Generic
      *
      * @par History
      * 26/04/2020 Created @n
      * 07/06/2020 Naming style changed @n
      * 24/08/2020 Data type changed from double to float. @n
      * 29/07/2026 The u32 variants declared by maf.h are implemented. @n
      *            They were missing, which broke linking for any caller @n
      *            that used them. @n
      *
      ******************************************************************************
      */

Documentation lives in `.c` files only; headers stay pure declarations.



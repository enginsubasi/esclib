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



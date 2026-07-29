/**
  ******************************************************************************
  *
  * @file:      hc597_drv.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.2
  * @cdate:     23/05/2022
  * @history:   23/05/2022 Created
  *             29/07/2026 Bug fix. hc597DrvOneShoot only filled data[ 0 ] and
  *                        ignored driver->size, so a chain longer than one
  *                        device could not be read.
  *             29/07/2026 hc597Init drives the clock and load pins to a known
  *                        state, the way hc595Init already does.
  *
  * @about:     HC597 driver file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         hc597Init           :
  *         hc597DlyCtrl        :
  *         hc597DrvLoop        :
  *         hc597DrvOneShoot    :
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "hc597_drv.h"

/*
 * @about: HC597_Driver Initializer.
 */
void hc597Init ( struct HC597_Driver* driver,
                            uint8_t* dataPtr,
                            uint32_t dataSize,
                            uint8_t dlyType,
                            uint32_t dlyCount,
                            void ( *clkDrvFnc )( uint8_t ),
                            void ( *lodDrvFnc )( uint8_t ),
                            uint8_t ( *datDrvFnc )( void ),
                            void ( *dlyMsFnc )( uint32_t ),
                            void ( *dlyNopFnc )( uint32_t ) )
{
    driver->data = dataPtr;
    driver->size = dataSize;
    driver->dlyType = dlyType;
    driver->dlyCount = dlyCount;

    driver->clkDrv = clkDrvFnc;
    driver->lodDrv = lodDrvFnc;
    driver->datDrv = datDrvFnc;
    driver->dlyMs = dlyMsFnc;
    driver->dlyNop = dlyNopFnc;

    // Idle state. The load pulse is active low, so it idles high.
    driver->clkDrv ( FALSE );
    driver->lodDrv ( TRUE );
}

/*
 * @about:
 */
static void hc597DlyCtrl ( struct HC597_Driver* driver )
{
    if ( driver->dlyType == HC597_DLY_NO )
    {
        // Intentionally blank.
    }
    else if ( driver->dlyType == HC597_DLY_NOP )
    {
        driver->dlyNop ( driver->dlyCount );
    }
    else if ( driver->dlyType == HC597_DLY_MS )
    {
        driver->dlyMs ( driver->dlyCount );
    }
    else
    {
        driver->dlyType = HC597_DLY_MS;
        driver->dlyCount = DEF_DLY_COUNT;
    }
}

/*
 * @about:
 */
void hc597DrvLoop ( struct HC597_Driver* driver )
{

}

/*
 * @about:
 */
void hc597DrvOneShoot ( struct HC597_Driver* driver )
{
    uint32_t i = 0;
    uint32_t j = 0;

    // Latch the parallel inputs of the whole chain once.
    driver->clkDrv ( TRUE );
    driver->clkDrv ( FALSE );

    driver->lodDrv ( FALSE );
    driver->lodDrv ( TRUE );

    for ( i = 0; i < driver->size; ++i )
    {
        driver->data[ i ] = 0;

        for ( j = 0; j < 8; ++j )
        {
            hc597DlyCtrl ( driver );

            // Any non zero read is one bit, so it is normalized here.
            if ( driver->datDrv ( ) != FALSE )
            {
                driver->data[ i ] |= ( uint8_t ) ( 1u << j );
            }
            else
            {
                /* Intentionally blank. */
            }

            driver->clkDrv ( TRUE );
            driver->clkDrv ( FALSE );
        }
    }
}

/*
 * @about:
 */
void hc597DrvInterrupt ( struct HC597_Driver* driver )
{

}

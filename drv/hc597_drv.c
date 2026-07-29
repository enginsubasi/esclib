/**
  ******************************************************************************
  *
  * @file      hc597_drv.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.2
  * @date      23/05/2022
  *
  * @brief     HC597 driver file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 23/05/2022 Created @n
  * 29/07/2026 Bug fix. hc597DrvOneShoot only filled data[ 0 ] and @n
  *            ignored driver->size, so a chain longer than one @n
  *            device could not be read. @n
  * 29/07/2026 hc597Init drives the clock and load pins to a known @n
  *            state, the way hc595Init already does. @n
  *
  ******************************************************************************
  */

#include "hc597_drv.h"

/**
 * @brief   Initializes the HC597 parallel-in shift register driver and idles
 *          its pins.
 * @param[out] driver     Driver state to initialize.
 * @param[in]  dataPtr    Caller owned array filled on each transfer.
 * @param[in]  dataSize   Number of bytes in dataPtr.
 * @param[in]  dlyType    HC597_DLY_NO, HC597_DLY_MS or HC597_DLY_NOP.
 * @param[in]  dlyCount   Delay length, in the unit chosen by dlyType.
 * @param[in]  clkDrvFnc  Drives the shift clock pin.
 * @param[in]  lodDrvFnc  Drives the parallel load pin.
 * @param[in]  datDrvFnc  Reads the serial data pin.
 * @param[in]  dlyMsFnc   Blocks for the given number of milliseconds.
 * @param[in]  dlyNopFnc  Spins for the given number of no-op cycles.
 * @note    Drives the clock pin low and the load pin high before returning.
 *          The load pulse is active low, so this leaves the pins idle.
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

/**
 * @brief   Applies the delay configured for hc597DrvOneShoot's clock steps.
 * @param[in,out] driver  Driver state.
 * @note    When dlyType holds a value other than HC597_DLY_NO, HC597_DLY_MS
 *          or HC597_DLY_NOP, this repairs it to HC597_DLY_MS and dlyCount to
 *          DEF_DLY_COUNT, without delaying on this call.
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

/**
 * @brief   Not implemented.
 * @param[in,out] driver  Driver state.
 * @note    Reserved for a non blocking read transfer driven from the main
 *          loop. The body is empty, which is why the compiler reports
 *          driver as an unused parameter.
 */
void hc597DrvLoop ( struct HC597_Driver* driver )
{

}

/**
 * @brief   Latches the parallel inputs once, then clocks driver->size bytes
 *          in least significant bit first, filling driver->data.
 * @param[in,out] driver  Driver state; data is written with the bytes read
 *                        from datDrv, clocked by clkDrv.
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

/**
 * @brief   Not implemented.
 * @param[in,out] driver  Driver state.
 * @note    Reserved for a non blocking read transfer driven from an
 *          interrupt. The body is empty, which is why the compiler reports
 *          driver as an unused parameter.
 */
void hc597DrvInterrupt ( struct HC597_Driver* driver )
{

}

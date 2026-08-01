/**
  ******************************************************************************
  *
  * @file      hc595_drv.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.2
  * @date      20/11/2021
  *
  * @brief     HC595 driver file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 20/11/2021 Created @n
  * 01/08/2026 The driver struct is a typedef named after the module, @n
  *            the way every other module in the library declares it. @n
  *            Callers no longer write the struct keyword. @n
  * 01/08/2026 hc595DrvLoop, hc595DrvOneShoot and hc595DrvInterrupt @n
  *            lost the Drv infix and the OneShoot spelling, so the @n
  *            names match the hc595 prefix used by hc595Init. @n
  * 01/08/2026 DEF_DLY_COUNT is renamed HC595_DEF_DLY_COUNT. The old @n
  *            name was a bare macro in the global namespace. @n
  *
  ******************************************************************************
  */

#include "hc595_drv.h"

/**
 * @brief   Initializes the HC595 shift register driver and idles its pins.
 * @param[out] driver     Driver state to initialize.
 * @param[in]  dataPtr    Caller owned array shifted out on each transfer.
 * @param[in]  dataSize   Number of bytes in dataPtr.
 * @param[in]  dlyType    HC595_DLY_NO, HC595_DLY_MS or HC595_DLY_NOP.
 * @param[in]  dlyCount   Delay length, in the unit chosen by dlyType.
 * @param[in]  sckDrvFnc  Drives the shift clock pin.
 * @param[in]  rckDrvFnc  Drives the latch clock pin.
 * @param[in]  datDrvFnc  Drives the serial data pin.
 * @param[in]  dlyMsFnc   Blocks for the given number of milliseconds.
 * @param[in]  dlyNopFnc  Spins for the given number of no-op cycles.
 * @note    Drives all three output pins low before returning, so the hardware
 *          is in a known state.
 */
void hc595Init ( hc595_t* driver,
                    uint8_t* dataPtr,
                    uint32_t dataSize,
                    uint8_t dlyType,
                    uint32_t dlyCount,
                    void ( *sckDrvFnc )( uint8_t ),
                    void ( *rckDrvFnc )( uint8_t ),
                    void ( *datDrvFnc )( uint8_t ),
                    void ( *dlyMsFnc )( uint32_t ),
                    void ( *dlyNopFnc )( uint32_t ) )
{
    driver->data = dataPtr;
    driver->size = dataSize;
    driver->dlyType = dlyType;
    driver->dlyCount = dlyCount;
    driver->sckDrv = sckDrvFnc;
    driver->rckDrv = rckDrvFnc;
    driver->datDrv = datDrvFnc;
    driver->dlyMs = dlyMsFnc;
    driver->dlyNop = dlyNopFnc;

    driver->datDrv ( FALSE );
    driver->sckDrv ( FALSE );
    driver->rckDrv ( FALSE );
}

/**
 * @brief   Applies the delay configured for hc595OneShot's shift steps
 *          and the latch pulse that follows them.
 * @param[in,out] driver  Driver state.
 * @note    When dlyType holds a value other than HC595_DLY_NO, HC595_DLY_MS
 *          or HC595_DLY_NOP, this repairs it to HC595_DLY_MS and dlyCount to
 *          HC595_DEF_DLY_COUNT, without delaying on this call.
 */
static void hc595DlyCtrl ( hc595_t* driver )
{
    if ( driver->dlyType == HC595_DLY_NO )
    {
        // Intentionally blank.
    }
    else if ( driver->dlyType == HC595_DLY_NOP )
    {
        driver->dlyNop ( driver->dlyCount );
    }
    else if ( driver->dlyType == HC595_DLY_MS )
    {
        driver->dlyMs ( driver->dlyCount );
    }
    else
    {
        driver->dlyType = HC595_DLY_MS;
        driver->dlyCount = HC595_DEF_DLY_COUNT;
    }
}

/**
 * @brief   Not implemented.
 * @param[in,out] driver  Driver state.
 * @note    Reserved for a non blocking write transfer driven from the main
 *          loop. The body is empty, which is why the compiler reports
 *          driver as an unused parameter.
 */
void hc595Loop ( hc595_t* driver )
{

}

/**
 * @brief   Shifts the whole data array out least significant bit first, then
 *          pulses the latch clock.
 * @param[in,out] driver  Driver state; data is read and shifted out onto the
 *                        pins driven by sckDrv and datDrv.
 */
void hc595OneShot ( hc595_t* driver )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < driver->size; ++i )
    {
        for ( j = 0; j < 8; ++j )
        {
            driver->datDrv ( ( driver->data[ i ] >> j ) & 0x01 );
            hc595DlyCtrl ( driver );

            driver->sckDrv ( TRUE );
            hc595DlyCtrl ( driver );
            driver->sckDrv ( FALSE );
            hc595DlyCtrl ( driver );
        }
    }

    driver->rckDrv ( TRUE );
    hc595DlyCtrl ( driver );
    driver->rckDrv ( FALSE );
    hc595DlyCtrl ( driver );
}

/**
 * @brief   Not implemented.
 * @param[in,out] driver  Driver state.
 * @note    Reserved for a non blocking write transfer driven from an
 *          interrupt. The body is empty, which is why the compiler reports
 *          driver as an unused parameter.
 */
void hc595Interrupt ( hc595_t* driver )
{

}

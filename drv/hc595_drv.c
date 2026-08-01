/**
  ******************************************************************************
  *
  * @file      hc595_drv.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.1
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
  * 01/08/2026 Init reports its outcome as a uint8_t status instead of @n
  *            returning void, and validates its arguments. The @n
  *            library used three different conventions for this. @n
  * 01/08/2026 The trigger field is cleared by Init. It belongs to the @n
  *            unwritten loop mode but was left uninitialized. @n
  * 01/08/2026 The interrupt driven mode is implemented. Start arms a @n
  *            transfer, Interrupt advances it one step per call and @n
  *            takes its timing from the call period, so nothing here @n
  *            blocks or delays. A step boundary sits wherever OneShot @n
  *            delayed, which is what keeps the pin order identical. @n
  * 01/08/2026 The Loop entry point is removed. Start rejects a running @n
  *            transfer, which was the only thing a deferred arming @n
  *            step would have bought. @n
  * 01/08/2026 The trigger field is replaced by state, phase and the @n
  *            step indices, all volatile since the caller and the @n
  *            interrupt both touch them. @n
  * 01/08/2026 A delay callback is required only when dlyType selects @n
  *            it, so an interrupt driven caller passes DLY_NO and two @n
  *            NULLs. An unknown dlyType is rejected by Init instead of @n
  *            being repaired mid transfer, which is what makes that @n
  *            safe. @n
  * 02/08/2026 OneShot and the interrupt mode drive the same pins but @n
  *            nothing arbitrated between them, so calling one while @n
  *            the other ran put two bit streams on the wire with no @n
  *            error reported. A BLOCKING state now marks a running @n
  *            OneShot: Start refuses while BUSY or BLOCKING, and @n
  *            Interrupt steps only on BUSY. OneShot returns a status @n
  *            instead of void. @n
  * 02/08/2026 HC595_DEF_DLY_COUNT is removed. Its only reader was @n
  *            the branch that repaired an unknown dlyType mid @n
  *            transfer, and Init rejects that case now, so the @n
  *            macro had no reader left. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

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
 * @return  TRUE on success, FALSE when driver, dataPtr or one of the three
 *          pin callbacks is NULL, when dataSize is zero, when dlyType is not
 *          one of the three known values, or when dlyType selects a delay
 *          callback that was passed as NULL.
 * @note    Drives all three output pins low before returning, so the hardware
 *          is in a known state.
 * @note    The three pin callbacks are always required. This function calls
 *          them before it returns and the transfer routines call them without
 *          checking, so a NULL there would surface as a crash rather than a
 *          status.
 * @note    The delay callbacks are only reachable through dlyType, so each is
 *          required only when dlyType selects it. A caller that drives the
 *          chain from hc595Interrupt alone takes its timing from the interrupt
 *          period and passes HC595_DLY_NO with both delay callbacks NULL.
 * @note    An unknown dlyType is rejected here rather than repaired during a
 *          transfer, which is what makes the NULL delay callbacks safe.
 */
uint8_t hc595Init ( hc595_t* driver,
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
    uint8_t retVal = FALSE;
    uint8_t dlyOk = FALSE;

    // Only the delay callback that dlyType actually selects has to be present.
    if ( dlyType == HC595_DLY_NO )
    {
        dlyOk = TRUE;
    }
    else if ( dlyType == HC595_DLY_MS )
    {
        if ( dlyMsFnc != NULL )
        {
            dlyOk = TRUE;
        }
        else
        {
            dlyOk = FALSE;
        }
    }
    else if ( dlyType == HC595_DLY_NOP )
    {
        if ( dlyNopFnc != NULL )
        {
            dlyOk = TRUE;
        }
        else
        {
            dlyOk = FALSE;
        }
    }
    else
    {
        // Unknown dlyType.
        dlyOk = FALSE;
    }

    if ( ( driver != NULL ) && ( dataPtr != NULL ) && ( dataSize != 0 ) &&
            ( sckDrvFnc != NULL ) && ( rckDrvFnc != NULL ) && ( datDrvFnc != NULL ) &&
            ( dlyOk == TRUE ) )
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

        // Interrupt driven transfer state.
        driver->state = HC595_IDLE;
        driver->phase = HC595_PHASE_SHIFT;
        driver->byteIndex = 0;
        driver->bitIndex = 0;
        driver->stepIndex = 0;

        driver->datDrv ( FALSE );
        driver->sckDrv ( FALSE );
        driver->rckDrv ( FALSE );

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Applies the delay configured for hc595OneShot's shift steps
 *          and the latch pulse that follows them.
 * @param[in,out] driver  Driver state.
 * @note    Only the branch that dlyType selects calls a delay callback, and
 *          hc595Init rejects a dlyType outside the three known values. That
 *          is what lets a caller leave the unused delay callbacks NULL: no
 *          value of dlyType can reach a callback that was not supplied.
 */
static void hc595DlyCtrl ( hc595_t* driver )
{
    if ( driver->dlyType == HC595_DLY_NOP )
    {
        driver->dlyNop ( driver->dlyCount );
    }
    else if ( driver->dlyType == HC595_DLY_MS )
    {
        driver->dlyMs ( driver->dlyCount );
    }
    else
    {
        // HC595_DLY_NO. No delay, and no call through an absent callback.
    }
}

/**
 * @brief   Shifts the whole data array out least significant bit first, then
 *          pulses the latch clock.
 * @param[in,out] driver  Driver state; data is read and shifted out onto the
 *                        pins driven by sckDrv and datDrv.
 * @return  TRUE when the transfer ran, FALSE when driver is NULL or the other
 *          transfer mode holds the driver.
 * @note    Blocks for the whole transfer and paces itself with the delay
 *          callbacks. hc595Start with hc595Interrupt is the non blocking
 *          alternative.
 * @note    The two modes drive the same pins, so only one of them may hold a
 *          driver at a time. This claims the driver as HC595_BLOCKING before
 *          the first pin move, which makes hc595Interrupt step nothing and
 *          hc595Start refuse until the transfer ends.
 */
uint8_t hc595OneShot ( hc595_t* driver )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( ( driver != NULL ) && ( driver->state != HC595_BUSY ) &&
            ( driver->state != HC595_BLOCKING ) )
    {
        driver->state = HC595_BLOCKING;

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

        driver->state = HC595_DONE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Performs one step of the shift phase.
 * @param[in,out] driver  Driver state.
 * @note    Three steps per bit, matching the three points where hc595OneShot
 *          delays. Advances to the latch phase after the last bit of the last
 *          byte.
 */
static void hc595ShiftStep ( hc595_t* driver )
{
    if ( driver->stepIndex == 0 )
    {
        driver->datDrv ( ( driver->data[ driver->byteIndex ] >> driver->bitIndex ) & 0x01 );
        driver->stepIndex = 1;
    }
    else if ( driver->stepIndex == 1 )
    {
        driver->sckDrv ( TRUE );
        driver->stepIndex = 2;
    }
    else
    {
        driver->sckDrv ( FALSE );
        driver->stepIndex = 0;

        ++driver->bitIndex;

        if ( driver->bitIndex >= 8 )
        {
            driver->bitIndex = 0;
            ++driver->byteIndex;

            if ( driver->byteIndex >= driver->size )
            {
                driver->phase = HC595_PHASE_LATCH;
            }
            else
            {
                /* Intentionally blank. */
            }
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}

/**
 * @brief   Performs one step of the latch phase.
 * @param[in,out] driver  Driver state.
 * @note    Two steps, matching the two points where hc595OneShot delays around
 *          the latch pulse. Moves the driver to HC595_DONE on the second.
 */
static void hc595LatchStep ( hc595_t* driver )
{
    if ( driver->stepIndex == 0 )
    {
        driver->rckDrv ( TRUE );
        driver->stepIndex = 1;
    }
    else
    {
        driver->rckDrv ( FALSE );
        driver->stepIndex = 0;
        driver->state = HC595_DONE;
    }
}

/**
 * @brief   Requests a non blocking transfer of the whole data array.
 * @param[in,out] driver  Driver state.
 * @return  TRUE when the transfer was armed, FALSE when driver is NULL, a
 *          transfer is already running, or hc595OneShot holds the driver.
 * @note    A running transfer is never interrupted and never queued behind.
 *          Poll hc595GetState and call again once it reports HC595_DONE.
 * @note    Writes state last, on purpose. Until that write lands
 *          hc595Interrupt sees a state other than HC595_BUSY and returns
 *          without reading any of the indices set above it, so this is safe to
 *          call from any context, including another interrupt.
 * @note    Refuses while the driver is HC595_BLOCKING. Arming a stepped
 *          transfer underneath a running hc595OneShot would put two bit
 *          streams on the same pins.
 */
uint8_t hc595Start ( hc595_t* driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( driver->state != HC595_BUSY ) &&
            ( driver->state != HC595_BLOCKING ) )
    {
        driver->phase = HC595_PHASE_SHIFT;
        driver->byteIndex = 0;
        driver->bitIndex = 0;
        driver->stepIndex = 0;

        driver->state = HC595_BUSY;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Advances a running transfer by one step. Call at a fixed rate.
 * @param[in,out] driver  Driver state.
 * @note    This is the non blocking alternative to hc595OneShot. It never
 *          delays and never loops: the interrupt period supplies the timing
 *          that hc595OneShot gets from its delay callbacks, so the shift clock
 *          runs at a third of the call rate.
 * @note    A full transfer takes ( 24 * size ) + 2 calls.
 * @note    Costs one state read and a comparison when no transfer is running.
 */
void hc595Interrupt ( hc595_t* driver )
{
    if ( driver->state == HC595_BUSY )
    {
        if ( driver->phase == HC595_PHASE_SHIFT )
        {
            hc595ShiftStep ( driver );
        }
        else
        {
            hc595LatchStep ( driver );
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Gets the state of the interrupt driven transfer.
 * @param[in] driver  Driver state.
 * @return  HC595_IDLE before the first transfer, HC595_BUSY while a stepped
 *          one is running, HC595_BLOCKING while hc595OneShot holds the driver,
 *          HC595_DONE once either has finished.
 * @note    HC595_DONE stands until the next hc595Start. Clearing it here would
 *          let a caller that polls one pass late miss the completion outright.
 */
uint8_t hc595GetState ( const hc595_t* const driver )
{
    return ( driver->state );
}

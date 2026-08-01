/**
  ******************************************************************************
  *
  * @file      hc597_drv.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.1
  * @date      23/05/2022
  *
  * @brief     HC597 driver file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 23/05/2022 Created @n
  * 29/07/2026 Bug fix. hc597OneShot only filled data[ 0 ] and @n
  *            ignored driver->size, so a chain longer than one @n
  *            device could not be read. @n
  * 29/07/2026 hc597Init drives the clock and load pins to a known @n
  *            state, the way hc595Init already does. @n
  * 01/08/2026 The driver struct is a typedef named after the module, @n
  *            the way every other module in the library declares it. @n
  *            Callers no longer write the struct keyword. @n
  * 01/08/2026 hc597DrvLoop, hc597DrvOneShoot and hc597DrvInterrupt @n
  *            lost the Drv infix and the OneShoot spelling, so the @n
  *            names match the hc597 prefix used by hc597Init. @n
  * 01/08/2026 DEF_DLY_COUNT is renamed HC597_DEF_DLY_COUNT. The old @n
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
  * 02/08/2026 HC597_DEF_DLY_COUNT is removed. Its only reader was @n
  *            the branch that repaired an unknown dlyType mid @n
  *            transfer, and Init rejects that case now, so the @n
  *            macro had no reader left. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

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
 * @return  TRUE on success, FALSE when driver, dataPtr or one of the three
 *          pin callbacks is NULL, when dataSize is zero, when dlyType is not
 *          one of the three known values, or when dlyType selects a delay
 *          callback that was passed as NULL.
 * @note    Drives the clock pin low and the load pin high before returning.
 *          The load pulse is active low, so this leaves the pins idle.
 * @note    The three pin callbacks are always required. This function calls
 *          two of them before it returns and the transfer routines call them
 *          without checking, so a NULL there would surface as a crash rather
 *          than a status.
 * @note    The delay callbacks are only reachable through dlyType, so each is
 *          required only when dlyType selects it. A caller that drives the
 *          chain from hc597Interrupt alone takes its timing from the interrupt
 *          period and passes HC597_DLY_NO with both delay callbacks NULL.
 * @note    An unknown dlyType is rejected here rather than repaired during a
 *          transfer, which is what makes the NULL delay callbacks safe.
 */
uint8_t hc597Init ( hc597_t* driver,
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
    uint8_t retVal = FALSE;
    uint8_t dlyOk = FALSE;

    // Only the delay callback that dlyType actually selects has to be present.
    if ( dlyType == HC597_DLY_NO )
    {
        dlyOk = TRUE;
    }
    else if ( dlyType == HC597_DLY_MS )
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
    else if ( dlyType == HC597_DLY_NOP )
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
            ( clkDrvFnc != NULL ) && ( lodDrvFnc != NULL ) && ( datDrvFnc != NULL ) &&
            ( dlyOk == TRUE ) )
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

        // Interrupt driven transfer state.
        driver->state = HC597_IDLE;
        driver->phase = HC597_PHASE_PROLOGUE;
        driver->byteIndex = 0;
        driver->bitIndex = 0;

        // Idle state. The load pulse is active low, so it idles high.
        driver->clkDrv ( FALSE );
        driver->lodDrv ( TRUE );

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Applies the delay configured for hc597OneShot's clock steps.
 * @param[in,out] driver  Driver state.
 * @note    Only the branch that dlyType selects calls a delay callback, and
 *          hc597Init rejects a dlyType outside the three known values. That
 *          is what lets a caller leave the unused delay callbacks NULL: no
 *          value of dlyType can reach a callback that was not supplied.
 */
static void hc597DlyCtrl ( hc597_t* driver )
{
    if ( driver->dlyType == HC597_DLY_NOP )
    {
        driver->dlyNop ( driver->dlyCount );
    }
    else if ( driver->dlyType == HC597_DLY_MS )
    {
        driver->dlyMs ( driver->dlyCount );
    }
    else
    {
        // HC597_DLY_NO. No delay, and no call through an absent callback.
    }
}

/**
 * @brief   Latches the parallel inputs once, then clocks driver->size bytes
 *          in least significant bit first, filling driver->data.
 * @param[in,out] driver  Driver state; data is written with the bytes read
 *                        from datDrv, clocked by clkDrv.
 * @return  TRUE when the transfer ran, FALSE when driver is NULL or the other
 *          transfer mode holds the driver.
 * @note    Blocks for the whole transfer and paces itself with the delay
 *          callback. hc597Start with hc597Interrupt is the non blocking
 *          alternative.
 * @note    The two modes drive the same pins, so only one of them may hold a
 *          driver at a time. This claims the driver as HC597_BLOCKING before
 *          the first pin move, which makes hc597Interrupt step nothing and
 *          hc597Start refuse until the transfer ends.
 */
uint8_t hc597OneShot ( hc597_t* driver )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( ( driver != NULL ) && ( driver->state != HC597_BUSY ) &&
            ( driver->state != HC597_BLOCKING ) )
    {
        driver->state = HC597_BLOCKING;

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

        driver->state = HC597_DONE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Performs the prologue step that latches the parallel inputs.
 * @param[in,out] driver  Driver state.
 * @note    One step for the whole prologue, because hc597OneShot takes no
 *          delay anywhere inside it.
 */
static void hc597PrologueStep ( hc597_t* driver )
{
    driver->clkDrv ( TRUE );
    driver->clkDrv ( FALSE );

    driver->lodDrv ( FALSE );
    driver->lodDrv ( TRUE );

    driver->phase = HC597_PHASE_SHIFT;
}

/**
 * @brief   Performs one step of the shift phase, reading a single bit.
 * @param[in,out] driver  Driver state.
 * @note    One step per bit, matching the single delay hc597OneShot takes per
 *          bit. The clock pulse stays inside the step because hc597OneShot
 *          does not delay between its two edges either.
 * @note    Moves the driver to HC597_DONE after the last bit of the last byte.
 */
static void hc597ShiftStep ( hc597_t* driver )
{
    if ( driver->bitIndex == 0 )
    {
        driver->data[ driver->byteIndex ] = 0;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Any non zero read is one bit, so it is normalized here.
    if ( driver->datDrv ( ) != FALSE )
    {
        driver->data[ driver->byteIndex ] |= ( uint8_t ) ( 1u << driver->bitIndex );
    }
    else
    {
        /* Intentionally blank. */
    }

    driver->clkDrv ( TRUE );
    driver->clkDrv ( FALSE );

    ++driver->bitIndex;

    if ( driver->bitIndex >= 8 )
    {
        driver->bitIndex = 0;
        ++driver->byteIndex;

        if ( driver->byteIndex >= driver->size )
        {
            driver->state = HC597_DONE;
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

/**
 * @brief   Requests a non blocking read of the whole chain into the data array.
 * @param[in,out] driver  Driver state.
 * @return  TRUE when the transfer was armed, FALSE when driver is NULL, a
 *          transfer is already running, or hc597OneShot holds the driver.
 * @note    A running transfer is never interrupted and never queued behind.
 *          Poll hc597GetState and call again once it reports HC597_DONE.
 * @note    Writes state last, on purpose. Until that write lands
 *          hc597Interrupt sees a state other than HC597_BUSY and returns
 *          without reading any of the indices set above it, so this is safe to
 *          call from any context, including another interrupt.
 * @note    Refuses while the driver is HC597_BLOCKING. Arming a stepped
 *          transfer underneath a running hc597OneShot would clock the same
 *          chain from two places into the same buffer.
 */
uint8_t hc597Start ( hc597_t* driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( driver->state != HC597_BUSY ) &&
            ( driver->state != HC597_BLOCKING ) )
    {
        driver->phase = HC597_PHASE_PROLOGUE;
        driver->byteIndex = 0;
        driver->bitIndex = 0;

        driver->state = HC597_BUSY;

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
 * @note    This is the non blocking alternative to hc597OneShot. It never
 *          delays and never loops: the interrupt period supplies the timing
 *          that hc597OneShot gets from its delay callback.
 * @note    A full transfer takes ( 8 * size ) + 1 calls.
 * @note    The data array is written across the whole transfer. Read it only
 *          once hc597GetState reports HC597_DONE.
 * @note    Costs one state read and a comparison when no transfer is running.
 */
void hc597Interrupt ( hc597_t* driver )
{
    if ( driver->state == HC597_BUSY )
    {
        if ( driver->phase == HC597_PHASE_PROLOGUE )
        {
            hc597PrologueStep ( driver );
        }
        else
        {
            hc597ShiftStep ( driver );
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
 * @return  HC597_IDLE before the first transfer, HC597_BUSY while a stepped
 *          one is running, HC597_BLOCKING while hc597OneShot holds the driver,
 *          HC597_DONE once either has finished and the data array holds the
 *          values read from the chain.
 * @note    HC597_DONE stands until the next hc597Start. Clearing it here would
 *          let a caller that polls one pass late miss the completion outright.
 */
uint8_t hc597GetState ( const hc597_t* const driver )
{
    return ( driver->state );
}

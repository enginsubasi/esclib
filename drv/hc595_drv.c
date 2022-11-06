/**
  ******************************************************************************
  *
  * @file:      hc595.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     20/11/2021
  * @history:   20/11/2021 Created
  *
  * @about:     HC595 driver file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         hc595Init           :
  *         hc595DlyCtrl        :
  *         hc595DrvLoop        :
  *         hc595DrvOneShoot    :
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "hc595_drv.h"

/*
 * @about:
 */
void hc595Init ( struct HC595_Driver *driver,
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

/*
 * @about:
 */
static void hc595DlyCtrl ( struct HC595_Driver *driver )
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
        driver->dlyCount = DEF_DLY_COUNT;
    }
}

/*
 * @about:
 */
void hc595DrvLoop ( struct HC595_Driver *driver )
{

}

/*
 * @about:
 */
void hc595DrvOneShoot ( struct HC595_Driver *driver )
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

/*
 * @about:
 */
void hc595DrvInterrupt ( struct HC595_Driver *driver )
{

}

/**
  ******************************************************************************
  *
  * @file:      hc597.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     23/05/2022
  * @history:   23/05/2022 Created
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
 * @about:
 */
void hc597Init ( struct HC597_Driver *driver,
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
    driver->rckDrv ( FALSE);

}

/*
 * @about:
 */
static void hc595DlyCtrl ( struct HC597_Driver *driver )
{
    if ( driver->dlyType == DLY_NO )
    {
        // Intentionally blank.
    }
    else if ( driver->dlyType == DLY_NOP )
    {
        driver->dlyNop ( driver->dlyCount );
    }
    else if ( driver->dlyType == DLY_MS )
    {
        driver->dlyMs ( driver->dlyCount );
    }
    else
    {
        driver->dlyType = DLY_MS;
        driver->dlyCount = DEF_DLY_COUNT;
    }
}

/*
 * @about:
 */
void hc597DrvLoop ( struct HC597_Driver *driver )
{

}

/*
 * @about:
 */
void hc597DrvOneShoot ( struct HC597_Driver *driver )
{

}

/*
 * @about:
 */
void hc597DrvInterrupt ( struct HC595_Driver *driver )
{

}

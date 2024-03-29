#ifndef INC_HC595_DRV_H_
#define INC_HC595_DRV_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define DEF_DLY_COUNT   1 // In millisecond.

/* TYPEDEFS */

/* STRUCTURES */


struct HC595_Driver
{
    uint8_t trigger;

    uint8_t* data;
    uint32_t size;

    uint8_t dlyType;
    uint32_t dlyCount;

    void ( *sckDrv )( uint8_t );
    void ( *rckDrv )( uint8_t );
    void ( *datDrv )( uint8_t );

    void ( *dlyMs )( uint32_t );
    void ( *dlyNop )( uint32_t );
};

/* ENUMS */

enum HC595_DLY_TYPE
{
    HC595_DLY_NO        = 0,
    HC595_DLY_MS        = 1,
    HC595_DLY_NOP       = 2
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */
void hc595Init ( struct HC595_Driver *driver,
                    uint8_t* dataPtr,
                    uint32_t dataSize,
                    uint8_t dlyType,
                    uint32_t dlyCount,
                    void ( *sckDrvFnc )( uint8_t ),
                    void ( *rckDrvFnc )( uint8_t ),
                    void ( *datDrvFnc )( uint8_t ),
                    void ( *dlyMsFnc )( uint32_t ),
                    void ( *dlyNopFnc )( uint32_t ) );
void hc595DrvLoop ( struct HC595_Driver *driver );
void hc595DrvOneShoot ( struct HC595_Driver *driver );
void hc595DrvInterrupt ( struct HC595_Driver *driver );

#ifdef __cplusplus
}
#endif

#endif /* INC_HC595_DRV_H_ */

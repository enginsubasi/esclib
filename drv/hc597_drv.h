#ifndef INC_HC597_DRV_H_
#define INC_HC597_DRV_H_

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


struct HC597_Driver
{
    uint8_t trigger;

    uint8_t* data;
    uint32_t size;

    uint8_t dlyType;
    uint32_t dlyCount;

    void ( *clkDrv )( uint8_t );
    void ( *lodDrv )( uint8_t );
    uint8_t ( *datDrv )( void );

    void ( *dlyMs )( uint32_t );
    void ( *dlyNop )( uint32_t );
};

/* ENUMS */

enum HC597_DLY_TYPE
{
    HC597_DLY_NO        = 0,
    HC597_DLY_MS        = 1,
    HC597_DLY_NOP       = 2
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */
void hc597Init ( struct HC597_Driver* driver,
                    uint8_t* dataPtr,
                    uint32_t dataSize,
                    uint8_t dlyType,
                    uint32_t dlyCount,
                    void ( *clkDrvFnc )( uint8_t ),
                    void ( *lodDrvFnc )( uint8_t ),
                    uint8_t ( *datDrvFnc )( void ),
                    void ( *dlyMsFnc )( uint32_t ),
                    void ( *dlyNopFnc )( uint32_t ) );
void hc597DrvLoop ( struct HC597_Driver *driver );
void hc597DrvOneShoot ( struct HC597_Driver *driver );
void hc597DrvInterrupt ( struct HC597_Driver *driver );

#ifdef __cplusplus
}
#endif

#endif /* INC_HC597_DRV_H_ */

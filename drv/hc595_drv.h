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

#define HC595_DEF_DLY_COUNT   1 // In millisecond.

/* TYPEDEFS */

/* STRUCTURES */


typedef struct
{
    uint8_t* data;
    uint32_t size;

    uint8_t dlyType;
    uint32_t dlyCount;

    /*
     * Shared between hc595Start on the caller side and hc595Interrupt on the
     * interrupt side. Declared volatile so the writes stay ordered with
     * respect to each other and neither side caches state in a register.
     */
    volatile uint8_t state;
    volatile uint8_t phase;
    volatile uint32_t byteIndex;
    volatile uint8_t bitIndex;
    volatile uint8_t stepIndex;

    void ( *sckDrv )( uint8_t );
    void ( *rckDrv )( uint8_t );
    void ( *datDrv )( uint8_t );

    void ( *dlyMs )( uint32_t );
    void ( *dlyNop )( uint32_t );
} hc595_t;

/* ENUMS */

enum HC595_DLY_TYPE
{
    HC595_DLY_NO        = 0,
    HC595_DLY_MS        = 1,
    HC595_DLY_NOP       = 2
};

enum HC595_STATE
{
    HC595_IDLE          = 0,
    HC595_BUSY          = 1,
    HC595_DONE          = 2,
    HC595_BLOCKING      = 3
};

enum HC595_PHASE
{
    HC595_PHASE_SHIFT   = 0,
    HC595_PHASE_LATCH   = 1
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */
uint8_t hc595Init ( hc595_t *driver,
                    uint8_t* dataPtr,
                    uint32_t dataSize,
                    uint8_t dlyType,
                    uint32_t dlyCount,
                    void ( *sckDrvFnc )( uint8_t ),
                    void ( *rckDrvFnc )( uint8_t ),
                    void ( *datDrvFnc )( uint8_t ),
                    void ( *dlyMsFnc )( uint32_t ),
                    void ( *dlyNopFnc )( uint32_t ) );
uint8_t hc595OneShot ( hc595_t *driver );
uint8_t hc595Start ( hc595_t *driver );
void hc595Interrupt ( hc595_t *driver );
uint8_t hc595GetState ( const hc595_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* INC_HC595_DRV_H_ */

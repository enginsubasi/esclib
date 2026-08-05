#ifndef COMSTXETX_H_
#define COMSTXETX_H_

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

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint8_t stx;
    uint8_t etx;
    uint8_t dle;

    volatile uint32_t rxTimeoutCounter;
    uint32_t rxTimeout;

    volatile uint32_t rxIndex;
    uint32_t rxSize;
    uint32_t txSize;

    /*
     * Shared between comstxetxReceive on the interrupt side and the caller
     * side. Declared volatile so the writes stay ordered with respect to each
     * other and neither side caches state in a register.
     */
    volatile uint8_t rxFrameOpen;
    volatile uint8_t rxEscape;
    volatile uint8_t rxReadyToEvaluate;

    uint32_t rxRejectCount;

    uint8_t *rxBuffer;

    uint8_t *txBuffer;

    uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length );
    void (*packetProcess) ( uint8_t* buffer, uint32_t index );
} comstxetx_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t comstxetxInit ( comstxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                        uint32_t rxSize, uint32_t txSize,
                        uint8_t stx, uint8_t etx, uint8_t dle,
                        uint32_t rxTimeout,
                        uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ),
                        void ( *packetProcess ) ( uint8_t* buffer, uint32_t index ) );
void comstxetxReceive ( comstxetx_t* driver, uint8_t data );
void comstxetxEvaluate ( comstxetx_t* driver );
void comstxetxTimeoutCounter ( comstxetx_t* driver );
uint8_t comstxetxBuildFrame ( comstxetx_t* driver, const uint8_t* const payload,
                              uint32_t length, uint32_t* frameLength );
uint32_t comstxetxGetRejectCount ( const comstxetx_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* COMSTXETX_H_ */

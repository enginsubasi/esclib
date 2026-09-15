#ifndef COMMODBUS_H_
#define COMMODBUS_H_

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

/* The address every device answers and no device owns. */
#define COMMODBUS_BROADCAST     0u

/* The addresses a device may be given. Above this the range is reserved. */
#define COMMODBUS_ADDRESS_MIN   1u
#define COMMODBUS_ADDRESS_MAX   247u

/* Address, function and the two check bytes: the shortest frame there is. */
#define COMMODBUS_MIN_FRAME     4u

/* What the specification allows a whole frame to reach. */
#define COMMODBUS_MAX_FRAME     256u

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint8_t address;

    volatile uint32_t silenceCounter;
    uint32_t silenceTicks;

    volatile uint32_t rxIndex;
    uint32_t rxSize;
    uint32_t txSize;

    /*
     * Shared between commodbusReceive on the interrupt side and the caller
     * side. Declared volatile so the writes stay ordered with respect to each
     * other and neither side caches state in a register.
     */
    volatile uint8_t rxReadyToEvaluate;
    volatile uint8_t rxOverrun;
    volatile uint8_t rxDropped;

    uint32_t rxRejectCount;
    uint32_t rxIgnoredCount;

    uint8_t* rxBuffer;

    uint8_t* txBuffer;

    uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length );
    void ( *packetProcess ) ( uint8_t* buffer, uint32_t length );
} commodbus_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t commodbusInit ( commodbus_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                        uint32_t rxSize, uint32_t txSize,
                        uint8_t address, uint32_t silenceTicks,
                        uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ),
                        void ( *packetProcess ) ( uint8_t* buffer, uint32_t length ) );
void commodbusReceive ( commodbus_t* driver, uint8_t data );
void commodbusTimeoutCounter ( commodbus_t* driver );
void commodbusEvaluate ( commodbus_t* driver );
uint8_t commodbusBuildFrame ( commodbus_t* driver, uint8_t address,
                              const uint8_t* const pdu, uint32_t length,
                              uint32_t* frameLength );
uint32_t commodbusGetRejectCount ( const commodbus_t* const driver );
uint32_t commodbusGetIgnoredCount ( const commodbus_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* COMMODBUS_H_ */

#ifndef COMSAFE_H_
#define COMSAFE_H_

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

/* Connection id, sequence and check bytes wrapped around every payload. */
#define COMSAFE_OVERHEAD    5u

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint8_t* rxBuffer;
    uint8_t* txBuffer;
    uint32_t rxSize;
    uint32_t txSize;

    uint32_t rxPayloadLength;

    uint32_t timeoutCounter;
    uint32_t timeout;

    uint32_t errorCount;

    uint16_t connectionId;

    uint8_t txSequence;
    uint8_t rxSequence;
    uint8_t rxArmed;

    uint8_t state;
    uint8_t lastError;

    uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length );
} comsafe_t;

/* ENUMS */

enum COMSAFE_STATE
{
    CS_INIT     = 0,
    CS_OK       = 1,
    CS_FAILED   = 2
};

enum COMSAFE_ERROR
{
    CS_NONE     = 0,
    CS_LENGTH   = 1,
    CS_CHECK    = 2,
    CS_ID       = 3,
    CS_SEQUENCE = 4,
    CS_TIMEOUT  = 5
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t comsafeInit ( comsafe_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                      uint32_t rxSize, uint32_t txSize,
                      uint16_t connectionId, uint32_t timeout,
                      uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ) );
uint8_t comsafeBuildFrame ( comsafe_t* driver, const uint8_t* const payload,
                            uint32_t length, uint32_t* frameLength );
uint8_t comsafeCheckFrame ( comsafe_t* driver, const uint8_t* const frame, uint32_t frameLength );
const uint8_t* comsafeGetPayload ( const comsafe_t* const driver );
uint32_t comsafeGetPayloadLength ( const comsafe_t* const driver );
void comsafeTimeoutCounter ( comsafe_t* driver );
uint8_t comsafeGetState ( const comsafe_t* const driver );
uint8_t comsafeGetLastError ( const comsafe_t* const driver );
uint32_t comsafeGetErrorCount ( const comsafe_t* const driver );
void comsafeReset ( comsafe_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* COMSAFE_H_ */

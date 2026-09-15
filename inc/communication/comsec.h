#ifndef COMSEC_H_
#define COMSEC_H_

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

/* Longest tag this module will carry. */
#define COMSEC_MAX_TAG      16u

/* Shortest tag it will accept. */
#define COMSEC_MIN_TAG      4u

/* Session id and counter in front of every payload. */
#define COMSEC_HEADER       6u

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint8_t* rxBuffer;
    uint8_t* txBuffer;
    uint32_t rxSize;
    uint32_t txSize;

    uint32_t rxPayloadLength;

    uint32_t txCounter;
    uint32_t rxCounter;

    uint32_t rejectCount;

    uint16_t sessionId;

    uint8_t tagSize;
    uint8_t rxArmed;
    uint8_t lastError;

    void ( *mac ) ( const uint8_t* const buffer, uint32_t length, uint8_t* tag );
} comsec_t;

/* ENUMS */

enum COMSEC_ERROR
{
    CSEC_NONE       = 0,
    CSEC_LENGTH     = 1,
    CSEC_TAG        = 2,
    CSEC_SESSION    = 3,
    CSEC_REPLAY     = 4,
    CSEC_EXHAUSTED  = 5
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t comsecInit ( comsec_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                     uint32_t rxSize, uint32_t txSize,
                     uint16_t sessionId, uint8_t tagSize,
                     void ( *mac ) ( const uint8_t* const buffer, uint32_t length, uint8_t* tag ) );
uint8_t comsecBuildFrame ( comsec_t* driver, const uint8_t* const payload,
                           uint32_t length, uint32_t* frameLength );
uint8_t comsecCheckFrame ( comsec_t* driver, const uint8_t* const frame, uint32_t frameLength );
const uint8_t* comsecGetPayload ( const comsec_t* const driver );
uint32_t comsecGetPayloadLength ( const comsec_t* const driver );
uint32_t comsecGetCounter ( const comsec_t* const driver );
uint8_t comsecGetLastError ( const comsec_t* const driver );
uint32_t comsecGetRejectCount ( const comsec_t* const driver );
uint8_t comsecRekey ( comsec_t* driver, uint16_t sessionId, uint32_t txCounter );

#ifdef __cplusplus
}
#endif

#endif /* COMSEC_H_ */

#ifndef ENCODER_H_
#define ENCODER_H_

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
    /*
     * Shared between encoderUpdate on the interrupt side and the caller side.
     * Declared volatile so the writes stay ordered with respect to each other
     * and neither side caches state in a register.
     */
    volatile uint8_t state;
    volatile int32_t position;
    volatile int8_t direction;
    volatile uint32_t errorCount;
} encoder_t;

/* ENUMS */

enum ENCODER_DIRECTION
{
    ENC_STOPPED         = 0,
    ENC_FORWARD         = 1,
    ENC_BACKWARD        = -1
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t encoderInit ( encoder_t* driver, uint8_t channelA, uint8_t channelB );
void encoderUpdate ( encoder_t* driver, uint8_t channelA, uint8_t channelB );
int32_t encoderGetPosition ( const encoder_t* const driver );
void encoderSetPosition ( encoder_t* driver, int32_t position );
int8_t encoderGetDirection ( const encoder_t* const driver );
uint32_t encoderGetErrorCount ( const encoder_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_H_ */

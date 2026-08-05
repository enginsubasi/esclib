#ifndef RAMP_H_
#define RAMP_H_

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
    float maxVelocity;
    float maxAcceleration;
    float ts;

    float position;
    float velocity;

    uint8_t arrived;
} ramp_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t rampInit ( ramp_t* driver, float maxVelocity, float maxAcceleration, float ts, float positionInit );
void rampIteration ( ramp_t* driver, float target );
float rampGetOutput ( const ramp_t* const driver );
float rampGetVelocity ( const ramp_t* const driver );
uint8_t rampIsArrived ( const ramp_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* RAMP_H_ */

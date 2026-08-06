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

typedef struct
{
    int64_t maxVelocity;
    int64_t maxAcceleration;

    int64_t position;
    int64_t velocity;

    uint8_t arrived;
} rampi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t rampInit ( ramp_t* driver, float maxVelocity, float maxAcceleration, float ts, float positionInit );
void rampIteration ( ramp_t* driver, float target );
float rampGetOutput ( const ramp_t* const driver );
float rampGetVelocity ( const ramp_t* const driver );
uint8_t rampIsArrived ( const ramp_t* const driver );

uint8_t rampIniti32 ( rampi32_t* driver, int32_t maxVelocity, int32_t maxAcceleration, int32_t positionInit );
void rampIterationi32 ( rampi32_t* driver, int32_t target );
int32_t rampGetOutputi32 ( const rampi32_t* const driver );
int32_t rampGetVelocityi32 ( const rampi32_t* const driver );
uint8_t rampIsArrivedi32 ( const rampi32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* RAMP_H_ */

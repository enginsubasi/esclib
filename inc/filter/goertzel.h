#ifndef GOERTZEL_H_
#define GOERTZEL_H_

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
    float coeff;
    float scale;
    float s1;
    float s2;
    float power;
    uint32_t blockLength;
    uint32_t count;
    uint8_t ready;
} goertzel_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t goertzelInit ( goertzel_t* driver, float sampleRate, float frequency, uint32_t blockLength );
void goertzelIteration ( goertzel_t* driver, float newData );
uint8_t goertzelIsReady ( goertzel_t* driver );
float goertzelGetPower ( const goertzel_t* const driver );
float goertzelGetMagnitude ( const goertzel_t* const driver );
void goertzelReset ( goertzel_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* GOERTZEL_H_ */

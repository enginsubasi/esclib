#ifndef BIQUAD_H_
#define BIQUAD_H_

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
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
    float s1;
    float s2;
    float output;
} biquad_t;

typedef struct
{
    int32_t b0;
    int32_t b1;
    int32_t b2;
    int32_t a1;
    int32_t a2;
    int64_t s1;
    int64_t s2;
    int32_t output;
} biquadi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t biquadInit ( biquad_t* driver, float b0, float b1, float b2, float a1, float a2 );
uint8_t biquadInitLowPass ( biquad_t* driver, float sampleRate, float cutoff, float q );
uint8_t biquadInitHighPass ( biquad_t* driver, float sampleRate, float cutoff, float q );
uint8_t biquadInitBandPass ( biquad_t* driver, float sampleRate, float centre, float q );
uint8_t biquadInitNotch ( biquad_t* driver, float sampleRate, float centre, float q );
void biquadIteration ( biquad_t* driver, float newData );
float biquadGetOutput ( const biquad_t* const driver );
void biquadReset ( biquad_t* driver, float inputInit );

uint8_t biquadIniti32 ( biquadi32_t* driver, int32_t b0, int32_t b1, int32_t b2, int32_t a1, int32_t a2 );
void biquadIterationi32 ( biquadi32_t* driver, int32_t newData );
int32_t biquadGetOutputi32 ( const biquadi32_t* const driver );
void biquadReseti32 ( biquadi32_t* driver, int32_t inputInit );

#ifdef __cplusplus
}
#endif

#endif /* BIQUAD_H_ */

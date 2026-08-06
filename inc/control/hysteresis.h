#ifndef HYSTERESIS_H_
#define HYSTERESIS_H_

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
    uint8_t output;
    float up;
    float dw;
} hysteresis_t;

typedef struct
{
    uint8_t output;
    int32_t up;
    int32_t dw;
} hysteresisi32_t;

typedef struct
{
    uint8_t output;
    uint32_t up;
    uint32_t dw;
} hysteresisu32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t hysteresisInit ( hysteresis_t* driver, float upValue, float downValue );
void hysteresisControl ( hysteresis_t* driver, float input );
uint8_t hysteresisGetOutput ( const hysteresis_t* const driver );

uint8_t hysteresisIniti32 ( hysteresisi32_t* driver, int32_t upValue, int32_t downValue );
void hysteresisControli32 ( hysteresisi32_t* driver, int32_t input );
uint8_t hysteresisGetOutputi32 ( const hysteresisi32_t* const driver );

uint8_t hysteresisInitu32 ( hysteresisu32_t* driver, uint32_t upValue, uint32_t downValue );
void hysteresisControlu32 ( hysteresisu32_t* driver, uint32_t input );
uint8_t hysteresisGetOutputu32 ( const hysteresisu32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* HYSTERESIS_H_ */

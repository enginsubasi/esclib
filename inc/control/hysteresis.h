#ifndef HYSTERESIS_H_
#define HYSTERESIS_H_

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

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void hysteresisInit ( hysteresis_t* driver, float upValue, float downValue );
void hysteresisControl ( hysteresis_t* driver, float input );
uint8_t hysteresisGetOutput ( hysteresis_t* driver );

#endif /* HYSTERESIS_H_ */

#ifndef HYSTERESIS_H_
#define HYSTERESIS_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#define TRUE 1
#define FALSE 0

/* STRUCTURES */

typedef struct
{
    uint8_t output;
    double up;
    double dw;
} hysteresis_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t hysteresisInit ( hysteresis_t* driver, double upValue, double downValue );
void hysteresisControl ( hysteresis_t* driver, double input );
uint8_t hysteresisGetOutput ( hysteresis_t* driver );

#endif /* HYSTERESIS_H_ */

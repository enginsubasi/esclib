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
    double output;
    double kp;
    double ki;
    double kd;
} pid_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t pidInit ( hysteresis_t* driver, double upValue, double downValue );
void pidControl ( hysteresis_t* driver, double input );
uint8_t pidGetOutput ( hysteresis_t* driver );

#endif /* HYSTERESIS_H_ */

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
    double error;
    double lastError;
    
    double output;
    
    double kp;
    double ki;
    double kd;
    
    double iMax;
    double iMin;
    
    double partP;
    double partI;
    double partD;
    
    double pidMax;
    double pidMin;
} pid_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t pidInit ( pid_t* driver, double upValue, double downValue );
void pidControl ( pid_t* driver, double input );
uint8_t pidGetOutput ( pid_t* driver );

#endif /* HYSTERESIS_H_ */

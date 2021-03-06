#ifndef PID_H_
#define PID_H_

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
    float error;
    float lastError;

    float output;

    float kp;
    float ki;
    float kd;

    float iMax;
    float iMin;

    float partP;
    float partI;
    float partD;

    float pidMax;
    float pidMin;
} pidc_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void pidInit ( pidc_t* driver, float kp, float ki, float kd, float iPartMaxLimit, float iPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit );
void pidChangeCoefficients ( pidc_t* driver, float kp, float ki, float kd );
void pidControl ( pidc_t* driver, float input );
float pidGetOutput ( pidc_t* driver );

#endif /* PID_H_ */

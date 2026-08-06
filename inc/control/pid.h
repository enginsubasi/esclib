#ifndef PID_H_
#define PID_H_

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
    float error;
    float lastError;

    float output;

    float kp;
    float ki;
    float kd;

    float ts;

    float pMax;
    float pMin;

    float iMax;
    float iMin;
    
    float dMax;
    float dMin;

    float partP;
    float partI;
    float partD;

    float pidMax;
    float pidMin;
} pidc_t;

typedef struct
{
    int32_t error;
    int32_t lastError;

    int32_t output;

    int32_t kp;
    int32_t ki;
    int32_t kd;

    int32_t pMax;
    int32_t pMin;

    int32_t iMax;
    int32_t iMin;

    int32_t dMax;
    int32_t dMin;

    int32_t partP;
    int64_t partI;
    int32_t partD;

    int32_t pidMax;
    int32_t pidMin;
} pidci32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t pidInit ( pidc_t* driver, float kp, float ki, float kd, float ts, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit );
uint8_t pidChangeCoefficients ( pidc_t* driver, float kp, float ki, float kd, float ts );
uint8_t pidChangeLimits ( pidc_t* driver, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                        float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit );
void pidControl ( pidc_t* driver, float error );
float pidGetOutput ( const pidc_t* const driver );

uint8_t pidIniti32 ( pidci32_t* driver, int32_t kp, int32_t ki, int32_t kd, int32_t pPartMaxLimit, int32_t pPartMinLimit, int32_t iPartMaxLimit, int32_t iPartMinLimit,
                int32_t dPartMaxLimit, int32_t dPartMinLimit, int32_t pidOutputMaxLimit, int32_t pidOutputMinLimit );
uint8_t pidChangeCoefficientsi32 ( pidci32_t* driver, int32_t kp, int32_t ki, int32_t kd );
uint8_t pidChangeLimitsi32 ( pidci32_t* driver, int32_t pPartMaxLimit, int32_t pPartMinLimit, int32_t iPartMaxLimit, int32_t iPartMinLimit,
                        int32_t dPartMaxLimit, int32_t dPartMinLimit, int32_t pidOutputMaxLimit, int32_t pidOutputMinLimit );
void pidControli32 ( pidci32_t* driver, int32_t error );
int32_t pidGetOutputi32 ( const pidci32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* PID_H_ */

#ifndef SLEW_H_
#define SLEW_H_

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
    float maxStep;
    float output;
} slew_t;

typedef struct
{
    int32_t maxStep;
    int32_t output;
} slewi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t slewInit ( slew_t* driver, float maxStep, float outputInit );
void slewIteration ( slew_t* driver, float newData );
float slewGetOutput ( const slew_t* const driver );

uint8_t slewIniti32 ( slewi32_t* driver, int32_t maxStep, int32_t outputInit );
void slewIterationi32 ( slewi32_t* driver, int32_t newData );
int32_t slewGetOutputi32 ( const slewi32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* SLEW_H_ */

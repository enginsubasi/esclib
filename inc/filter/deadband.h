#ifndef DEADBAND_H_
#define DEADBAND_H_

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
    float threshold;
    uint8_t mode;
    float output;
} deadband_t;

typedef struct
{
    int32_t threshold;
    uint8_t mode;
    int32_t output;
} deadbandi32_t;

/* ENUMS */

enum DEADBAND_MODE
{
    DB_SNAP             = 0,
    DB_DRAG             = 1
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t deadbandInit ( deadband_t* driver, float threshold, uint8_t mode, float outputInit );
void deadbandIteration ( deadband_t* driver, float newData );
float deadbandGetOutput ( const deadband_t* const driver );

uint8_t deadbandIniti32 ( deadbandi32_t* driver, int32_t threshold, uint8_t mode, int32_t outputInit );
void deadbandIterationi32 ( deadbandi32_t* driver, int32_t newData );
int32_t deadbandGetOutputi32 ( const deadbandi32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* DEADBAND_H_ */

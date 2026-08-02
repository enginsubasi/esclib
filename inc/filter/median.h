#ifndef MEDIAN_H_
#define MEDIAN_H_

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
    float* buffer;
    float* sorted;
    uint32_t length;
    uint32_t index;
    float output;
} median_t;

typedef struct
{
    uint32_t* buffer;
    uint32_t* sorted;
    uint32_t length;
    uint32_t index;
    uint32_t output;
} medianu32_t;

typedef struct
{
    int32_t* buffer;
    int32_t* sorted;
    uint32_t length;
    uint32_t index;
    int32_t output;
} mediani32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t medianInit ( median_t* driver, float* buffer, float* sorted, uint32_t length, float outputInit );
void medianIteration ( median_t* driver, float newData );
float medianGetOutput ( const median_t* const driver );

uint8_t medianInitu32 ( medianu32_t* driver, uint32_t* buffer, uint32_t* sorted, uint32_t length, uint32_t outputInit );
void medianIterationu32 ( medianu32_t* driver, uint32_t newData );
uint32_t medianGetOutputu32 ( const medianu32_t* const driver );

uint8_t medianIniti32 ( mediani32_t* driver, int32_t* buffer, int32_t* sorted, uint32_t length, int32_t outputInit );
void medianIterationi32 ( mediani32_t* driver, int32_t newData );
int32_t medianGetOutputi32 ( const mediani32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* MEDIAN_H_ */

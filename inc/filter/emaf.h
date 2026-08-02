#ifndef EMAF_H_
#define EMAF_H_

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
    float alpha;
    float alphan;
    float output;
} emaf_t;

typedef struct
{
    float alpha;
    float alphan;
    float accumulator;
} emafu32_t;

typedef struct
{
    int32_t accumulator;
    int32_t output;
    uint8_t shift;
} emafi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t emafInit ( emaf_t* driver, float alpha, float outputInit );
void emafIteration ( emaf_t* driver, float newData );
float emafGetOutput ( const emaf_t* const driver );

uint8_t emafInitu32 ( emafu32_t* driver, float alpha, uint32_t outputInit );
void emafIterationu32 ( emafu32_t* driver, uint32_t newData );
uint32_t emafGetOutputu32 ( const emafu32_t* const driver );

uint8_t emafIniti32 ( emafi32_t* driver, uint8_t shift, int32_t outputInit );
void emafIterationi32 ( emafi32_t* driver, int32_t newData );
int32_t emafGetOutputi32 ( const emafi32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* EMAF_H_ */

#ifndef MAF_H_
#define MAF_H_

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
    uint32_t length;
    uint32_t index;
    float sumOfArray;
    float output;
} maf_t;

typedef struct
{
    uint32_t* buffer;
    uint32_t length;
    uint32_t index;
    uint32_t sumOfArray;
    uint32_t output;
} mafu32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t mafInit ( maf_t* driver, float* buffer, uint32_t length, float outputInit );
void mafIteration ( maf_t* driver, float newData );
float mafGetOutput ( maf_t* driver );

int8_t mafInitu32 ( mafu32_t* driver, uint32_t* buffer, uint32_t length, uint32_t outputInit );
void mafIterationu32 ( mafu32_t* driver, uint32_t newData );
uint32_t mafGetOutputu32 ( mafu32_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* MAF_H_ */

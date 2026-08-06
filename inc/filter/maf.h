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

typedef struct
{
    int32_t* buffer;
    uint32_t length;
    uint32_t index;
    int32_t sumOfArray;
    int32_t output;
} mafi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t mafInit ( maf_t* driver, float* buffer, uint32_t length, float outputInit );
void mafIteration ( maf_t* driver, float newData );
float mafGetOutput ( const maf_t* const driver );

uint8_t mafInitu32 ( mafu32_t* driver, uint32_t* buffer, uint32_t length, uint32_t outputInit );
void mafIterationu32 ( mafu32_t* driver, uint32_t newData );
uint32_t mafGetOutputu32 ( const mafu32_t* const driver );

uint8_t mafIniti32 ( mafi32_t* driver, int32_t* buffer, uint32_t length, int32_t outputInit );
void mafIterationi32 ( mafi32_t* driver, int32_t newData );
int32_t mafGetOutputi32 ( const mafi32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* MAF_H_ */

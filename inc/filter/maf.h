#ifndef MAF_H_
#define MAF_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#define MAX_FILTER_LENGTH       32UL

#define TRUE    1
#define FALSE   0

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    float buffer[ MAX_FILTER_LENGTH ];
    uint32_t length;
    uint32_t index;
    float sumOfArray;
    float output;
} maf_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t mafInit ( maf_t* driver, uint32_t length, float outputInit );
void mafIteration ( maf_t* driver, float newData );
float mafGetOutput ( maf_t* driver );

#endif /* MAF_H_ */

#ifndef MAF_H_
#define MAF_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#define MAX_FILTER_LENGTH       32UL

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

/* STRUCTURES */

typedef struct
{
    double buffer[ MAX_FILTER_LENGTH ];
    uint32_t length;
    uint32_t index;
    double sumOfArray;
    double output;
} maf_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t mafInit ( maf_t* driver, uint32_t length, double outputInit );
void mafIteration ( maf_t* driver, double newData );
double mafGetOutput ( maf_t* driver );

#endif /* MAF_H_ */

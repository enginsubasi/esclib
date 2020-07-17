#ifndef EMAF_H_
#define EMAF_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#define TRUE    1
#define FALSE   0

/* STRUCTURES */

typedef struct
{
    double alpha;
    double alphan;
    double output;
} emaf_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t emafInit ( emaf_t* driver, double alpha, double outputInit );
void emafIteration ( emaf_t* driver, double newData );
double emafGetOutput ( emaf_t* driver );

#endif /* EMAF_H_ */

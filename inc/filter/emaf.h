#ifndef EMAF_H_
#define EMAF_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

/* STRUCTURES */

typedef struct
{
    double alpha;
    double alphan;
    double output;
} Emaf_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t Emaf_Init ( Emaf_t* driver, double alpha, double outputInit );
void Emaf_Iteration ( Emaf_t* driver, double newData );
double Emaf_GetOutput ( Emaf_t* driver );

#endif /* EMAF_H_ */

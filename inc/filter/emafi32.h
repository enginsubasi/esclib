#ifndef EMAFI32_H_
#define EMAFI32_H_

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
    int32_t accumulator;
    int32_t output;
    uint8_t shift;
} emafi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t emafi32Init ( emafi32_t* driver, uint8_t shift, int32_t outputInit );
void emafi32Iteration ( emafi32_t* driver, int32_t newData );
int32_t emafi32GetOutput ( const emafi32_t* const driver );

#ifdef __cplusplus
}
#endif

#endif /* EMAFI32_H_ */

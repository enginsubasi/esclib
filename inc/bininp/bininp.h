#ifndef BININP_H_
#define BININP_H_

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
    uint32_t filterCounter;
    uint32_t filterCount;
    
    uint8_t output;
} bininp_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void bininpInit ( bininp_t* driver, uint32_t filterCount );
void bininpUpdate ( bininp_t* driver, uint8_t newData );
uint8_t bininpGetValue ( bininp_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* BININP_H_ */


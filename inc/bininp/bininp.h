#ifndef BININP_H_
#define BININP_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint32_t filterCounter;
    uint32_t filterCount;
    uint8_t (*readBininp) ( void );
    
    uint8_t output;
} bininp_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t bininpInit ( bininp_t* driver, uint32_t filterCount );
void bininpUpdate ( bininp_t* driver );
uint8_t bininpGetValue ( bininp_t* driver );

#endif /* BININP_H_ */


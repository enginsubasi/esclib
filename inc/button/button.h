#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint32_t filterCounter;
    uint32_t filterCount;
    uint8_t (*readButton) ( void );
    
    uint8_t output;
} button_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t buttonInit ( button_t* driver, uint32_t filterCount );
void buttonUpdate ( button_t* driver );
uint8_t buttonGetValue ( button_t* driver );

#endif /* BUTTON_H_ */


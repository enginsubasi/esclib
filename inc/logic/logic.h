#ifndef GENERIC_H_
#define GENERIC_H_

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

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t rsff ( uint8_t r, uint8_t s, uint8_t* mem );
uint8_t dff ( uint8_t d, uint8_t* mem );

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_H_ */   

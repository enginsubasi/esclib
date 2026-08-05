#ifndef CHECKSUM_H_
#define CHECKSUM_H_

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

uint8_t checksumXor ( const uint8_t* const array, uint32_t size );
uint8_t checksumSum8 ( const uint8_t* const array, uint32_t size );
uint16_t checksumSum16 ( const uint8_t* const array, uint32_t size );
uint16_t checksumFletcher16 ( const uint8_t* const array, uint32_t size );
uint32_t checksumAdler32 ( const uint8_t* const array, uint32_t size );

#ifdef __cplusplus
}
#endif

#endif /* CHECKSUM_H_ */

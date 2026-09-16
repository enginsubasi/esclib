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

/* Where the two stateful checksums start. Adler32's first accumulator begins
   at one rather than at zero, which is the thing to get wrong. */
#define CHECKSUM_FLETCHER16_SEED    0x0000u
#define CHECKSUM_ADLER32_SEED       0x00000001u

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
uint16_t checksumFletcher16Update ( uint16_t checksum, uint8_t data );
uint32_t checksumAdler32Update ( uint32_t checksum, uint8_t data );

#ifdef __cplusplus
}
#endif

#endif /* CHECKSUM_H_ */

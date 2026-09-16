#ifndef CRC32_H_
#define CRC32_H_

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

/* Where a streaming CRC32 starts. crc32 ( a, n ) is this seed run through
   crc32Update once for every byte. */
#define CRC32_SEED              0xFFFFFFFFu

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint32_t crc32 ( const uint8_t* const array, uint32_t size );
uint32_t crc32Alt ( const uint8_t* const array, uint32_t size );
uint32_t crc32Update ( uint32_t crc, uint8_t data );
uint32_t crc32AltUpdate ( uint32_t crc, uint8_t data );

#ifdef __cplusplus
}
#endif

#endif /* CRC32_H_ */

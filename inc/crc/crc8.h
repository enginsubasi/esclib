#ifndef CRC8_H_
#define CRC8_H_

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

/* Where a streaming CRC8 starts. Both polynomials seed with zero, which is
   what makes a buffer carrying its own CRC check to zero. */
#define CRC8_SEED               0x00u
#define CRC8_DALLAS_SEED        0x00u

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t crc8 ( const uint8_t* const array, uint32_t size );
uint8_t crc8Dallas ( const uint8_t* const array, uint32_t size );
uint8_t crc8Update ( uint8_t crc, uint8_t data );
uint8_t crc8DallasUpdate ( uint8_t crc, uint8_t data );

#ifdef __cplusplus
}
#endif

#endif /* CRC8_H_ */

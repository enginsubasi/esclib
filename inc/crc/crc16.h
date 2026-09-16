#ifndef CRC16_H_
#define CRC16_H_

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

/* Where a streaming CRC16 starts. crc16 ( a, n ) is this seed run through
   crc16Update once for every byte. */
#define CRC16_SEED              0xFFFFu

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint16_t crc16 ( const uint8_t* const array, uint32_t size );
uint16_t crc16Alt ( const uint8_t* const array, uint32_t size );
uint16_t crc16Update ( uint16_t crc, uint8_t data );
uint16_t crc16AltUpdate ( uint16_t crc, uint8_t data );

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H_ */

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

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t crc8 ( const uint8_t* const array, uint32_t size );
uint8_t crc8Dallas ( const uint8_t* const array, uint32_t size );

#ifdef __cplusplus
}
#endif

#endif /* CRC8_H_ */
